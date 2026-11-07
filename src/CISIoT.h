/**
 * @file CISIoT.h
 * @brief ไลบรารีมาตรฐานสำหรับเชื่อมต่อบอร์ดไมโครคอนโทรลเลอร์ (ESP32, ESP8266, Arduino UNO R4 WiFi / UNO Q)
 *        เข้ากับแพลตฟอร์ม IoT Scidi (CIS-IoT Platform) มหาวิทยาลัยทักษิณ
 * @version 1.0.0
 * @author CIS-IoT Thaksin University (TSU)
 */

#ifndef CISIOT_H
#define CISIOT_H

#include <Arduino.h>

#if defined(ESP32)
    #include <WiFi.h>
    #include <HTTPClient.h>
    #include <WiFiClientSecure.h>
    #include <esp_task_wdt.h>
    #include "CISIoT_ESP32.h"
#elif defined(ESP8266)
    #include <ESP8266WiFi.h>
    #include <ESP8266HTTPClient.h>
    #include <WiFiClientSecureBearSSL.h>
    #include "CISIoT_ESP8266.h"
#elif defined(ARDUINO_UNOR4_WIFI) || defined(ARDUINO_UNO_Q)
    #if __has_include(<WiFiS3.h>)
        #include <WiFiS3.h>
        #include <WiFiSSLClient.h>
    #endif
    #include "CISIoT_UnoQ.h"
#else
    #include "CISIoT_UnoQ.h"
#endif

/**
 * ชนิดฟังก์ชัน Callback เมื่อบอร์ดได้รับคำสั่งใหม่จากหน้าเว็บ Dashboard
 * @param pin   ชื่อช่องสัญญาณ เช่น "V0", "V1", "DOOR_CMD"
 * @param value ค่าที่ส่งมาจากหน้าเว็บ เช่น "1", "0", "OPEN", "CLOSE", หรือข้อความจาก Input Field
 */
typedef void (*CISIoTCommandCallback)(const String& pin, const String& value);

class CISIoTClient {
private:
    const char* _ssid = nullptr;
    const char* _password = nullptr;
    String _token = "";
    String _baseUrl = "https://dic.scidi.tsu.ac.th/cisiot";
    String _fallbackBaseUrl = "";
    int _failCount = 0;
    int _maxFailsBeforeReboot = 15;
    bool _wdtEnabled = false;
    CISIoTCommandCallback _commandCallback = nullptr;

    String resolveUrl(const String& endpoint, bool useFallback = false) const;
    int executeRequest(const String& method, const String& url, const String& payload, String& responseOut);
    int sendHttpRequest(const String& method, const String& endpoint, const String& payload, String& responseOut);
    void dispatchPendingCommands(const String& jsonResponse);
    static String escapeJsonString(const String& input);

public:
    CISIoTClient();

    /**
     * เริ่มต้นเชื่อมต่อ WiFi และตั้งค่า DEVICE_TOKEN สำหรับติดต่อกับเซิร์ฟเวอร์ IoT Scidi
     */
    void begin(
        const char* ssid,
        const char* password,
        const char* deviceToken,
        const char* baseUrl = "https://dic.scidi.tsu.ac.th/cisiot"
    );

    /**
     * ตั้งค่า URL สำรอง (เช่น IP ภายในมหาวิทยาลัยทักษิณ) เพื่อสลับใช้งานอัตโนมัติเมื่อ DNS ภายนอกล้มเหลว
     */
    void setFallbackBaseUrl(const char* fallbackUrl);

    /**
     * กำหนดจำนวนครั้งสูงสุดที่ส่ง HTTP ล้มเหลวติดต่อกันก่อนสั่งรีบูตบอร์ดอัตโนมัติ (0 = ปิดการรีบูตอัตโนมัติ)
     */
    void setMaxFailsBeforeReboot(int maxFails);

    /**
     * เปิดใช้งาน Hardware Watchdog Timer (รองรับ ESP32 / ESP8266)
     */
    void enableWatchdog(uint32_t timeoutSeconds = 30);

    /**
     * รีเซ็ตสัญญาณ Watchdog Timer ใน loop()
     */
    void feedWatchdog();

    /**
     * ตรวจสอบและเชื่อมต่อ WiFi ใหม่อัตโนมัติหากสัญญาณหลุด
     */
    void reconnectWiFi();

    /**
     * ตรวจสอบสถานะการเชื่อมต่อ WiFi
     */
    bool isConnected() const;

    /**
     * คืนค่าหมายเลข IP Address ปัจจุบันของบอร์ด
     */
    String localIP() const;

    /**
     * กำหนดฟังก์ชัน Callback ที่จะถูกเรียกอัตโนมัติเมื่อมีคำสั่งใหม่จากหน้าเว็บ Dashboard
     */
    void onCommand(CISIoTCommandCallback cb);

    /**
     * ส่งค่าเซ็นเซอร์สภาพแวดล้อมมาตรฐาน (อุณหภูมิ, ความชื้น, ความกดอากาศ) และ Virtual Pins เพิ่มเติม
     * ไปยัง POST /api/v1/telemetry
     */
    bool sendEnvironment(float temp, float humid, float pressure, const String& extraPinsJson = "");

    /**
     * ส่งค่าไปยัง Virtual Pin ที่ระบุ (เช่น "V0", "V1", "TEMP") ไปยัง POST /api/v1/telemetry
     */
    bool virtualWrite(const String& pin, const String& value);
    bool virtualWrite(const String& pin, float value, uint8_t decimals = 2);
    bool virtualWrite(const String& pin, int value);

    /**
     * อัปเดตสถานะการทำงานจริงของอุปกรณ์กลับไปยังหน้าเว็บ Dashboard (POST /api/v1/status)
     */
    bool reportStatus(const String& pin, const String& value);
    bool reportStatus(const String& pin, int value);
    bool reportDoorStatus(const String& doorState);

    /**
     * ดึงคำสั่งที่ค้างอยู่จากหน้าเว็บ Dashboard (GET /api/v1/poll)
     * หากตั้งค่า onCommand(...) ไว้ ระบบจะแยกคู่ pin/value และเรียก Callback ให้อัตโนมัติ
     */
    String pollCommands();

    /**
     * ยืนยันการรับคำสั่ง (POST /api/v1/ack)
     */
    bool ackCommands();
};

extern CISIoTClient CISIoT;

#endif // CISIOT_H
