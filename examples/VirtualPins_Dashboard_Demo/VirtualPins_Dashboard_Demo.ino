/**
 * VirtualPins_Dashboard_Demo.ino
 * ตัวอย่างการรับ-ส่งข้อมูลครบทุกประเภท Widget บนกระดานข้อมูล IoT Scidi:
 * - V0 : Switch / Button (เปิด-ปิดไฟ/รีเลย์)
 * - V1 : Input Field (รับค่าข้อความหรือตัวเลขที่ผู้ใช้พิมพ์ส่งมาจากหน้าเว็บ)
 * - V2 : Gauge (ส่งค่าตัวเลขทศนิยมไปแสดงผลเกจวัดและกราฟเส้น)
 * - V3 : Text / LogStream (ส่งข้อความสถานะไปแสดงในตาราง LogStream เรียงเวลาล่าสุด DESC)
 */

#include <CISIoT.h>

const char* WIFI_SSID     = "YOUR_WIFI_SSID";
const char* WIFI_PASSWORD = "YOUR_WIFI_PASSWORD";
const char* DEVICE_TOKEN  = "YOUR_DEVICE_TOKEN_FROM_DASHBOARD";
const char* API_BASE_URL  = "https://dic.scidi.tsu.ac.th/cisiot";

unsigned long lastSendMs = 0;
unsigned long lastPollMs = 0;

void onDashboardCommand(const String& pin, const String& value) {
    Serial.printf("[Dashboard -> Device] Pin %s = %s\n", pin.c_str(), value.c_str());

    if (pin == "V0") {
        // รับคำสั่งจากปุ่ม Switch (ON / OFF)
        bool isOn = (value == "1" || value == "ON");
        CISIoT.reportStatus("V0", isOn ? "1" : "0");
    } else if (pin == "V1") {
        // รับข้อความหรือค่าตั้งต้นจากกล่อง Input Field บนหน้าเว็บ
        Serial.println("ได้รับข้อความจากช่อง Input Field (V1): " + value);
        CISIoT.reportStatus("V1", value);
        CISIoT.virtualWrite("V3", "Ack Input: " + value);
    }
}

void setup() {
    Serial.begin(115200);
    CISIoT.enableWatchdog(30);
    CISIoT.onCommand(onDashboardCommand);
    CISIoT.begin(WIFI_SSID, WIFI_PASSWORD, DEVICE_TOKEN, API_BASE_URL);
}

void loop() {
    CISIoT.feedWatchdog();
    unsigned long now = millis();

    if (now - lastPollMs >= 2000) {
        lastPollMs = now;
        CISIoT.pollCommands();
    }

    if (now - lastSendMs >= 10000) {
        lastSendMs = now;
        float gaugeVal = 55.0 + (random(-50, 50) / 10.0);
        CISIoT.virtualWrite("V2", gaugeVal);
        CISIoT.virtualWrite("V3", "System Normal @ " + String(millis() / 1000) + "s");
    }
}
