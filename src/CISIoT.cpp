/**
 * @file CISIoT.cpp
 * @brief ซอร์สโค้ดหลัก (.cpp) ของไลบรารี CISIoT สำหรับ ESP32, ESP8266, และ Arduino UNO R4 WiFi / UNO Q
 * @version 1.0.0
 */

#include "CISIoT.h"

CISIoTClient::CISIoTClient() {}

String CISIoTClient::escapeJsonString(const String& input) {
    String out = "";
    out.reserve(input.length() + 4);
    for (unsigned int i = 0; i < input.length(); i++) {
        char c = input.charAt(i);
        if (c == '\"') {
            out += "\\\"";
        } else if (c == '\\') {
            out += "\\\\";
        } else if (c == '\n') {
            out += "\\n";
        } else if (c == '\r') {
            out += "\\r";
        } else {
            out += c;
        }
    }
    return out;
}

String CISIoTClient::resolveUrl(const String& endpoint, bool useFallback) const {
    String base = (useFallback && _fallbackBaseUrl.length() > 0) ? _fallbackBaseUrl : _baseUrl;
    while (base.endsWith("/")) {
        base.remove(base.length() - 1);
    }
    if (endpoint.startsWith("/")) {
        return base + endpoint;
    }
    return base + "/" + endpoint;
}

void CISIoTClient::begin(
    const char* ssid,
    const char* password,
    const char* deviceToken,
    const char* baseUrl
) {
    _ssid = ssid;
    _password = password;
    _token = String(deviceToken);
    if (baseUrl != nullptr && strlen(baseUrl) > 0) {
        _baseUrl = String(baseUrl);
    }

#if defined(ESP32) || defined(ESP8266)
    WiFi.mode(WIFI_STA);
#endif
    reconnectWiFi();
}

void CISIoTClient::setFallbackBaseUrl(const char* fallbackUrl) {
    _fallbackBaseUrl = (fallbackUrl != nullptr) ? String(fallbackUrl) : "";
}

void CISIoTClient::setMaxFailsBeforeReboot(int maxFails) {
    _maxFailsBeforeReboot = maxFails;
}

void CISIoTClient::enableWatchdog(uint32_t timeoutSeconds) {
#if defined(ESP32)
    #if ESP_IDF_VERSION_MAJOR >= 5
        esp_task_wdt_config_t twdt_config = {
            .timeout_ms = timeoutSeconds * 1000,
            .idle_core_mask = (1 << portNUM_PROCESSORS) - 1,
            .trigger_panic = true
        };
        esp_task_wdt_init(&twdt_config);
    #else
        esp_task_wdt_init(timeoutSeconds, true);
    #endif
    esp_task_wdt_add(NULL);
    _wdtEnabled = true;
    Serial.printf("[CISIoT] ESP32 Watchdog enabled (%u sec)\n", timeoutSeconds);
#elif defined(ESP8266)
    ESP.wdtEnable(timeoutSeconds * 1000);
    _wdtEnabled = true;
    Serial.println("[CISIoT] ESP8266 Watchdog enabled");
#else
    (void)timeoutSeconds;
    _wdtEnabled = false;
#endif
}

void CISIoTClient::feedWatchdog() {
#if defined(ESP32)
    if (_wdtEnabled) {
        esp_task_wdt_reset();
    }
#elif defined(ESP8266)
    ESP.wdtFeed();
#endif
}

void CISIoTClient::reconnectWiFi() {
#if defined(ESP32) || defined(ESP8266)
    if (WiFi.status() == WL_CONNECTED) return;
    if (_ssid == nullptr) return;

    Serial.printf("[CISIoT] Connecting to WiFi: %s", _ssid);
    WiFi.begin(_ssid, _password);

    unsigned long startMs = millis();
    while (WiFi.status() != WL_CONNECTED && (millis() - startMs) < 15000) {
        delay(500);
        Serial.print(".");
        feedWatchdog();
    }

    if (WiFi.status() == WL_CONNECTED) {
        Serial.printf("\n[CISIoT] WiFi Connected! IP: %s\n", WiFi.localIP().toString().c_str());
    } else {
        Serial.println("\n[CISIoT] WiFi Connection Timeout!");
    }
#elif __has_include(<WiFiS3.h>)
    if (WiFi.status() == WL_CONNECTED) return;
    if (_ssid == nullptr) return;

    Serial.print("[CISIoT] Connecting UNO WiFi: ");
    Serial.println(_ssid);
    WiFi.begin(_ssid, _password);

    unsigned long startMs = millis();
    while (WiFi.status() != WL_CONNECTED && (millis() - startMs) < 15000) {
        delay(500);
        Serial.print(".");
    }
    Serial.println();
#endif
}

