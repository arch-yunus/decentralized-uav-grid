<p align="center">
  <img src="assets/banner.png" alt="Decentralized UAV Grid Banner" width="100%">
</p>

# 🛸 Decentralized UAV Grid (DUG)

![Version](https://img.shields.io/badge/version-1.0.0--alpha-blue.svg)
![ROS2](https://img.shields.io/badge/ROS2-Humble-brightgreen.svg)
![C++](https://img.shields.io/badge/C++-14%2F17-blue.svg)
![License](https://img.shields.io/badge/license-MIT-green.svg)
![Build](https://img.shields.io/badge/Status-Active_Development-orange.svg)

**Decentralized UAV Grid (DUG)**, yüzlerce veya binlerce insansız hava aracının (İHA) herhangi bir merkezi sunucuya, karargaha veya baz istasyonuna ihtiyaç duymadan, kendi aralarında P2P (Peer-to-Peer) ve Mesh (Ağ) topolojileri üzerinden haberleşerek otonom şekilde görev yapmasını sağlayan bir sürü zekası (swarm intelligence) altyapısıdır.

---

## 🇹🇷 Türkiye'nin "Topyekûn Savunma" Vizyonuna Katkımız

DUG, Türkiye'nin **81 İlde Drone Seferberliği** ve yeni **Topyekûn Savunma** konseptine sivil Ar-Ge ekosisteminden sunulan stratejik bir teknolojidir. 

### Temel Stratejik Sütunlar
*   **Asimetrik Güç Çarpanı:** Hantal ve pahalı tekil platformlar yerine, ucuz ve koordinatlı binlerce platformun oluşturduğu "sürü" etkisi.
*   **Dağıtık Üretim:** Farklı atölyelerde üretilen İHA'ların tek bir ağda "tak-çalıştır" (plug-and-play) mantığıyla entegre olması.
*   **Elektronik Harp Dayanımı:** Tek bir sinyal kulesine bağımlı olmayan, kendi iç haberleşmesini dinamik olarak rotalayan mesh yapısı sayesinde Jammer direnci.

---

## 🏗 Sistem Mimarisi ve Veri Akışı

Mimarimiz, donanım katmanından otonomi katmanına kadar **sıfır-merkeziyet (zero-centrality)** prensibiyle tasarlanmıştır.

### 1. Katmanlı Mimari (UAV Edge-Node)

```mermaid
graph TD
    subgraph "Sensör ve Fiziksel Katman"
        MAV[MAVLink / PX4]
        CAM[OAK-D / Vision Sensor]
    end
    
    subgraph "Otonomi Merkezi (dug_core)"
        LDR[Leader Election Engine]
        FORM[Formation Controller]
        NAV[Navigation & Setpoints]
    end
    
    subgraph "Mesh Haberleşme (dug_communication)"
        BAT[B.A.T.M.A.N. Adv]
        ZT[Zero-Trust Handshake]
    end

    MAV <--> LDR
    CAM --> VIS[dug_vision]
    VIS --> LDR
    LDR <--> BAT
    BAT <--> ZT
    FORM --> NAV
    NAV --> MAV
```

---

## 📦 Paket Ekosistemi

| Paket | Katman | Teknoloji | Detaylı Görev |
| :--- | :--- | :--- | :--- |
| `dug_msgs` | **Interface** | ROS2 IDL | SwarmState, TargetInfo ve FormationState gibi sürü-özel mesaj tipleri. |
| `dug_core` | **Logic** | C++ 17 | Sürü lideri seçimi, formasyon matematiği ve görev dağıtımı. |
| `dug_vision` | **Perception** | Py / AI | YOLO/TensorRT tabanlı hedef tespiti ve 3D koordinat kestirimi. |
| `dug_communication` | **Networking** | Python / Mesh | P2P el sıkışma, ağ topolojisi izleme ve şifreli veri aktarımı. |
| `dug_simulation` | **Testing** | Gazebo | Çoklu PX4 SITL araçlarının ve ROS2 düğümlerinin koordinasyonu. |

---

## 🧠 Temel Algoritmalar

### A. Dinamik Lider Seçimi (Self-Healing Election)
Sürüde her İHA, bir "Aday" (Candidate) olarak başlar. Liderlik kriterleri şunlardır:
1.  **Pil Sağlığı ($B_i$):** En yüksek enerji rezervine sahip olan önceliklidir.
2.  **Bağlantı Derecesi ($C_i$):** Mesh ağında en fazla komşuya (peer) doğrudan erişimi olan.
3.  **Donanım Yükü ($L_i$):** CPU/GPU kullanım oranı en düşük olan.

**Matematiksel Skor:** $Score_i = w_1 B_i + w_2 C_i - w_3 L_i$
*Eğer mevcut liderin skoru kritik eşiğin altına düşerse, yeni seçim milisaniyeler içinde gerçekleşir.*

### B. Formasyon Matematiği (Relative Offsets)
Takipçiler, liderin $P_{leader}$ konumuna göre kendi yerel ofsetlerini ($O_{uav\_id}$) hesaplar:
$P_{cmd} = P_{leader} + R(\psi) \cdot O_{uav\_id}$
Burada $R(\psi)$, liderin yönelimine (heading) göre dönme matrisidir.

---

## 🛡 Zero-Trust ve Siber Güvenlik

Merkeziyetsiz ağlarda "içeriden saldırı" (Insider Threat) en büyük risktir. DUG bunu şu şekilde engeller:
*   **Handshake:** Her veri paketi, gönderici İHA'nın benzersiz imzasıyla doğrulanır.
*   **Outlier Detection:** Sürü ortalamasından sapan (örneğin aniden saçma koordinatlar bildiren) düğümler, ağ tarafından otomatik olarak "izole" edilir.

---

## 🚀 Donanım ve Saha Kurulumu

### Minimum Gereksinimler
*   **SBC:** Nvidia Jetson Nano / Orin Nano (Görü işleme için önerilir).
*   **Flight Controller:** Pixhawk v5+ veya Cube Orange.
*   **Mesh Adaptör:** 802.11s destekli Wi-Fi kartları (Atheros çipsetli önerilir).

### Yazılım Kurulumu
```bash
# Bağımlılıklar ve ROS2 Humble Kurulumu
chmod +x scripts/setup_env.sh
./scripts/setup_env.sh

# Workspace Derleme
colcon build --symlink-install
source install/setup.bash
```

---

## 🧪 Simülasyon ve Validasyon

Sistemi 10 adet İHA ile Gazebo üzerinde test etmek için:
```bash
ros2 launch dug_simulation tactical_swarm.launch.py drone_count:=10
```

**Gözlemlenecek Parametreler:**
*   `ros2 topic echo /swarm/status`: Sürü üyelerinin sağlık durumu.
*   `ros2 topic echo /swarm/targets`: Senkronize edilmiş küresel hedef listesi.

---

## 🗺 Yol Haritası (Roadmap)

- [x] **Faz 1:** Merkeziyetsiz haberleşme ve lider seçimi.
- [x] **Faz 2:** Dinamik formasyon ve hedef senkronizasyonu.
- [ ] **Faz 3:** Engelden kaçınma (Obstacle Avoidance) ve VFH+ algoritması.
- [ ] **Faz 4:** GNSS-Denied ortamlarda görsel navigasyon (SLAM).
- [ ] **Faz 5:** Kamikaze ve mühimmat bırakma modülleri entegrasyonu.

---

## 🤝 Katkıda Bulunma

Bu proje Türkiye'nin teknolojik bağımsızlığına katkı sunmayı amaçlayan bir Ar-Ge çalışmasıdır. Katkılarınız için `CONTRIBUTING.md` dosyasına göz atın.

## 📄 Lisans

Decentralized UAV Grid (DUG) **MIT Lisansı** ile korunmaktadır.