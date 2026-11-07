/**
 * ESP8266_Basic.ino
 * ตัวอย่างการใช้งานบอร์ด ESP8266 (NodeMCU / Wemos D1 Mini) ร่วมกับแพลตฟอร์ม IoT Scidi (CIS-IoT)
 */

#include <CISIoT.h>

const char* WIFI_SSID     = "YOUR_WIFI_SSID";
const char* WIFI_PASSWORD = "YOUR_WIFI_PASSWORD";
const char* DEVICE_TOKEN  = "YOUR_DEVICE_TOKEN_FROM_DASHBOARD";
const char* API_BASE_URL  = "https://dic.scidi.tsu.ac.th/cisiot";

const int RELAY_PIN = D1;
unsigned long lastTelemetryMs = 0;
unsigned long lastPollMs      = 0;

void onServerCommand(const String& pin, const String& value) {
    Serial.printf("[ESP8266 Command] %s = %s\n", pin.c_str(), value.c_str());
    if (pin == "V0") {
        bool turnOn = (value == "1" || value == "ON" || value == "OPEN");
        digitalWrite(RELAY_PIN, turnOn ? HIGH : LOW);
        CISIoT.reportStatus("V0", turnOn ? "1" : "0");
    }
}

void setup() {
    Serial.begin(115200);
    pinMode(RELAY_PIN, OUTPUT);
    digitalWrite(RELAY_PIN, LOW);

    CISIoT.onCommand(onServerCommand);
    CISIoT.begin(WIFI_SSID, WIFI_PASSWORD, DEVICE_TOKEN, API_BASE_URL);
}

void loop() {
    CISIoT.feedWatchdog();
    unsigned long now = millis();

    // Poll คำสั่งจากหน้าเว็บทุก 2 วินาที
    if (now - lastPollMs >= 2000) {
        lastPollMs = now;
        CISIoT.pollCommands();
    }

    // ส่งค่า Analog A0 ไปยัง Virtual Pin V1 ทุก 10 วินาที
    if (now - lastTelemetryMs >= 10000) {
        lastTelemetryMs = now;
        float sensorVal = analogRead(A0);
        CISIoT.virtualWrite("V1", sensorVal);
    }
}
