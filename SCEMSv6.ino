#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <SoftwareSerial.h>
#include <PZEM004Tv30.h>

// =========================================================================
// 1. PENGATURAN WI-FI & AWS IOT CORE
// =========================================================================
const char* WIFI_SSID     = "Ika Semok_real";
const char* WIFI_PASSWORD = "wifilagiwifilagi";

const char* AWS_IOT_ENDPOINT = "a1jat3ondn4obc-ats.iot.ap-southeast-1.amazonaws.com"; 
const char* AWS_IOT_CLIENT_ID = "ESP8266-01";
const char* AWS_IOT_PUBLISH_TOPIC   = "scems/energy/esp1"; 
const char* AWS_IOT_SUBSCRIBE_TOPIC = "scems/relay/esp1/control";   

// =========================================================================
// 2. SERTIFIKAT AWS
// =========================================================================
static const char AWS_CERT_CA[] PROGMEM = R"EOF(
-----BEGIN CERTIFICATE-----
MIIDQTCCAimgAwIBAgITBmyfz5m/jAo54vB4ikPmljZbyjANBgkqhkiG9w0BAQsF
ADA5MQswCQYDVQQGEwJVUzEPMA0GA1UEChMGQW1hem9uMRkwFwYDVQQDExBBbWF6
b24gUm9vdCBDQSAxMB4XDTE1MDUyNjAwMDAwMFoXDTM4MDExNzAwMDAwMFowOTEL
MAkGA1UEBhMCVVMxDzANBgNVBAoTBkFtYXpvbjEZMBcGA1UEAxMQQW1hem9uIFJv
b3QgQ0EgMTCCASIwDQYJKoZIhvcNAQEBBQADggEPADCCAQoCggEBALJ4gHHKeNXj
ca9HgFB0fW7Y14h29Jlo91ghYPl0hAEvrAIthtOgQ3pOsqTQNroBvo3bSMgHFzZM
9O6II8c+6zf1tRn4SWiw3te5djgdYZ6k/oI2peVKVuRF4fn9tBb6dNqcmzU5L/qw
IFAGbHrQgLKm+a/sRxmPUDgH3KKHOVj4utWp+UhnMJbulHheb4mjUcAwhmahRWa6
VOujw5H5SNz/0egwLX0tdHA114gk957EWW67c4cX8jJGKLhD+rcdqsq08p8kDi1L
93FcXmn/6pUCyziKrlA4b9v7LWIbxcceVOF34GfID5yHI9Y/QCB/IIDEgEw+OyQm
jgSubJrIqg0CAwEAAaNCMEAwDwYDVR0TAQH/BAUwAwEB/zAOBgNVHQ8BAf8EBAMC
AYYwHQYDVR0OBBYEFIQYzIU07LwMlJQuCFmcx7IQTgoIMA0GCSqGSIb3DQEBCwUA
A4IBAQCY8jdaQZChGsV2USggNiMOruYou6r4lK5IpDB/G/wkjUu0yKGX9rbxenDI
U5PMCCjjmCXPI6T53iHTfIUJrU6adTrCC2qJeHZERxhlbI1Bjjt/msv0tadQ1wUs
N+gDS63pYaACbvXy8MWy7Vu33PqUXHeeE6V/Uq2V8viTO96LXFvKWlJbYK8U90vv
o/ufQJVtMVT8QtPHRh8jrdkPSHCa2XV4cdFyQzR1bldZwgJcJmApzyMZFo6IQ6XU
5MsI+yMRQ+hDKXJioaldXgjUkK642M4UwtBV8ob2xJNDd2ZhwLnoQdeXeGADbkpy
rqXRfboQnoZsG4q5WTP468SQvvG5
-----END CERTIFICATE-----
)EOF";

static const char AWS_CERT_CRT[] PROGMEM = R"EOF(
-----BEGIN CERTIFICATE-----
MIIDWTCCAkGgAwIBAgIUZO8mn3VvbgEQGTA6BMleIFWHjVMwDQYJKoZIhvcNAQEL
BQAwTTFLMEkGA1UECwxCQW1hem9uIFdlYiBTZXJ2aWNlcyBPPUFtYXpvbi5jb20g
SW5jLiBMPVNlYXR0bGUgU1Q9V2FzaGluZ3RvbiBDPVVTMB4XDTI2MDUyNjA4NTUz
NloXDTQ5MTIzMTIzNTk1OVowHjEcMBoGA1UEAwwTQVdTIElvVCBDZXJ0aWZpY2F0
ZTCCASIwDQYJKoZIhvcNAQEBBQADggEPADCCAQoCggEBAN+/WB4zeGBUAuq8wo42
sydLwZsF7t7rAklHcFrcLb1xTFLHgGf4Z/iVs60grKDO2967aY4zcM2QLONodJz8
92jMHWEFyH5Nf4YFvoVoMPN6J2/GCu/KQ5QQvXXVQTCZ4/anbwX72ES6jjMDPbIb
lnSupM7j4PXNQn2Hc3R48/hpDun70gjdBKKzdI+c56oGE5fPAVjEyi64NQhyRMy4
H9HdfT+msSP8AsT2SFu7T26j1mJRo+b7ohWIkIXA9QWq+rj0W4RBEs+iRuUYYCI0
hANOobNolt9209vrcunxMZfMn/dQjx3zK44aRsXaD2K5E5lIvXhhF0xbeMem24sQ
qA8CAwEAAaNgMF4wHwYDVR0jBBgwFoAU9i7BApHocP5jqFfdxYZ4CDDiwkIwHQYD
VR0OBBYEFEsi0n8GjaBpgTruPSxr5txUrPn4MAwGA1UdEwEB/wQCMAAwDgYDVR0P
AQH/BAQDAgeAMA0GCSqGSIb3DQEBCwUAA4IBAQAQzqCFKjBmdwzQu3WhQrO85e3G
BWDSbB3WZhrf6poXo+B8C911pPjujC6VzJdUcn/+F/77MreikwYi3WsOwXJ2w7L6
U6+A55XlnQHMtpUaIzZSTQGleg3mz2LtmeulcyNR813YWI8jwkDKrWo0MI9DH7Ui
hioXwQA4a63cX2weNfBogKigvXvtpFTr7mJTsK+CDVhO/DJtDh1/8cIqlCEAD8+U
7vrMa2ijwGyHiC7oEohUMfNs5378/FzPg9+i26P5sOO7eAOD3YQvk5weeO9ryJJ1
tVq8wp7L/hDaunGP3UL7Q7VO0YnCEXRRVp95xALKNkQ9tkFzX4SIEIPJGFMJ
-----END CERTIFICATE-----
)EOF";