bool CISIoTClient::isConnected() const {
#if defined(ESP32) || defined(ESP8266) || __has_include(<WiFiS3.h>)
    return WiFi.status() == WL_CONNECTED;
#else
    return false;
#endif
}

String CISIoTClient::localIP() const {
#if defined(ESP32) || defined(ESP8266) || __has_include(<WiFiS3.h>)
    return WiFi.localIP().toString();
#else
    return "0.0.0.0";
#endif
}

void CISIoTClient::onCommand(CISIoTCommandCallback cb) {
    _commandCallback = cb;
}

int CISIoTClient::executeRequest(const String& method, const String& url, const String& payload, String& responseOut) {
#if defined(ESP32)
    HTTPClient http;
    http.setTimeout(8000);

    WiFiClientSecure secureClient;
    WiFiClient plainClient;

    bool isHttps = url.startsWith("https://");
    if (isHttps) {
        secureClient.setInsecure();
        if (!http.begin(secureClient, url)) return -2;
    } else {
        if (!http.begin(plainClient, url)) return -2;
    }

    http.addHeader("Content-Type", "application/json");
    http.addHeader("Accept", "application/json");
    http.addHeader("Authorization", "Bearer " + _token);

    int httpCode = (method == "POST") ? http.POST(payload) : http.GET();
    if (httpCode > 0) {
        responseOut = http.getString();
    }
    http.end();
    return httpCode;

#elif defined(ESP8266)
    HTTPClient http;
    http.setTimeout(8000);
    int httpCode = -1;

    if (url.startsWith("https://")) {
        std::unique_ptr<BearSSL::WiFiClientSecure> client(new BearSSL::WiFiClientSecure);
        client->setInsecure();
        if (http.begin(*client, url)) {
            http.addHeader("Content-Type", "application/json");
            http.addHeader("Accept", "application/json");
            http.addHeader("Authorization", "Bearer " + _token);
            httpCode = (method == "POST") ? http.POST(payload) : http.GET();
            if (httpCode > 0) responseOut = http.getString();
            http.end();
        }
    } else {
        WiFiClient client;
        if (http.begin(client, url)) {
            http.addHeader("Content-Type", "application/json");
            http.addHeader("Accept", "application/json");
            http.addHeader("Authorization", "Bearer " + _token);
            httpCode = (method == "POST") ? http.POST(payload) : http.GET();
            if (httpCode > 0) responseOut = http.getString();
            http.end();
        }
    }
    return httpCode;

#else
    return CISIoT_UnoQ_Helper::executeHttp(method, url, _token, payload, responseOut);
#endif
}

int CISIoTClient::sendHttpRequest(const String& method, const String& endpoint, const String& payload, String& responseOut) {
    if (!isConnected()) {
        reconnectWiFi();
        if (!isConnected()) return -1;
    }

    feedWatchdog();

    String targetUrl = resolveUrl(endpoint, false);
    int code = executeRequest(method, targetUrl, payload, responseOut);

    if (code <= 0 && _fallbackBaseUrl.length() > 0) {
        String fbUrl = resolveUrl(endpoint, true);
        Serial.print("[CISIoT] Primary URL failed, retrying fallback: ");
        Serial.println(fbUrl);
        code = executeRequest(method, fbUrl, payload, responseOut);
    }

    if (code > 0 && code < 500) {
        _failCount = 0;
    } else {
        _failCount++;
        if (_maxFailsBeforeReboot > 0 && _failCount >= _maxFailsBeforeReboot) {
            Serial.println("[CISIoT] Max consecutive HTTP failures reached! Rebooting...");
            delay(500);
            #if defined(ESP32) || defined(ESP8266)
                ESP.restart();
            #elif defined(NVIC_SystemReset)
                NVIC_SystemReset();
            #endif
        }
    }

    feedWatchdog();
    return code;
}

bool CISIoTClient::sendEnvironment(float temp, float humid, float pressure, const String& extraPinsJson) {
    String payload = "{";
    payload += "\"temperature\":" + String(temp, 2) + ",";
    payload += "\"humidity\":" + String(humid, 2) + ",";
    payload += "\"pressure\":" + String(pressure, 2);
    if (extraPinsJson.length() > 0) {
        payload += ",\"pins\":" + extraPinsJson;
    }
    payload += "}";

    String resp;
    int code = sendHttpRequest("POST", "/api/v1/telemetry", payload, resp);
    return (code >= 200 && code < 300);
}

