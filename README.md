# IoT ProyeK
<img width="814" height="263" alt="data flow iot" src="https://github.com/user-attachments/assets/1e90e8f5-fd8f-4720-8ed7-e9d823183dfb" />
1. Diagram Pertama: OVERALL SYSTEM ARCHITECTURE (IoT DATA FLOW)
Diagram ini menunjukkan gambaran besar atau high-level architecture tentang bagaimana data mengalir dari perangkat keras fisik hingga menjadi informasi yang bisa divisualisasikan.

Akuisisi Data: Perangkat keras (ESP 8266) membaca data sensor dan mengirimkannya menggunakan protokol MQTT ke broker cloud, yaitu AWS IoT Core. Terdapat juga AWS Lambda yang berfungsi sebagai manajemen failover (penanganan otomatis jika terjadi kegagalan sistem).
Percabangan Aliran Data (Penting untuk optimalisasi): Setelah masuk ke IoT Core, data dipecah menjadi dua jalur utama:

Jalur Operasional (Real-time): Data dikirim ke FastAPI (sebagai backend), lalu disimpan di pangkalan data relasional Amazon RDS, dan akhirnya ditampilkan secara real-time ke Web Dashboard.

Jalur Analitik / Big Data: Data mentah juga dikirimkan ke Amazon S3 yang bertindak sebagai Data Lake. Data dalam jumlah besar ini kemudian diproses menggunakan klaster EMR Hadoop. Setelah diproses, data di-kueri menggunakan Amazon Athena dan divisualisasikan untuk keperluan analitik mendalam menggunakan Grafana.
<img width="1091" height="351" alt="aws cloud" src="https://github.com/user-attachments/assets/8af86e30-61c7-4720-9459-a557c32f0c5b" />
2. Diagram Kedua: Arsitektur Jaringan Cloud (AWS VPC)
Diagram ini berfokus pada topologi jaringan dan keamanan (sekuriti) infrastruktur di dalam AWS Cloud. Sistem ini ditempatkan dalam satu Isolated VPC (Virtual Private Cloud) untuk mengontrol akses jaringan.

Public Subnet: Terhubung langsung ke Internet Gateway, sehingga bisa diakses dari luar. Di sini terdapat instance EC2 (m5.xlarge) yang menjalankan layanan aplikasi dan Nginx Proxy. Ini adalah garda depan dari sistem.

Private Subnet: Area terisolasi yang tidak memiliki akses langsung ke internet publik. Pangkalan data RDS MySQL diletakkan di sini demi keamanan.

Konektivitas Internal: Komunikasi antara EC2 (di public subnet) dan RDS (di private subnet) dilakukan secara aman melalui VPC Endpoint.

Akses ke Data Lake: Instance EC2 juga terhubung ke ekosistem big data (Amazon S3 dan EMR Hadoop) menggunakan VPC Endpoint, tanpa perlu melewati NAT Gateway. Ini membuat transfer data antar layanan AWS menjadi lebih cepat, murah, dan tertutup dari internet publik.
<img width="1297" height="208" alt="data" src="https://github.com/user-attachments/assets/07db2731-9958-4761-9218-25ac1d828b43" />
3. Diagram Ketiga: Arsitektur Web Server & API (Nginx dan FastAPI)
Diagram terakhir ini memperbesar (zoom-in) bagian instance EC2 dari diagram kedua untuk menunjukkan bagaimana request dari pengguna ditangani oleh server.

Nginx sebagai Reverse Proxy: Saat Client Device (laptop/PC pengguna) mengirim request, Nginx menjadi pintu gerbang pertama. Nginx bertugas sangat efisien dengan langsung melayani file statis (seperti HTML, CSS, dan JavaScript) kembali ke klien tanpa membebani aplikasi backend.

Komunikasi dengan FastAPI: Jika klien meminta data dinamis (API), Nginx akan meneruskan (forward) trafik tersebut ke FastAPI (berjalan di port 8000).

