import paho.mqtt.client as mqtt
import json
import os
import ssl
import asyncio
import threading
import boto3
from datetime import datetime
from dotenv import load_dotenv

from database import save_sensor, save_relay_log

load_dotenv()

ESP_TO_ROOM = {
    "esp1": "room-101",
    "esp2": "room-102"
}

TOPICS = [
    "scems/energy/esp1",
    "scems/energy/esp2",
]

S3_BUCKET = os.getenv("S3_BUCKET", "s3-iotproyek-abizhar-2")


class MQTTClient:
    def __init__(self, manager):
        self.manager = manager
        self.client  = None
        self.loop    = None
        self.s3      = boto3.client("s3")

        # State relay terakhir per ESP untuk deteksi perubahan auto
        self.last_relay = {
            "esp1": {"r1": None, "r2": None, "r3": None, "r4": None},
            "esp2": {"r1": None, "r2": None, "r3": None, "r4": None}
        }

    def start(self):
        self.loop   = asyncio.get_running_loop()
        self.client = mqtt.Client(client_id="fastapi-scems")
        self.client.on_connect = self._on_connect
        self.client.on_message = self._on_message

        cert_dir  = os.path.join(os.path.dirname(__file__), "certs")
        ca_cert   = os.path.join(cert_dir, "AmazonRootCA1.pem")
        cert_file = os.path.join(cert_dir, "esp8266-01.cert.pem")
        key_file  = os.path.join(cert_dir, "esp8266-01.private.key")

        self.client.tls_set(
            ca_certs    = ca_cert,
            certfile    = cert_file,
            keyfile     = key_file,
            tls_version = ssl.PROTOCOL_TLSv1_2
        )

        broker = os.getenv("IOT_ENDPOINT", "a1jat3ondn4obc-ats.iot.ap-southeast-1.amazonaws.com")
        port   = 8883

        self.client.connect(broker, port, keepalive=60)
        thread = threading.Thread(target=self.client.loop_forever, daemon=True)
        thread.start()
        print(f"[MQTT] Connected to {broker}:{port}")

    def stop(self):
        if self.client:
            self.client.disconnect()

    def publish(self, topic: str, payload: str):
        if self.client:
            self.client.publish(topic, payload)
            print(f"[MQTT] Published → {topic}: {payload}")

    def _on_connect(self, client, userdata, flags, rc):
        if rc == 0:
            for topic in TOPICS:
                client.subscribe(topic)
                print(f"[MQTT] Subscribed: {topic}")
        else:
            print(f"[MQTT] Connect failed rc={rc}")

    def _on_message(self, client, userdata, msg):
        try:
            topic   = msg.topic
            payload = json.loads(msg.payload.decode())

            esp     = "esp1" if "esp1" in topic else "esp2"
            room_id = ESP_TO_ROOM.get(esp, "room-101")

            print(f"[MQTT] {topic} → {payload}")

            # 1. Simpan ke RDS
            save_sensor(esp, payload)

            # 2. Upload ke S3
            self._upload_s3(esp, payload)

            # 3. Broadcast sensor ke WebSocket
            asyncio.run_coroutine_threadsafe(
                self.manager.broadcast({
                    "type":    "sensor",
                    "room_id": room_id,
                    "data": {
                        "volt":   payload.get("voltage", 0),
                        "ampere": payload.get("ampere",  0),
                        "watt":   payload.get("power",   0),
                    }
                }),
                self.loop
            )

            # 4. Broadcast occupancy ke WebSocket
            students = payload.get("students", 0)
            asyncio.run_coroutine_threadsafe(
                self.manager.broadcast({
                    "type":    "occupancy",
                    "room_id": room_id,
                    "data": {
                        "person_count": students,
                        "is_occupied":  students >= 4
                    }
                }),
                self.loop
            )

            # 5. Deteksi perubahan relay AUTO
            # Cek setiap r1-r4 apakah berubah dari state sebelumnya
            mode = payload.get("mode", "auto")
            for i in range(1, 5):
                key      = f"r{i}"
                relay_id = f"relay-{i}"
                new_val  = payload.get(key, "off")
                old_val  = self.last_relay[esp].get(key)

                # Hanya catat jika:
                # 1. Ada perubahan nilai relay
                # 2. Bukan pertama kali (old_val != None)
                if old_val is not None and new_val != old_val:
                    status = (new_val == "on")

                    print(f"[AUTO] {esp} {relay_id}: {old_val} → {new_val} (mode: {mode})")

                    # Simpan ke DB dengan source sesuai mode
                    save_relay_log(room_id, relay_id, status, source=mode)

                    # Broadcast ke dashboard
                    asyncio.run_coroutine_threadsafe(
                        self.manager.broadcast({
                            "type":    "relay",
                            "room_id": room_id,
                            "data": {
                                "relay_id": relay_id,
                                "status":   status,
                                "source":   mode
                            }
                        }),
                        self.loop
                    )

                # Update state terakhir
                self.last_relay[esp][key] = new_val

        except Exception as e:
            print(f"[MQTT] on_message error: {e}")

    def _upload_s3(self, esp: str, payload: dict):
        try:
            now = datetime.utcnow()
            key = f"raw/telemetry/{esp}/{int(now.timestamp())}.json"
            self.s3.put_object(
                Bucket      = S3_BUCKET,
                Key         = key,
                Body        = json.dumps(payload),
                ContentType = "application/json"
            )
        except Exception as e:
            print(f"[S3] Upload error: {e}")
