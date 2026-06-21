#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <SoftwareSerial.h>
#include <PZEM004Tv30.h>

// =========================================================================
// 1. PENGATURAN WI-FI & AWS IOT CORE
// =========================================================================
const char* WIFI_SSID     = " ";
const char* WIFI_PASSWORD = " ";

const char* AWS_IOT_ENDPOINT = " "; 
const char* AWS_IOT_CLIENT_ID = " ";
const char* AWS_IOT_PUBLISH_TOPIC   = " "; 
const char* AWS_IOT_SUBSCRIBE_TOPIC = " ";   

// =========================================================================
// 2. SERTIFIKAT AWS
// =========================================================================
static const char AWS_CERT_CA[] PROGMEM = R"EOF(
-----BEGIN CERTIFICATE-----

-----END CERTIFICATE-----
)EOF";

static const char AWS_CERT_CRT[] PROGMEM = R"EOF(
-----BEGIN CERTIFICATE-----

-----END CERTIFICATE-----
)EOF";

static const char AWS_CERT_PRIVATE[] PROGMEM = R"EOF(
-----BEGIN RSA PRIVATE KEY-----

-----END RSA PRIVATE KEY-----
)EOF";

// =========================================================================
// 3. KONFIGURASI PIN & SENSOR
// =========================================================================
#define RELAY_1 D1 
#define RELAY_2 D2 
#define RELAY_3 D5 
#define RELAY_4 D6 

SoftwareSerial pzemSerial(D7, D4); // RX (ke TX PZEM), TX (ke RX PZEM)
PZEM004Tv30 pzem(pzemSerial);

SoftwareSerial camSerial(D3, D0);  // RX (dari TX ESP32-CAM), TX (tidak dipakai)

// =========================================================================
// 4. VARIABEL SISTEM
// =========================================================================
WiFiClientSecure net;
PubSubClient client(net);

bool isAutoMode = true; 
int detectedStudents = 0;

// Status masing-masing relay agar mandiri
bool r1_status = false;
bool r2_status = false;
bool r3_status = false;
bool r4_status = false;

unsigned long lastPublishTime = 0;
unsigned long lastReconnectAttempt = 0;

// =========================================================================
// 5. FUNGSI KONTROL RELAY
// =========================================================================
void kontrolRelay(int pin, bool nyala, bool &statusVariabel) {
  if (nyala) {
    digitalWrite(pin, LOW); // Aktif LOW
    statusVariabel = true;
  } else {
    digitalWrite(pin, HIGH); // Aktif LOW
    statusVariabel = false;
  }
}

void nyalakanSemuaRelay() {
  kontrolRelay(RELAY_1, true, r1_status);
  kontrolRelay(RELAY_2, true, r2_status);
  kontrolRelay(RELAY_3, true, r3_status);
  kontrolRelay(RELAY_4, true, r4_status);
  Serial.println("[SISTEM] Semua Relay DINYALAKAN otomatis (Kuota Terpenuhi)");
}

void matikanSemuaRelay() {
  kontrolRelay(RELAY_1, false, r1_status);
  kontrolRelay(RELAY_2, false, r2_status);
  kontrolRelay(RELAY_3, false, r3_status);
  kontrolRelay(RELAY_4, false, r4_status);
  Serial.println("[SISTEM] Semua Relay DIMATIKAN otomatis (Kuota Kosong/Kurang)");
}

// =========================================================================
// 6. FUNGSI PENERIMA PERINTAH (KONTROL JARAK JAUH AWS)
// =========================================================================
void messageHandler(char* topic, byte* payload, unsigned int length) {
  StaticJsonDocument<256> doc;
  deserializeJson(doc, payload);

  if (doc.containsKey("mode")) {
    String mode = doc["mode"].as<String>();
    if (mode == "manual") {
      isAutoMode = false;
      Serial.println("[AWS] Sistem Beralih: MANUAL");
    }
    else if (mode == "auto") {
      isAutoMode = true;
      Serial.println("[AWS] Sistem Beralih: OTOMATIS");
    }
  }

  if (!isAutoMode) {
    if (doc.containsKey("r1")) {
      String cmd = doc["r1"].as<String>();
      if (cmd == "on") kontrolRelay(RELAY_1, true, r1_status);
      else if (cmd == "off") kontrolRelay(RELAY_1, false, r1_status);
      Serial.printf("[AWS] Relay 1 Manual: %s\n", cmd.c_str());
    }
    if (doc.containsKey("r2")) {
      String cmd = doc["r2"].as<String>();
      if (cmd == "on") kontrolRelay(RELAY_2, true, r2_status);
      else if (cmd == "off") kontrolRelay(RELAY_2, false, r2_status);
      Serial.printf("[AWS] Relay 2 Manual: %s\n", cmd.c_str());
    }
    if (doc.containsKey("r3")) {
      String cmd = doc["r3"].as<String>();
      if (cmd == "on") kontrolRelay(RELAY_3, true, r3_status);
      else if (cmd == "off") kontrolRelay(RELAY_3, false, r3_status);
      Serial.printf("[AWS] Relay 3 Manual: %s\n", cmd.c_str());
    }
    if (doc.containsKey("r4")) {
      String cmd = doc["r4"].as<String>();
      if (cmd == "on") kontrolRelay(RELAY_4, true, r4_status);
      else if (cmd == "off") kontrolRelay(RELAY_4, false, r4_status);
      Serial.printf("[AWS] Relay 4 Manual: %s\n", cmd.c_str());
    }
  }
}

