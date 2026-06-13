import mysql.connector
import os
from datetime import datetime
from dotenv import load_dotenv

load_dotenv()

def get_conn():
    return mysql.connector.connect(
        host     = os.getenv("DB_HOST"),
        database = os.getenv("DB_NAME", "iot_db"),
        user     = os.getenv("DB_USER", "adminrds"),
        password = os.getenv("DB_PASSWORD"),
        connect_timeout = 5
    )

def save_sensor(esp: str, payload: dict):
    try:
        conn = get_conn()
        cur  = conn.cursor()
        cur.execute("""
            CREATE TABLE IF NOT EXISTS sensor_data (
                id        INT AUTO_INCREMENT PRIMARY KEY,
                esp       VARCHAR(10)  NOT NULL,
                mode      VARCHAR(10),
                students  INT          DEFAULT 0,
                r1        VARCHAR(5),
                r2        VARCHAR(5),
                r3        VARCHAR(5),
                r4        VARCHAR(5),
                voltage   FLOAT        DEFAULT 0,
                ampere    FLOAT        DEFAULT 0,
                power     FLOAT        DEFAULT 0,
                timestamp DATETIME     DEFAULT CURRENT_TIMESTAMP,
                INDEX idx_esp_ts (esp, timestamp DESC)
            )
        """)
        cur.execute("""
            INSERT INTO sensor_data
              (esp, mode, students, r1, r2, r3, r4, voltage, ampere, power)
            VALUES (%s, %s, %s, %s, %s, %s, %s, %s, %s, %s)
        """, (
            esp,
            payload.get("mode",     "auto"),
            payload.get("students", 0),
            payload.get("r1",       "off"),
            payload.get("r2",       "off"),
            payload.get("r3",       "off"),
            payload.get("r4",       "off"),
            payload.get("voltage",  0),
            payload.get("ampere",   0),
            payload.get("power",    0),
        ))
        conn.commit()
    except Exception as e:
        print(f"[DB] save_sensor error: {e}")
    finally:
        try: conn.close()
        except: pass


def get_latest_sensor(esp: str) -> dict | None:
    try:
        conn = get_conn()
        cur  = conn.cursor(dictionary=True)
        cur.execute("""
            SELECT * FROM sensor_data
            WHERE esp = %s
            ORDER BY timestamp DESC
            LIMIT 1
        """, (esp,))
        return cur.fetchone()
    except Exception as e:
        print(f"[DB] get_latest_sensor error: {e}")
        return None
    finally:
        try: conn.close()
        except: pass

def get_latest(esp: str) -> dict | None:
    return get_latest_sensor(esp)


def save_relay_log(room_id: str, relay_id: str, status: bool, source: str = "manual"):
    """Simpan log relay — source: 'manual' atau 'auto'."""
    try:
        conn = get_conn()
        cur  = conn.cursor()
        cur.execute("""
            CREATE TABLE IF NOT EXISTS relay_log (
                id         INT AUTO_INCREMENT PRIMARY KEY,
                room_id    VARCHAR(20) NOT NULL,
                relay_id   VARCHAR(20) NOT NULL,
                status     TINYINT(1)  NOT NULL,
                source     VARCHAR(10) DEFAULT 'manual',
                changed_at DATETIME    DEFAULT CURRENT_TIMESTAMP,
                INDEX idx_room_ts (room_id, changed_at DESC)
            )
        """)
        cur.execute("""
            INSERT INTO relay_log (room_id, relay_id, status, source)
            VALUES (%s, %s, %s, %s)
        """, (room_id, relay_id, 1 if status else 0, source))
        conn.commit()
    except Exception as e:
        print(f"[DB] save_relay_log error: {e}")
    finally:
        try: conn.close()
        except: pass


def get_relay_log(room_id: str, limit: int = 30) -> list:
    try:
        conn = get_conn()
        cur  = conn.cursor(dictionary=True)
        cur.execute("""
            SELECT relay_id, status, source, changed_at
            FROM relay_log
            WHERE room_id = %s
            ORDER BY changed_at DESC
            LIMIT %s
        """, (room_id, limit))
        rows = cur.fetchall()
        return [
            {
                "relay_id":   r["relay_id"],
                "status":     bool(r["status"]),
                "source":     r.get("source", "manual"),
                "changed_at": r["changed_at"].isoformat() if r["changed_at"] else None
            }
            for r in rows
        ]
    except Exception as e:
        print(f"[DB] get_relay_log error: {e}")
        return []
    finally:
        try: conn.close()
        except: pass
