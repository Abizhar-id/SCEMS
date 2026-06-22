# Panduan Instalasi & Penggunaan Proyek IoT Smart Classroom

Repositori ini memuat kode sumber (source code) dan konfigurasi untuk sistem manajemen daya kelas berbasis kehadiran fisik, dengan infrastruktur komputasi awan (AWS) dan pemrosesan edge (IoT).

## 📂 Struktur Repositori

Sebelum memulai, pastikan Anda memahami struktur direktori pada repositori ini:
* `backend/`: Berisi inti server menggunakan FastAPI, file database (`database.py`), klien MQTT (`mqtt_client.py`), dan daftar dependensi (`requirements.txt`).
* `frontend/`: Antarmuka dashboard pengguna berbasis HTML, CSS, dan JS.
* `mosquitto/`: Konfigurasi untuk broker MQTT (`mosquitto.conf`).
* `nginx/`: Konfigurasi reverse proxy web server (`iot-project.conf`).
* `Image_Detection_ESP32v2.ino`: Kode program untuk ESP32-CAM (Deteksi Objek/Kamera).
* `SCEMSv6.ino`: Kode program untuk NodeMCU ESP8266 & Sensor PZEM-004T.

---

## 🛠️ Instruksi Penggunaan & Setup

Ikuti tahapan di bawah ini secara berurutan untuk menjalankan sistem mulai dari perangkat keras hingga infrastruktur *cloud*.

### Tahap 1: Setup Perangkat Keras (IoT Edge) & Mikrokontroler

Tahap pertama adalah menanamkan logika otomatisasi pada perangkat fisik di lapangan.

<img width="1144" height="602" alt="WhatsApp Image 2026-06-08 at 00 18 15" src="https://github.com/user-attachments/assets/8cb35089-11a5-4453-bc11-90b5c2cbbb2e" />

*Gambar 1: Alur Otomatisasi Perangkat Keras (IoT Edge)*

**Langkah-langkah:**
1.  **ESP32-CAM (Kamera):** Buka file `Image_Detection_ESP32v2.ino` menggunakan Arduino IDE. Lakukan *flashing* ke modul ESP32-CAM. Modul ini akan bertugas mendeteksi dan menghitung jumlah siswa di ruangan.
2.  **NodeMCU & Sensor Daya:** Buka file `SCEMSv6.ino` dan lakukan *flashing* ke NodeMCU ESP8266. Mikrokontroler ini akan membaca data dari ESP32-CAM. 
3.  **Logika Relay:** Jika siswa berjumlah ≥ 5 orang, NodeMCU akan memicu modul Relay untuk menyalakan listrik. Data metrik kelistrikan (Volt/Watt/Ampere) kemudian dibaca oleh sensor PZEM-004T dan dikirimkan ke cloud (AWS IoT Core) melalui protokol MQTT.

### Tahap 2: Konfigurasi Keamanan Jaringan Cloud (AWS VPC)

Setelah perangkat keras siap, siapkan fondasi infrastruktur awan yang aman sebelum mengunggah aplikasi.

<img width="1091" height="351" alt="aws cloud" src="https://github.com/user-attachments/assets/8af86e30-61c7-4720-9459-a557c32f0c5b" />
*Gambar 2: Keamanan Jaringan Server (VPC Subnetting)*

**Langkah-langkah di AWS Console:**
1.  Buat **VPC (Virtual Private Cloud)** baru dengan topologi terisolasi.
2.  Siapkan instance **EC2** di dalam **Public Subnet** agar server dapat diakses dari internet.
3.  Siapkan database **RDS (MySQL)** di dalam **Private Subnet** agar data tetap aman dan tidak terekspos ke internet publik.
4.  Konfigurasikan **VPC Endpoint** untuk menghubungkan EC2 dan RDS secara internal tanpa melalui jalur internet.

### Tahap 3: Setup Web Server (Nginx) & Server Aplikasi (FastAPI)

Tahap ini mengatur bagaimana aplikasi melayani permintaan dari antarmuka pengguna (dashboard).

<img width="1297" height="208" alt="data" src="https://github.com/user-attachments/assets/07db2731-9958-4761-9218-25ac1d828b43" />
*Gambar 3: Manajemen Lalu Lintas Aplikasi (Nginx & FastAPI)*