Dua Jalur Komunikasi Backend:
Path A (REST API): Digunakan untuk operasi relai atau request/response standar (seperti mengambil riwayat data).
Path B (Web Socket): Membangun koneksi duplex agar server bisa mengirim pembaruan data secara real-time langsung ke klien (sangat penting untuk dashboard pemantauan IoT).

Database: FastAPI kemudian berinteraksi dengan RDS MySQL untuk membaca atau menulis data operasional tersebut.
Secara keseluruhan, arsitektur ini sangat kokoh karena memisahkan antara beban kerja transaksional/operasional (FastAPI + RDS) dengan beban kerja analitik big data (EMR Hadoop + S3), serta mengimplementasikan best practice keamanan jaringan menggunakan subnetting yang tepat.
<img width="982" height="560" alt="flowchart iot" src="https://github.com/user-attachments/assets/685ac561-e2bb-4970-8c5d-b88f5205be3e" />
4. Diagram Keempat: Flowchart IoT
Jika 3 diagram sebelumnya fokus pada arsitektur jaringan Cloud dan Backend di AWS, diagram ini berfokus pada Perception Layer (Perangkat Edge/Fisik). Ini adalah bagian "ujung tombak" dari ekosistem IoT yang berinteraksi langsung dengan lingkungan fisik sebelum datanya dikirim ke database.

A. Perangkat Input (Sensor & Akuisisi Data)
ESP32-CAM (Detection): Bertindak sebagai sensor visual. Tugasnya adalah mendeteksi dan menghitung jumlah siswa (orang) di suatu area. Output yang dihasilkan berupa data numerik jumlah orang (Output Student Threshold).
PZEM-004T (Measurement): Ini adalah sensor daya. Tugasnya membaca konsumsi listrik secara fisik, menghasilkan metrik seperti Tegangan (Volt), Daya (Watt), dan Arus (Ampere).

B. Pusat Pemrosesan Lokal (Edge Computing)
NodeMCU ESP8266 (Data Processing): Berperan sebagai "otak" utama di lokasi (microcontroller/gateway). Ia bertugas mengumpulkan data dari ESP32-CAM dan sensor PZEM.
Logika Keputusan (Threshold > 5): Ini adalah contoh penerapan edge computing. NodeMCU tidak perlu melempar data ke cloud hanya untuk memutuskan apakah saklar harus menyala. Ia mengevaluasi aturan secara lokal:
Jika jumlah siswa kurang dari 5 (No), sistem looping kembali melakukan deteksi melalui kamera.
Jika jumlah siswa 5 atau lebih (Yes), ia langsung mengirimkan perintah ke aktuator.

C. Perangkat Output (Aktuator)
Relay 5V 4 Channel On/Off: Berfungsi sebagai saklar mekanis otomatis. Saat menerima sinyal "Yes" dari NodeMCU, relay akan aktif dan mengalirkan listrik ke beban (misalnya menyalakan lampu, AC, atau perangkat lab di ruangan tersebut).

D. Komunikasi & Integrasi Dashboard
Payload JSON : Setelah listrik mengalir dan sensor PZEM membaca konsumsi dayanya, NodeMCU mengemas data Volt, Watt, Ampere tersebut ke dalam format JSON yang ringan, lalu mengirimkannya ke Database & Monitoring Dashboard untuk disimpan dan divisualisasikan.

E. Two-way Communication & Remote Control : Sistem ini tidak hanya berjalan satu arah. Terdapat Web Server Manual Input, yang memungkinkan operator melakukan Remote Control. Artinya, administrator bisa mengesampingkan sistem otomatis (misalnya, mematikan listrik secara manual untuk maintenance dari dashboard meskipun di ruangan tersebut ada diatas 5 siswa).

**ROLE MEMBER**

Abizhar  :

Alika    :

Arfa     :

Musa     :
