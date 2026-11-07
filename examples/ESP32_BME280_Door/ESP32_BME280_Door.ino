/**
 * ESP32_BME280_Door.ino
 * ตัวอย่างโค้ดมาตรฐานสำหรับบอร์ด ESP32 เชื่อมต่อกับแพลตฟอร์ม IoT Scidi (CIS-IoT)
 *
 * คุณสมบัติที่รองรับ:
 * 1. ส่งค่าอุณหภูมิ ความชื้น ความกดอากาศ (BME280 หรือค่าจำลอง) ไปยัง POST /api/v1/telemetry
 * 2. ดึงคำสั่งเปิด-ปิดประตู/รีเลย์ (DOOR_CMD, V0) ทุกๆ DOOR_POLL_INTERVAL_MS (2 วินาที)
 * 3. ข้ามการตรวจ SSL Certificate อัตโนมัติ (client.setInsecure())
 * 4. สลับใช้ IP ภายในของ ม.ทักษิณ อัตโนมัติหาก DNS มีปัญหา (setFallbackBaseUrl)
 * 5. เปิดระบบ Watchdog Timer (esp_task_wdt.h) และ Auto-Reboot เมื่อขาดการติดต่อต่อเนื่อง
 */

#include <CISIoT.h>

// ================= ตั้งค่า WiFi และ DEVICE_TOKEN =================
const char* WIFI_SSID     = "YOUR_WIFI_SSID";
const char* WIFI_PASSWORD = "YOUR_WIFI_PASSWORD";
const char* DEVICE_TOKEN  = "YOUR_DEVICE_TOKEN_FROM_DASHBOARD";

// URL หลัก และ URL สำรอง (IP ภายใน ม.ทักษิณ เพื่อป้องกันปัญหา DNS Timeout)
const char* API_BASE_URL          = "https://dic.scidi.tsu.ac.th/cisiot";
const char* INTERNAL_FALLBACK_URL = "http://10.10.10.10/cisiot";

// ตั้งเวลาส่งค่า Sensor และ Poll คำสั่งประตู/รีเลย์
const unsigned long TELEMETRY_INTERVAL_MS = 10000; // ส่งค่าสภาพแวดล้อมทุก 10 วินาที
const unsigned long DOOR_POLL_INTERVAL_MS = 2000;  // เช็คคำสั่งประตูทุก 2 วินาที

unsigned long lastTelemetryMs = 0;
unsigned long lastPollMs      = 0;

const int DOOR_RELAY_PIN = 2; // ขา GPIO สำหรับควบคุมประตู / รีเลย์

void onServerCommand(const String& pin, const String& value) {
    Serial.printf("[Command] Pin: %s => Value: %s\n", pin.c_str(), value.c_str());

    if (pin == "DOOR_CMD" || pin == "door_action" || pin == "V0") {
        if (value == "OPEN" || value == "1" || value == "ON") {
            digitalWrite(DOOR_RELAY_PIN, HIGH);
            CISIoT.reportDoorStatus("OPEN");
        } else {
            digitalWrite(DOOR_RELAY_PIN, LOW);
            CISIoT.reportDoorStatus("CLOSE");
        }
    }
}

void setup() {
    Serial.begin(115200);
    pinMode(DOOR_RELAY_PIN, OUTPUT);
    digitalWrite(DOOR_RELAY_PIN, LOW);

    // เปิดใช้งาน Watchdog Timer 30 วินาที
    CISIoT.enableWatchdog(30);

    // ตั้งค่า URL สำรองผ่าน IP ภายใน
    CISIoT.setFallbackBaseUrl(INTERNAL_FALLBACK_URL);

    // ผูก Callback เมื่อได้รับคำสั่งจากหน้าเว็บ
    CISIoT.onCommand(onServerCommand);

    // เริ่มต้นเชื่อมต่อ WiFi และ CIS-IoT Server
    CISIoT.begin(WIFI_SSID, WIFI_PASSWORD, DEVICE_TOKEN, API_BASE_URL);
}

void loop() {
    CISIoT.feedWatchdog();
    unsigned long now = millis();

    // 1. เช็คคำสั่งเปิด/ปิดประตู หรือ Virtual Pin จาก Web Dashboard
    if (now - lastPollMs >= DOOR_POLL_INTERVAL_MS) {
        lastPollMs = now;
        CISIoT.pollCommands();
    }

    // 2. ส่งค่าเซ็นเซอร์สภาพแวดล้อม (Temperature, Humidity, Pressure)
    if (now - lastTelemetryMs >= TELEMETRY_INTERVAL_MS) {
        lastTelemetryMs = now;

        float temp = 28.5 + (random(-15, 15) / 10.0);
        float humid = 64.0 + (random(-30, 30) / 10.0);
        float pressure = 1012.5 + (random(-10, 10) / 10.0);

        String extraPins = "{\"V0\":" + String(digitalRead(DOOR_RELAY_PIN)) + "}";
        bool ok = CISIoT.sendEnvironment(temp, humid, pressure, extraPins);
        Serial.printf("[Telemetry] Sent T=%.2f H=%.2f P=%.2f -> %s\n",
                      temp, humid, pressure, ok ? "OK" : "FAILED");
    }
}