**Langkah-langkah Konfigurasi di EC2:**
1.  **Backend (FastAPI):** * Masuk ke direktori `backend/`.
    * Instal dependensi Python: `pip install -r requirements.txt`.
    * Jalankan server menggunakan Uvicorn di port 8000. API ini akan melayani REST request (Path A) dan WebSocket (Path B) untuk *real-time updates*.
2.  **Konfigurasi Mosquitto:** Pindahkan file konfigurasi dari `mosquitto/mosquitto.conf` ke direktori sistem broker Anda untuk mengatur penerimaan MQTT dari perangkat IoT.
3.  **Nginx (Reverse Proxy):**
    * Salin file konfigurasi dari direktori `nginx/iot-project.conf` ke `/etc/nginx/sites-available/`.
    * Aktifkan konfigurasi Nginx. Nginx akan langsung melayani *static files* (HTML/CSS/JS) yang berada di direktori `frontend/`, dan meneruskan *request* dinamis ke FastAPI.

### Tahap 4: Analitik Big Data & Skalabilitas

Langkah terakhir ini memisahkan jalur data operasional (real-time) dengan data analitik jangka panjang.

<img width="814" height="263" alt="data flow iot" src="https://github.com/user-attachments/assets/1e90e8f5-fd8f-4720-8ed7-e9d823183dfb" />
*Gambar 4: Arsitektur Aliran Data di Cloud (Pemisahan Beban Kerja)*

**Cara Kerja Alur Data:**
* **Jalur Operasional (Real-time):** Data yang masuk melalui broker disimpan di database relasional (RDS MySQL) oleh FastAPI dan langsung ditampilkan di web klien via koneksi WebSocket.
* **Jalur Analitik (Data Lake):** Data mentah sensor diteruskan menuju Amazon S3 secara massal. Lakukan konfigurasi pada **EMR Hadoop** dan **Amazon Athena** untuk memproses data berukuran besar (Big Data) yang tersimpan di S3, lalu hubungkan hasilnya ke platform visualisasi analitik seperti **Grafana** untuk melihat tren pemakaian listrik jangka panjang.
**ROLE MEMBER**

1. Abizhar-id zurrr — Cloud Infrastructure & Backend Developer

Abizhar-id zurrr bertanggung jawab penuh atas fondasi komputasi awan, manajemen basis data, penyediaan jaringan aman, serta pengembangan logika bisnis di sisi server. Perannya menjembatani transmisi data mentah dari perangkat fisik hingga menjadi informasi siap saji bagi antarmuka pengguna.

Arsitektur Jaringan & Isolasi Cloud : Merancang dan mengonfigurasi topologi jaringan pada AWS Cloud dengan mengimplementasikan Isolated VPC. Mengatur pembagian Public Subnet untuk meletakkan instance EC2 m5.xlarge agar dapat berinteraksi dengan luar, serta mengisolasi Private Subnet khusus untuk pangkalan data RDS MySQL demi menjaga keamanan data sensitif. Abizar juga mengonfigurasi VPC Endpoint sebagai jalur internal aman, mengeliminasi kebutuhan akan NAT Gateway untuk interkoneksi ke Data Lake.

Manajemen Lalu Lintas & Web Server : Membangun dan mengonfigurasi Nginx sebagai Reverse Proxy di garda terdepan server EC2 untuk mengelola request traffic dari klien. Di balik Nginx, Abizar mengembangkan core backend menggunakan FastAPI yang berjalan pada port 8000. Ia merancang Path A (REST API) untuk menangani operasi relai data dan request/response konvensional, serta mengimplementasikan Path B (WebSocket) untuk mendukung koneksi duplex bertukar data secara real-time.

Pemrosesan Big Data & Analitik : Mengonfigurasi ekosistem data lake hulu-ke-hilir di sisi cloud. Ini mencakup manajemen penyimpanan terdistribusi, koordinasi klaster pemrosesan EMR Hadoop untuk kalkulasi data analitik skala besar, penyusunan skema kueri interaktif menggunakan Amazon Athena, hingga integrasi data ke Grafana untuk visualisasi analitik jangka panjang.

2. DanishArfa98 — Frontend Developer & Cloud Assistant 

DanishArfa98 memegang kendali atas representasi visual sistem di sisi pengguna (client-side) sekaligus mendampingi penguatan infrastruktur cloud, khususnya pada bagian penyerapan data (data ingestion) dan manajemen kegagalan (failover).

