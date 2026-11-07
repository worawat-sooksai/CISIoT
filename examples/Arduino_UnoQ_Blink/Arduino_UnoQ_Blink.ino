/**
 * Arduino_UnoQ_Blink.ino
 * ตัวอย่างการใช้งานบอร์ด Arduino UNO R4 WiFi / UNO Q ร่วมกับแพลตฟอร์ม IoT Scidi (CIS-IoT)
 */

#include <CISIoT.h>

const char* WIFI_SSID     = "YOUR_WIFI_SSID";
const char* WIFI_PASSWORD = "YOUR_WIFI_PASSWORD";
const char* DEVICE_TOKEN  = "YOUR_DEVICE_TOKEN_FROM_DASHBOARD";
const char* API_BASE_URL  = "https://dic.scidi.tsu.ac.th/cisiot";

unsigned long lastPollMs = 0;
unsigned long lastSendMs = 0;

void onServerCommand(const String& pin, const String& value) {
    if (pin == "V0") {
        bool isOn = (value == "1" || value == "ON");
        digitalWrite(LED_BUILTIN, isOn ? HIGH : LOW);
        CISIoT.reportStatus("V0", isOn ? 1 : 0);
    }
}

void setup() {
    Serial.begin(115200);
    pinMode(LED_BUILTIN, OUTPUT);

    CISIoT.onCommand(onServerCommand);
    CISIoT.begin(WIFI_SSID, WIFI_PASSWORD, DEVICE_TOKEN, API_BASE_URL);
}

void loop() {
    unsigned long now = millis();

    if (now - lastPollMs >= 3000) {
        lastPollMs = now;
        CISIoT.pollCommands();
    }

    if (now - lastSendMs >= 10000) {
        lastSendMs = now;
        CISIoT.virtualWrite("V1", 27.5f);
    }
}