bool CISIoTClient::virtualWrite(const String& pin, const String& value) {
    String safePin = escapeJsonString(pin);
    String safeVal = escapeJsonString(value);
    String payload = "{\"pins\":{\"" + safePin + "\":\"" + safeVal + "\"}}";
    String resp;
    int code = sendHttpRequest("POST", "/api/v1/telemetry", payload, resp);
    return (code >= 200 && code < 300);
}

bool CISIoTClient::virtualWrite(const String& pin, float value, uint8_t decimals) {
    String safePin = escapeJsonString(pin);
    String payload = "{\"pins\":{\"" + safePin + "\":" + String(value, (unsigned int)decimals) + "}}";
    String resp;
    int code = sendHttpRequest("POST", "/api/v1/telemetry", payload, resp);
    return (code >= 200 && code < 300);
}

bool CISIoTClient::virtualWrite(const String& pin, int value) {
    String safePin = escapeJsonString(pin);
    String payload = "{\"pins\":{\"" + safePin + "\":" + String(value) + "}}";
    String resp;
    int code = sendHttpRequest("POST", "/api/v1/telemetry", payload, resp);
    return (code >= 200 && code < 300);
}

bool CISIoTClient::reportStatus(const String& pin, const String& value) {
    String safePin = escapeJsonString(pin);
    String safeVal = escapeJsonString(value);
    String payload = "{\"pin\":\"" + safePin + "\",\"value\":\"" + safeVal + "\"}";
    String resp;
    int code = sendHttpRequest("POST", "/api/v1/status", payload, resp);
    return (code >= 200 && code < 300);
}

bool CISIoTClient::reportStatus(const String& pin, int value) {
    return reportStatus(pin, String(value));
}

bool CISIoTClient::reportDoorStatus(const String& doorState) {
    String safeVal = escapeJsonString(doorState);
    String payload = "{\"door_status\":\"" + safeVal + "\"}";
    String resp;
    int code = sendHttpRequest("POST", "/api/v1/status", payload, resp);
    return (code >= 200 && code < 300);
}

void CISIoTClient::dispatchPendingCommands(const String& jsonResponse) {
    if (_commandCallback == nullptr) return;
    if (jsonResponse.indexOf("\"has_pending\":true") == -1) return;

    int cmdKeyIdx = jsonResponse.indexOf("\"commands\":");
    if (cmdKeyIdx == -1) return;

    int braceStart = jsonResponse.indexOf('{', cmdKeyIdx);
    int braceEnd = jsonResponse.indexOf('}', braceStart);
    if (braceStart == -1 || braceEnd == -1 || braceEnd <= braceStart + 1) return;

    String cmdObj = jsonResponse.substring(braceStart + 1, braceEnd);
    int pos = 0;
    while (pos < (int)cmdObj.length()) {
        int q1 = cmdObj.indexOf('\"', pos);
        if (q1 == -1) break;
        int q2 = cmdObj.indexOf('\"', q1 + 1);
        if (q2 == -1) break;

        String key = cmdObj.substring(q1 + 1, q2);
        int colon = cmdObj.indexOf(':', q2 + 1);
        if (colon == -1) break;

        int valStart = colon + 1;
        while (valStart < (int)cmdObj.length() && cmdObj.charAt(valStart) == ' ') {
            valStart++;
        }

        String val = "";
        if (valStart < (int)cmdObj.length() && cmdObj.charAt(valStart) == '\"') {
            int vq2 = cmdObj.indexOf('\"', valStart + 1);
            if (vq2 == -1) break;
            val = cmdObj.substring(valStart + 1, vq2);
            pos = cmdObj.indexOf(',', vq2 + 1);
        } else {
            int comma = cmdObj.indexOf(',', valStart);
            if (comma == -1) {
                val = cmdObj.substring(valStart);
                val.trim();
                pos = cmdObj.length();
            } else {
                val = cmdObj.substring(valStart, comma);
                val.trim();
                pos = comma + 1;
            }
        }

        if (key.length() > 0) {
            _commandCallback(key, val);
        }

        if (pos == -1) break;
    }
}

String CISIoTClient::pollCommands() {
    String resp = "";
    int code = sendHttpRequest("GET", "/api/v1/poll", "", resp);
    if (code == 200 && resp.length() > 0) {
        dispatchPendingCommands(resp);
    }
    return resp;
}

bool CISIoTClient::ackCommands() {
    String resp = "";
    int code = sendHttpRequest("POST", "/api/v1/ack", "{}", resp);
    return (code >= 200 && code < 300);
}

CISIoTClient CISIoT;
