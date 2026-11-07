# ไลบรารี `CISIoT` สำหรับ Arduino IDE และชุดพัฒนาฮาร์ดแวร์ (IoT Scidi Platform)

ไลบรารีทางการของแพลตฟอร์ม **IoT Scidi (`CIS-IoT`)** มหาวิทยาลัยทักษิณ สำหรับเชื่อมต่อบอร์ดไมโครคอนโทรลเลอร์ **ESP32, ESP8266, Arduino UNO R4 WiFi / UNO Q** และ **Raspberry Pi 5** เข้ากับระบบ Web Dashboard (`https://dic.scidi.tsu.ac.th/cisiot`)

ไลบรารีนี้ออกแบบตามมาตรฐาน **Arduino Library Specification v1.5 (rev.2)** ทุกประการ เพื่อให้สามารถนำขึ้น **GitHub Repository** และส่งเข้าสู่ **Arduino Library Registry** ให้ผู้ใช้งานพิมพ์ค้นหาคำว่า **`CISIoT`** ในช่อง Search ของ **Arduino IDE Library Manager** แล้วกด **Install** พร้อมเปิดโค้ดตัวอย่างจากเมนู **File > Examples > CISIoT** ได้ทันที

---

## 1. วิธีการติดตั้งไลบรารีในโปรแกรม Arduino IDE

ผู้ใช้งานสามารถติดตั้งไลบรารี `CISIoT` ได้ **3 วิธี** ดังนี้:

### วิธีที่ 1: ค้นหาผ่านช่อง Search ของ Library Manager (เมื่อนำเข้า GitHub & Arduino Registry แล้ว)
1. เปิดโปรแกรม **Arduino IDE 2.x**
2. คลิกไอคอน **Library Manager** (รูปหนังสือด้านซ้าย) หรือไปที่เมนู **Sketch > Include Library > Manage Libraries...**
3. พิมพ์ค้นหาในช่อง Search ว่า **`CISIoT`**
4. กดปุ่ม **Install**
5. หลังติดตั้งเสร็จ สามารถเปิดโค้ดตัวอย่างได้ทันทีที่เมนู **File > Examples > CISIoT**

### วิธีที่ 2: ติดตั้งด้วยไฟล์ `.ZIP` จากหน้าเว็บ IoT Scidi โดยตรง
1. ดาวน์โหลดไฟล์ `CISIoT-Library.zip` จากเมนูด้านซ้ายของหน้าเว็บ (`GET /downloads/CISIoT-Library.zip`)
2. ในโปรแกรม Arduino IDE ไปที่เมนู **Sketch > Include Library > Add .ZIP Library...**
3. เลือกไฟล์ `CISIoT-Library.zip` ที่ดาวน์โหลดมา

### วิธีที่ 3: ติดตั้งบอร์ดพร้อมไลบรารีผ่าน Additional Boards Manager URLs
1. เปิดโปรแกรม **Arduino IDE** ไปที่ **File > Preferences**
2. ในช่อง **Additional Boards Manager URLs** ให้วาง URL:
   ```text
   https://dic.scidi.tsu.ac.th/cisiot/package_cisiot_index.json
   ```
3. ไปที่ **Tools > Board > Boards Manager...** ค้นหาคำว่า **`CIS-IoT`** แล้วกด **Install**

---

## 2. ตัวอย่างการเขียนโค้ดใช้งานไลบรารี (`#include <CISIoT.h>`)

```cpp
#include <CISIoT.h>

const char* WIFI_SSID     = "YOUR_WIFI_SSID";
const char* WIFI_PASSWORD = "YOUR_WIFI_PASSWORD";
const char* DEVICE_TOKEN  = "YOUR_DEVICE_TOKEN";
const char* API_BASE_URL  = "https://dic.scidi.tsu.ac.th/cisiot";

void onCommandReceived(const String& pin, const String& value) {
    Serial.printf("ได้รับคำสั่งที่ช่อง %s = %s\n", pin.c_str(), value.c_str());
    if (pin == "V0") {
        bool isOn = (value == "1" || value == "ON");
        digitalWrite(2, isOn ? HIGH : LOW);
        CISIoT.reportStatus("V0", isOn ? "1" : "0");
    }
}

void setup() {
    Serial.begin(115200);
    pinMode(2, OUTPUT);

    CISIoT.enableWatchdog(30); // เปิด Watchdog 30 วินาที
    CISIoT.onCommand(onCommandReceived);
    CISIoT.begin(WIFI_SSID, WIFI_PASSWORD, DEVICE_TOKEN, API_BASE_URL);
}

void loop() {
    CISIoT.feedWatchdog();
    CISIoT.pollCommands();              // ดึงคำสั่งจากหน้าเว็บ
    CISIoT.virtualWrite("V1", 28.5f);   // ส่งค่าไปยัง Virtual Pin V1
    delay(2000);
}
```

---

## 3. คำสั่งหลักของคลาส `CISIoT`

| เมธอด / ฟังก์ชัน | คำอธิบาย |
| :--- | :--- |
| `CISIoT.begin(ssid, pass, token, baseUrl)` | เชื่อมต่อ WiFi และกำหนด `DEVICE_TOKEN` พร้อม URL ของเซิร์ฟเวอร์ |
| `CISIoT.setFallbackBaseUrl(ipUrl)` | กำหนด URL สำรอง (เช่น IP ภายใน ม.ทักษิณ) เมื่อ DNS มีปัญหา |
| `CISIoT.enableWatchdog(seconds)` | เปิดใช้งาน Hardware Watchdog Timer ป้องกันบอร์ดค้าง |
| `CISIoT.feedWatchdog()` | รีเซ็ตเวลา Watchdog Timer ใน `loop()` |
| `CISIoT.onCommand(callback)` | ผูกฟังก์ชันรับค่าเมื่อผู้ใช้กดปุ่ม Switch หรือส่งค่าจาก Input Field บนเว็บ |
| `CISIoT.virtualWrite(pin, value)` | ส่งค่าตัวเลขหรือข้อความขึ้นไปยังช่อง Virtual Pin (บันทึกทั้งกราฟและตาราง `LogStream`) |
| `CISIoT.sendEnvironment(temp, humid, press)` | ส่งค่าอุณหภูมิ ความชื้น และความกดอากาศพร้อมกัน |
| `CISIoT.pollCommands()` | ดึงคำสั่งใหม่ที่รอประมวลผล (`is_command_pending = true`) จากเซิร์ฟเวอร์ |
| `CISIoT.reportStatus(pin, value)` | อัปเดตสถานะจริงของอุปกรณ์กลับไปยังหน้าเว็บ Dashboard |
| `CISIoT.reportDoorStatus(state)` | อัปเดตสถานะประตู (`"OPEN"` / `"CLOSE"`) กลับไปยังหน้าเว็บ |