static const char AWS_CERT_PRIVATE[] PROGMEM = R"EOF(
-----BEGIN RSA PRIVATE KEY-----
MIIEpAIBAAKCAQEA379YHjN4YFQC6rzCjjazJ0vBmwXu3usCSUdwWtwtvXFMUseA
Z/hn+JWzrSCsoM7b3rtpjjNwzZAs42h0nPz3aMwdYQXIfk1/hgW+hWgw83onb8YK
78pDlBC9ddVBMJnj9qdvBfvYRLqOMwM9shuWdK6kzuPg9c1CfYdzdHjz+GkO6fvS
CN0EorN0j5znqgYTl88BWMTKLrg1CHJEzLgf0d19P6axI/wCxPZIW7tPbqPWYlGj
5vuiFYiQhcD1Bar6uPRbhEESz6JG5RhgIjSEA06hs2iW33bT2+ty6fExl8yf91CP
HfMrjhpGxdoPYrkTmUi9eGEXTFt4x6bbixCoDwIDAQABAoIBAHKTcXRFtybc3oGG
F7rrl+JPkNzptODCR3Fu+8ILbgDMu+DH+KFFOzi4sEG/sabHRyBPqEYBYYpighoR
q2WYxLkNjR3Z8El9NghTeLSoHikQLJG4QHF6ihCQYfStN6zDoVD1fIz74kuPnLHS
vw0tw2YM6KhpWRGGAr51VlIWfhh9/pLpMOEJrYqQx+xNdeS78i/QObYon58OsZCM
8S0JNaTxZGEY3t14KFKlPUQXOZFCsaT15rglN5ENnYOx/QtfcyMWK57z0fQNzeDG
Bb+25cksMOErpLJs8dle7yC5OV+xIPIE3C4Yf9nSiVlHJ+/8ohhMWr64bP0EhmAT
KCN+lIECgYEA+3Q/nvvv1wwzBTKCk5BOLyhzUU2+4LOY/HfpBxL7XqT5u6CICe5/
Yt1t+vC0EJk+nVALVCz5wyZ6mysRGNsLNVeSSgSFceszmKl7Shv4JoOaQc3ExqC3
PdgWdtSgR9G9R/3v2K7Y2LbaLy8ODObGHZS62cFyx4yI5sMCZF9vXpcCgYEA48rd
6tdEFqvP7oBF8dVbNBrhkKCjft9mIJZn6IT+VgeMuXx6fwGP3UYzatJudOXmkuci
KBNBxCgyCP6K/smunIkKFILZdnsa4OT9RdMAF3ELIvQB3ooprQjE3ss6n3/NEMuA
8pUGUrN4sv2ex+gckG884tY2ajFRSyILeWCrqUkCgYEAj8YIGn8yw2LWSUJ4Jqd8
DLq0NOifGxuVfcWSF9lioNrzb7R6FDOp4n15ROIcEuGMS6ZY0+hjZpG5yL73J58W
6YkIvAmZw2kYN1GwjM8xM9RLfxSITonWPCYxsgAhJO9nqqInYV2X31GtrwzYm8fX
v1IrBBb36eEQ5eDanUl0BEECgYEAjMlgWPnM+locPGMJV8svAEgw60ttYUe/fhqX
hA1WiGNIJYf6ya50dSUjOD1wyU0iMd8qrCwitJLHQenYjFqoUdUc5spsjx7M4PyG
UPZ1EwfqAyNeWGV6FpjZ3H0s8VFys5OjUcqrSsfjrHS2AwXBSb+GmQgeP4nVnaMg
BleqIQkCgYBMgoAWoEiF5FGF9bSULBzOiGYWUHlv0/RRDpiwPtt+Is5siZl2DIr3
oDJsjewRO34PkPU/qy4Ckodm6QW9Vr5MMjfI80aFmj6scyjVxJXQXeLn8LANWnyq
eb7EIIraVYI2WpoQhosAlZWzQcrMG4LkwuNYam+UVJS0icK4NNgg/Q==
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