Pengembangan Antarmuka Dashboard : Merancang dan membangun Web Dashboard / Client Device sebagai pusat monitoring interaktif bagi pengguna. Danish mengimplementasikan integrasi kode (HTML, CSS, JS) yang dioptimalkan agar dapat disajikan secara instan oleh Nginx. Ia juga menyusun logika penangkapan data dinamis melalui REST request/response serta mempertahankan performa halaman tanpa reload memanfaatkan pipa komunikasi WebSocket untuk pembaruan data real-time.

Konfigurasi Ingesti Data Cloud : Membantu penyusunan infrastruktur cloud awal yang berinteraksi langsung dengan perangkat IoT. Danish berkontribusi dalam mengonfigurasi AWS IoT Core sebagai broker protokol MQTT untuk menerima data sensor. Ia juga bertanggung jawab membangun fungsi komputasi serverless menggunakan AWS Lambda yang didekasikan khusus sebagai Lambda: Failover management untuk menjamin ketersediaan tinggi sistem apabila jalur utama mengalami kendali teknis. Selain itu, Danish ikut mengelola penyimpanan Amazon S3 sebagai wadah pendaratan pertama (landing zone) bagi data mentah IoT.

3. MAdiW — Embedded Systems (IoT) Engineer & Machine Learning Specialist

MAdiW bertanggung jawab penuh di ranah fisik lapangan (hardware layer). Tugasnya mencakup rekayasa perangkat tertanam (embedded system), interkoneksi sensor-aktuator di tingkat edge, serta penyematan kecerdasan buatan langsung pada perangkat keras.

Kecerdasan Buatan & Pengenalan Objek : Mengembangkan dan menerapkan model Machine Learning (ML) berbasis Computer Vision yang ditanamkan langsung pada modul ESP32-CAM. Model ini bertindak sebagai komponen persepsi utama yang secara cerdas mendeteksi keberadaan dan menghitung jumlah murid secara otomatis di dalam ruangan.

Komputasi Edge & Logika Otomatisasi : Memprogram mikrokontroler NodeMCU ESP8266 sebagai pusat pemrosesan data lokal (Edge Computing). Musa menyusun algoritma pengkondisian threshold (≥5 murid). Jika kondisi terpenuhi, NodeMCU secara otomatis mengirim sinyal pemicu ke aktuator berupa Relay 5V 4 Channel (On/Off) untuk menyalakan atau mematikan aliran listrik ruangan secara mekanis.

Telemetri Daya & Komunikasi Protokol : Mengintegrasikan modul PZEM-004T untuk mengukur metrik kelistrikan fisik (Volt, Watt, Ampere) secara presisi setelah relay aktif. Musa bertugas mengemas seluruh data pengukuran tersebut menjadi struktur Payload JSON, lalu membangun pipa komunikasi nirkabel berbasis protokol MQTT (Sensor Data) dari NodeMCU untuk ditembakkan secara berkala menuju AWS IoT Core.

4. Rey_Adellforte — Technical Documentation & Cross-Course Design Specialist

Rey_Adellforte memegang peran krusial dalam mentransformasikan seluruh arsitektur teknis yang kompleks menjadi dokumentasi formal, laporan akademis, dan media publikasi visual yang terstruktur rapi. Perannya memastikan proyek ini memenuhi standar capaian dari berbagai mata kuliah yang saling beririsan.

Penyusunan Modul Laporan & Poster Diseminasi : Rey_Adellforte merelasikan implementasi sistem ke dalam tiga pilar akademik utama melalui dokumentasi teknis menyeluruh dan pembuatan poster visual proyek:

Perancangan Jaringan: Mendokumentasikan konfigurasi topologi jaringan, menjelaskan secara rinci fungsionalitas pembagian public/private subnet, manajemen gerbang internet, serta efisiensi penggunaan VPC Endpoints dalam mereduksi celah keamanan jaringan cloud.

Teknologi Server: Menyusun materi analitis dari penggunaan materi teknologi server di proyek, membedah cara kerja arsitektur asynchronous FastAPI, efisiensi Nginx dalam menangani aset statis, perbedaan mendasar manajemen data transaksional di Amazon RDS vs pengolahan data massal di EMR Hadoop, serta mekanisme failover menggunakan AWS Lambda.

Praktikum IoT: Menguraikan alur kerja perangkat keras dari IoT, mendokumentasikan skema pengabelan fisik sensor PZEM-004T, modul kamera, relay, logika interkoneksi edge, hingga struktur pembentukan berkas JSON yang dikirim via broker MQTT.