// =========================================================================
// 7. SETUP (Revisi Non-Blocking)
// =========================================================================
void setup() {
  Serial.begin(115200);
  camSerial.begin(9600);
  camSerial.setTimeout(50); 
  
  pinMode(RELAY_1, OUTPUT);
  pinMode(RELAY_2, OUTPUT);
  pinMode(RELAY_3, OUTPUT);
  pinMode(RELAY_4, OUTPUT);

  matikanSemuaRelay();

  // Memulai proses Wi-Fi tanpa menunggunya (Non-blocking)
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  
  // Mengatur target sinkronisasi NTP (Akan sinkron otomatis saat Wi-Fi terhubung nanti)
  configTime(7 * 3600, 0, "pool.ntp.org", "time.nist.gov");

  // Konfigurasi Sertifikat
  net.setTrustAnchors(new X509List(AWS_CERT_CA));
  net.setClientRSACert(new X509List(AWS_CERT_CRT), new PrivateKey(AWS_CERT_PRIVATE));

  client.setServer(AWS_IOT_ENDPOINT, 8883);
  client.setCallback(messageHandler);
  client.setBufferSize(512); 

  Serial.println("\n===========================================");
  Serial.println("Sistem Smart Classroom Dimulai!");
  Serial.println("Mode Lokal: AKTIF | AWS IoT: MENUNGGU KONEKSI...");
  Serial.println("===========================================\n");
}

// =========================================================================
// 8. KONEKSI NON-BLOCKING (Revisi)
// =========================================================================
void handleConnection() {
  // 1. Jika Wi-Fi belum terhubung, keluar dari fungsi ini dan biarkan alat bekerja lokal
  if (WiFi.status() != WL_CONNECTED) {
    return; 
  }

  // 2. Jika Wi-Fi terhubung, cek apakah NTP sudah berhasil sinkron (Wajib untuk AWS)
  time_t now = time(nullptr);
  if (now < 1600000000) {
    return; // Waktu belum valid, jangan paksa konek ke AWS dulu
  }

  // 3. Jika Wi-Fi dan Waktu sudah aman, baru urus koneksi MQTT AWS
  if (!client.connected()) {
    if (millis() - lastReconnectAttempt > 5000) {
      lastReconnectAttempt = millis();
      
      Serial.print("[AWS] Mencoba koneksi ke AWS IoT... ");
      if (client.connect(AWS_IOT_CLIENT_ID)) {
        Serial.println("TERHUBUNG!");
        client.subscribe(AWS_IOT_SUBSCRIBE_TOPIC);
      } else {
        Serial.println("Gagal.");
      }
    }
  } else {
    // Jaga koneksi tetap hidup dan dengarkan perintah manual
    client.loop(); 
  }
}

// =========================================================================
// 9. LOOP UTAMA 
// =========================================================================
void loop() {
  
  handleConnection(); 

  // --- BACA DATA KAMERA ---
  camSerial.listen(); 
  if (camSerial.available()) {
    String dataKamera = camSerial.readStringUntil('\n');
    dataKamera.trim();
    
    if (dataKamera.length() > 0) {
      if (dataKamera == "0" || dataKamera == "3" || dataKamera == "9" || dataKamera == "20") {
        detectedStudents = dataKamera.toInt();
        
        // --- PRINT DATA KAMERA KE SERIAL MONITOR ---
        Serial.print("[KAMERA] Update: Terdeteksi ");
        Serial.print(detectedStudents);
        Serial.println(" Mahasiswa");
        // ------------------------------------------
      }
    }
  }

  // --- LOGIKA HIRARKI OTOMATIS (Lokal) ---
  if (isAutoMode) {
    if (detectedStudents >= 5) {
      if (!r1_status) nyalakanSemuaRelay();
    } else {
      if (r1_status) matikanSemuaRelay();
    }
  }

  // --- BACA PZEM & PUBLISH KE AWS (Setiap 5 Detik) ---
  if (millis() - lastPublishTime > 5000) {
    pzemSerial.listen(); 
    delay(50); 
    
    float voltage = pzem.voltage();
    float current = pzem.current();
    float power = pzem.power();
    
    // --- PRINT DATA PZEM KE SERIAL MONITOR ---
    if(isnan(voltage)) {
      Serial.println("[PZEM] Error: Gagal membaca data dari sensor!");
    } else {
      Serial.printf("[PZEM] Tegangan: %.2fV | Arus: %.2fA | Daya: %.2fW\n", voltage, current, power);
    }
    // ------------------------------------------

    if (client.connected()) {
      StaticJsonDocument<256> doc;
      doc["mode"] = isAutoMode ? "auto" : "manual";
      doc["students"] = detectedStudents;
      doc["r1"] = r1_status ? "on" : "off";
      doc["r2"] = r2_status ? "on" : "off";
      doc["r3"] = r3_status ? "on" : "off";
      doc["r4"] = r4_status ? "on" : "off";
      
      if(!isnan(voltage)) {
        doc["voltage"] = voltage;
        doc["ampere"] = current;
        doc["power"] = power;
      } else {
        doc["voltage"] = 0;
        doc["ampere"] = 0;
        doc["power"] = 0;
      }

      char jsonBuffer[512];
      serializeJson(doc, jsonBuffer);
      client.publish(AWS_IOT_PUBLISH_TOPIC, jsonBuffer);
      
      // Print indikator bahwa data telah terkirim ke cloud
      Serial.println("[AWS] Payload JSON berhasil dipublish ke Cloud.");
    }
    
    lastPublishTime = millis();
  }
}
