# IoT ProyeK
<img width="814" height="263" alt="data flow iot" src="https://github.com/user-attachments/assets/1e90e8f5-fd8f-4720-8ed7-e9d823183dfb" />
1. Diagram 1: Arsitektur Aliran Data di Cloud (Pemisahan Beban Kerja)

Diagram ini menjelaskan perjalanan data setelah dikirim oleh perangkat IoT ke sistem awan (AWS). Kunci utamanya adalah pemisahan dua jalur data:

Jalur Cepat (Operasional): Data dialirkan ke server aplikasi (FastAPI) dan disimpan di database relasional (RDS). Tujuannya agar data bisa langsung ditampilkan secara real-time di Dashboard pengguna.

Jalur Lambat/Analitik (Big Data): Data mentah juga dikumpulkan secara massal ke penyimpanan besar (S3), lalu diproses oleh sistem analitik (EMR Hadoop & Athena). Tujuannya untuk melihat tren, riwayat jangka panjang, dan analisis mendalam menggunakan grafik Grafana.

<img width="1091" height="351" alt="aws cloud" src="https://github.com/user-attachments/assets/8af86e30-61c7-4720-9459-a557c32f0c5b" />
2. Diagram 2: Keamanan Jaringan Server (VPC Subnetting)

Diagram ini fokus pada bagaimana AWS mengisolasi dan mengamankan infrastruktur dari ancaman luar internet.

Pemisahan Akses: Server utama (EC2) diletakkan di Public Subnet karena harus bisa diakses oleh pengguna dari internet. Namun, Database (RDS) menyimpan data sensitif, sehingga disembunyikan di Private Subnet yang tidak memiliki akses internet sama sekali.

Koneksi Internal: Agar Server (EC2) dan Database (RDS) bisa saling berkomunikasi, mereka menggunakan VPC Endpoint. Ini adalah terowongan internal AWS yang membuat transfer data menjadi sangat aman tanpa harus keluar ke jalur internet publik.

<img width="1297" height="208" alt="data" src="https://github.com/user-attachments/assets/07db2731-9958-4761-9218-25ac1d828b43" />
3. Diagram 3: Manajemen Lalu Lintas Aplikasi (Nginx & FastAPI)

Diagram ini menyoroti apa yang terjadi di dalam server EC2 saat pengguna membuka aplikasi dashboard di perangkat mereka.

Nginx sebagai Penerima Tamu: Nginx melayani permintaan sederhana (seperti gambar, desain halaman, atau teks statis) secara langsung ke pengguna tanpa membebani aplikasi utama.

FastAPI sebagai Mesin Utama: Untuk permintaan data dinamis (misalnya histori sensor), Nginx meneruskannya ke FastAPI. FastAPI kemudian menarik data dari Database (RDS). FastAPI juga membuka jalur Web Socket agar dashboard pengguna bisa terus diperbarui secara real-time tiap detiknya tanpa perlu menekan tombol refresh.
<img width="982" height="560" alt="flowchart iot" src="https://github.com/user-attachments/assets/685ac561-e2bb-4970-8c5d-b88f5205be3e" />
4. Diagram 4: Alur Otomatisasi Perangkat Keras (IoT Edge)

Diagram ini bergeser dari ranah cloud ke ranah fisik (perangkat keras di lapangan). Ini adalah sistem manajemen daya berbasis kehadiran fisik.

Logika Otomatisasi: Kamera (ESP32-CAM) bertugas menghitung jumlah siswa di sebuah ruangan. Mikrokontroler (NodeMCU) kemudian memproses data tersebut: jika jumlah siswa 5 orang atau lebih, ia akan memerintahkan Relay untuk menyalakan aliran listrik ruangan.

Siklus Pemantauan: Setelah listrik menyala, sensor daya (PZEM-004T) akan membaca berapa banyak listrik (Volt/Watt/Ampere) yang sedang digunakan. Data pemakaian listrik ini kemudian dikirim kembali ke server web dan dashboard agar bisa dipantau secara langsung oleh administrator. Administrator juga diberi hak akses kendali manual dari web server jika diperlukan.

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
