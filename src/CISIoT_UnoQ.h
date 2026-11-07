/**
 * @file CISIoT_UnoQ.h
 * @brief ตัวช่วยเชื่อมต่อ HTTP/HTTPS สำหรับบอร์ด Arduino UNO R4 WiFi / UNO Q (ใช้ไลบรารี WiFiS3)
 */

#ifndef CISIOT_UNOQ_H
#define CISIOT_UNOQ_H

#include <Arduino.h>

#define CISIOT_BOARD_NAME "ARDUINO_UNO_Q"

namespace CISIoT_UnoQ_Helper {

    inline int executeHttp(
        const String& method,
        const String& fullUrl,
        const String& token,
        const String& payload,
        String& responseOut
    ) {
        responseOut = "";
    #if __has_include(<WiFiS3.h>)
        bool isHttps = fullUrl.startsWith("https://");
        int schemeEnd = fullUrl.indexOf("://");
        String remainder = (schemeEnd != -1) ? fullUrl.substring(schemeEnd + 3) : fullUrl;

        int slashIdx = remainder.indexOf('/');
        String hostPort = (slashIdx != -1) ? remainder.substring(0, slashIdx) : remainder;
        String path = (slashIdx != -1) ? remainder.substring(slashIdx) : "/";

        String host = hostPort;
        int port = isHttps ? 443 : 80;
        int colonIdx = hostPort.indexOf(':');
        if (colonIdx != -1) {
            host = hostPort.substring(0, colonIdx);
            port = hostPort.substring(colonIdx + 1).toInt();
        }

        WiFiSSLClient sslClient;
        WiFiClient plainClient;
        Client* client = isHttps ? (Client*)&sslClient : (Client*)&plainClient;

        if (!client->connect(host.c_str(), port)) {
            return -1;
        }

        client->println(method + " " + path + " HTTP/1.1");
        client->println("Host: " + host);
        client->println("Authorization: Bearer " + token);
        client->println("Accept: application/json");
        client->println("Content-Type: application/json");
        client->println("Connection: close");
        if (payload.length() > 0) {
            client->println("Content-Length: " + String(payload.length()));
            client->println();
            client->print(payload);
        } else {
            client->println();
        }

        String raw = "";
        unsigned long timeout = millis();
        while (client->connected() && (millis() - timeout) < 6000) {
            while (client->available()) {
                char c = client->read();
                raw += c;
                timeout = millis();
            }
        }
        client->stop();

        int bodyIdx = raw.indexOf("\r\n\r\n");
        if (bodyIdx != -1) {
            responseOut = raw.substring(bodyIdx + 4);
        } else {
            responseOut = raw;
        }
        return (raw.indexOf(" 200 ") != -1 || raw.indexOf(" 201 ") != -1) ? 200 : 400;
    #else
        (void)method;
        (void)fullUrl;
        (void)token;
        (void)payload;
        return -1;
    #endif
    }

} // namespace CISIoT_UnoQ_Helper

#endif // CISIOT_UNOQ_H
