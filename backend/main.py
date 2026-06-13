from fastapi import FastAPI, WebSocket, WebSocketDisconnect, HTTPException
from fastapi.middleware.cors import CORSMiddleware
from fastapi.staticfiles import StaticFiles
from fastapi.responses import FileResponse
from pydantic import BaseModel
from typing import Optional
import asyncio
import json
import os

from database import get_latest, get_relay_log, save_relay_log, get_latest_sensor
from mqtt_client import MQTTClient

app = FastAPI(title="SCEMS API")

app.add_middleware(
    CORSMiddleware,
    allow_origins=["*"],
    allow_methods=["*"],
    allow_headers=["*"],
)

ROOM_TO_ESP = {
    "room-101": "esp1",
    "room-102": "esp2"
}

ESP_TO_ROOM = {v: k for k, v in ROOM_TO_ESP.items()}

# ═══════════════════════════════════════════
# WEBSOCKET MANAGER
# ═══════════════════════════════════════════
class ConnectionManager:
    def __init__(self):
        self.active: list[WebSocket] = []

    async def connect(self, ws: WebSocket):
        await ws.accept()
        self.active.append(ws)

    def disconnect(self, ws: WebSocket):
        if ws in self.active:
            self.active.remove(ws)

    async def broadcast(self, data: dict):
        msg = json.dumps(data)
        dead = []
        for ws in self.active:
            try:
                await ws.send_text(msg)
            except Exception:
                dead.append(ws)
        for ws in dead:
            self.disconnect(ws)

manager = ConnectionManager()
mqtt = MQTTClient(manager)

# State relay terakhir per ESP untuk deteksi perubahan auto
last_relay_state = {
    "esp1": {"r1": "off", "r2": "off", "r3": "off", "r4": "off"},
    "esp2": {"r1": "off", "r2": "off", "r3": "off", "r4": "off"}
}

@app.on_event("startup")
async def startup():
    mqtt.start()

@app.on_event("shutdown")
async def shutdown():
    mqtt.stop()

# ═══════════════════════════════════════════
# WEBSOCKET ENDPOINT
# ═══════════════════════════════════════════
@app.websocket("/ws")
async def websocket_endpoint(ws: WebSocket):
    await manager.connect(ws)
    try:
        while True:
            await asyncio.sleep(20)
            try:
                await ws.send_text(json.dumps({"type": "ping"}))
            except Exception:
                break
    except WebSocketDisconnect:
        pass
    finally:
        manager.disconnect(ws)

# ═══════════════════════════════════════════
# REST API ENDPOINTS
# ═══════════════════════════════════════════

@app.get("/api/dashboard/overview")
async def overview():
    rooms = []
    for room_id, esp in ROOM_TO_ESP.items():
        sensor    = get_latest_sensor(esp)
        relay_log = get_relay_log(room_id, limit=1)

        relays = []
        for i in range(1, 5):
            relay_id = f"relay-{i}"
            key = f"r{i}"
            status = False
            if sensor and key in sensor:
                status = (sensor[key] == "on")
            relays.append({"relay_id": relay_id, "status": status})

        rooms.append({
            "room_id": room_id,
            "sensor": {
                "volt":   sensor.get("voltage", 0) if sensor else 0,
                "ampere": sensor.get("ampere", 0)  if sensor else 0,
                "watt":   sensor.get("power", 0)   if sensor else 0,
            } if sensor else None,
            "occupancy": {
                "person_count": sensor.get("students", 0) if sensor else 0,
                "is_occupied":  (sensor.get("students", 0) >= 4) if sensor else False,
            } if sensor else None,
            "relays": relays
        })
    return {"rooms": rooms}


# ═══════════════════════════════════════════
# MODE ENDPOINT
# ═══════════════════════════════════════════
class ModeCommand(BaseModel):
    mode: str

@app.post("/api/mode/{room_id}")
async def set_mode(room_id: str, cmd: ModeCommand):
    esp = ROOM_TO_ESP.get(room_id)
    if not esp:
        raise HTTPException(status_code=404, detail="Room tidak ditemukan")

    topic   = f"scems/relay/{esp}/control"
    payload = json.dumps({"mode": cmd.mode})
    mqtt.publish(topic, payload)

    return {"ok": True, "mode": cmd.mode}


# ═══════════════════════════════════════════
# RELAY ENDPOINT
# ═══════════════════════════════════════════
class RelayCommand(BaseModel):
    status: bool

@app.post("/api/relay/{room_id}/{relay_id}")
async def control_relay(room_id: str, relay_id: str, cmd: RelayCommand):
    esp = ROOM_TO_ESP.get(room_id)
    if not esp:
        raise HTTPException(status_code=404, detail="Room tidak ditemukan")

    num = relay_id.split("-")[1]
    key = f"r{num}"
    val = "on" if cmd.status else "off"

    topic   = f"scems/relay/{esp}/control"
    payload = json.dumps({"mode": "manual", key: val})
    mqtt.publish(topic, payload)

    save_relay_log(room_id, relay_id, cmd.status, source="manual")

    await manager.broadcast({
        "type":    "relay",
        "room_id": room_id,
        "data": {
            "relay_id": relay_id,
            "status":   cmd.status,
            "source":   "manual"
        }
    })

    return {"ok": True, "relay_id": relay_id, "status": cmd.status}


@app.get("/api/relay/{room_id}/log")
async def relay_log(room_id: str, limit: int = 30):
    logs = get_relay_log(room_id, limit=limit)
    return {"logs": logs}


# ═══════════════════════════════════════════
# ENDPOINT UNTUK MQTT CLIENT — broadcast relay auto
# ═══════════════════════════════════════════
async def broadcast_auto_relay_changes(esp: str, payload: dict):
    """Deteksi perubahan relay dari ESP (mode auto) dan broadcast ke dashboard."""
    room_id = ESP_TO_ROOM.get(esp)
    if not room_id:
        return

    for i in range(1, 5):
        key      = f"r{i}"
        relay_id = f"relay-{i}"
        new_val  = payload.get(key, "off")
        old_val  = last_relay_state[esp].get(key, "off")

        if new_val != old_val:
            status = (new_val == "on")
            last_relay_state[esp][key] = new_val

            # Simpan ke DB dengan source auto
            save_relay_log(room_id, relay_id, status, source="auto")

            # Broadcast ke dashboard
            await manager.broadcast({
                "type":    "relay",
                "room_id": room_id,
                "data": {
                    "relay_id": relay_id,
                    "status":   status,
                    "source":   "auto"
                }
            })


# ═══════════════════════════════════════════
# SERVE FRONTEND
# ═══════════════════════════════════════════
FRONTEND = os.path.join(os.path.dirname(__file__), "..", "frontend")

@app.get("/")
async def index():
    return FileResponse(os.path.join(FRONTEND, "index.html"))

app.mount("/css", StaticFiles(directory=os.path.join(FRONTEND, "css")), name="css")
app.mount("/js",  StaticFiles(directory=os.path.join(FRONTEND, "js")),  name="js")
