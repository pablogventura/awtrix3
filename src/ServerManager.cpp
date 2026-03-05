#include "ServerManager.h"
#include "Globals.h"
#include <WebServer.h>
#include <esp-fs-webserver.h>
#include "htmls.h"
#include <Update.h>
#include <ESPmDNS.h>
#include <LittleFS.h>
#include <WiFi.h>
#include "DisplayManager.h"
#include "UpdateManager.h"
#include "PeripheryManager.h"
#include "PowerManager.h"
#include <WiFiUdp.h>
#include <HTTPClient.h>
#include "Games/GameManager.h"
#include <EEPROM.h>
#include <ArduinoJson.h>
#include <functional>

WiFiUDP udp;

// Remove keys that can cause duplicate menu sections in the config page (legacy or duplicate box titles)
static void removeDuplicateConfigBoxKeys()
{
    if (!LittleFS.exists("/DoNotTouch.json"))
        return;
    File file = LittleFS.open("/DoNotTouch.json", "r");
    if (!file)
        return;
    String content = file.readString();
    file.close();
    if (content.length() == 0)
        return;
    DynamicJsonDocument doc(content.length() * 2);
    if (deserializeJson(doc, content) != DeserializationError::Ok || doc.overflowed())
        return;
    const char *boxTitles[] = {"Network", "MQTT", "Time", "Icons", "Auth", "Files"};
    for (const char *title : boxTitles)
        if (doc.containsKey(title))
            doc.remove(title);
    file = LittleFS.open("/DoNotTouch.json", "w");
    if (file)
    {
        serializeJsonPretty(doc, file);
        file.close();
    }
}

unsigned int localUdpPort = 4210;
char incomingPacket[255];

// Pufferdefinition
#define BUFFER_SIZE 64
char dataBuffer[BUFFER_SIZE];
int bufferIndex = 0;

// Aktueller verbundener Client
WiFiClient currentClient = WiFiClient();
WebServer server(80);
FSWebServer mws(LittleFS, server);

// Erstelle eine Server-Instanz
WiFiServer TCPserver(8080);

// The getter for the instantiated singleton instance
ServerManager_ &ServerManager_::getInstance()
{
    static ServerManager_ instance;
    return instance;
}

// Initialize the global shared instance
ServerManager_ &ServerManager = ServerManager.getInstance();

void versionHandler()
{
    WebServerClass *webRequest = mws.getRequest();
    webRequest->send(200, F("text/plain"), VERSION);
}

void ServerManager_::erase()
{
    DisplayManager.HSVtext(0, 6, "RESET", true, 0);
    wifi_config_t conf;
    memset(&conf, 0, sizeof(conf)); // Set all the bytes in the structure to 0
    esp_wifi_set_config(WIFI_IF_STA, &conf);
    LittleFS.format();
    delay(200);
    formatSettings();
    delay(200);
}

void saveHandler()
{
    WebServerClass *webRequest = mws.getRequest();
    ServerManager.getInstance().loadSettings();
    webRequest->send(200);
}

void addHandler()
{

    mws.addHandler("/api/power", HTTP_POST, []()
                   { DisplayManager.powerStateParse(mws.webserver->arg("plain").c_str()); mws.webserver->send(200,F("text/plain"),F("OK")); });
    mws.addHandler(
        "/api/sleep", HTTP_POST, []()
        { 
            mws.webserver->send(200,F("text/plain"),F("OK"));
            DisplayManager.setPower(false);
            PowerManager.sleepParser(mws.webserver->arg("plain").c_str()); });
    mws.addHandler("/api/loop", HTTP_GET, []()
                   { mws.webserver->send_P(200, "application/json", DisplayManager.getAppsAsJson().c_str()); });
    mws.addHandler("/api/effects", HTTP_GET, []()
                   { mws.webserver->send_P(200, "application/json", DisplayManager.getEffectNames().c_str()); });
    mws.addHandler("/api/transitions", HTTP_GET, []()
                   { mws.webserver->send_P(200, "application/json", DisplayManager.getTransitionNames().c_str()); });
    mws.addHandler("/api/reboot", HTTP_ANY, []()
                   { mws.webserver->send(200,F("text/plain"),F("OK")); delay(200); ESP.restart(); });
    mws.addHandler("/api/rtttl", HTTP_POST, []()
                   { mws.webserver->send(200,F("text/plain"),F("OK")); PeripheryManager.playRTTTLString(mws.webserver->arg("plain").c_str()); });
    mws.addHandler("/api/sound", HTTP_POST, []()
                   { if (PeripheryManager.parseSound(mws.webserver->arg("plain").c_str())){
                    mws.webserver->send(200,F("text/plain"),F("OK")); 
                   }else{
                    mws.webserver->send(404,F("text/plain"),F("FileNotFound"));  
                   }; });

    mws.addHandler("/api/moodlight", HTTP_POST, []()
                   {
                    if (DisplayManager.moodlight(mws.webserver->arg("plain").c_str()))
                    {
                        mws.webserver->send(200, F(F("text/plain")), F("OK"));
                    }
                    else
                    {
                        mws.webserver->send(500, F("text/plain"), F("ErrorParsingJson"));
                    } });
    mws.addHandler("/api/notify", HTTP_POST, []()
                   {
                       if (DisplayManager.generateNotification(1,mws.webserver->arg("plain").c_str()))
                       {
                        mws.webserver->send(200, F("text/plain"), F("OK"));
                       }else{
                        mws.webserver->send(500, F("text/plain"), F("ErrorParsingJson"));
                       } });
    mws.addHandler("/api/nextapp", HTTP_ANY, []()
                   { DisplayManager.nextApp(); mws.webserver->send(200,F("text/plain"),F("OK")); });
    mws.addHandler("/fullscreen", HTTP_GET, []()
                   {
    String fps = mws.webserver->arg("fps");
    if (fps == "") {
        fps = "30"; 
    }
    String finalHTML = screenfull_html; 
    finalHTML.replace("%%FPS%%", fps);

    mws.webserver->send(200, "text/html", finalHTML.c_str()); });
    mws.addHandler("/screen", HTTP_GET, []()
                   { mws.webserver->send(200, "text/html", screen_html); });
    mws.addHandler("/backup", HTTP_GET, []()
                   { mws.webserver->send(200, "text/html", backup_html); });
    mws.addHandler("/lang", HTTP_GET, []()
                   { mws.webserver->send(200, "text/html", lang_html); });
    mws.addHandler("/api/previousapp", HTTP_POST, []()
                   { DisplayManager.previousApp(); mws.webserver->send(200,F("text/plain"),F("OK")); });
    mws.addHandler("/api/notify/dismiss", HTTP_ANY, []()
                   { DisplayManager.dismissNotify(); mws.webserver->send(200,F("text/plain"),F("OK")); });
    mws.addHandler("/api/apps", HTTP_POST, []()
                   { DisplayManager.updateAppVector(mws.webserver->arg("plain").c_str()); mws.webserver->send(200,F("text/plain"),F("OK")); });
    mws.addHandler(
        "/api/switch", HTTP_POST, []()
        {
        if (DisplayManager.switchToApp(mws.webserver->arg("plain").c_str()))
        {
            mws.webserver->send(200, F("text/plain"), F("OK"));
        }
        else
        {
            mws.webserver->send(500, F("text/plain"), F("FAILED"));
        } });
    mws.addHandler("/api/apps", HTTP_GET, []()
                   { mws.webserver->send_P(200, "application/json", DisplayManager.getAppsWithIcon().c_str()); });
    mws.addHandler("/api/settings", HTTP_POST, []()
                   { DisplayManager.setNewSettings(mws.webserver->arg("plain").c_str()); mws.webserver->send(200,F("text/plain"),F("OK")); });
    mws.addHandler("/api/erase", HTTP_ANY, []()
                   { ServerManager.erase();  mws.webserver->send(200,F("text/plain"),F("OK"));delay(200); ESP.restart(); });
    mws.addHandler("/api/resetSettings", HTTP_ANY, []()
                   { formatSettings();   mws.webserver->send(200,F("text/plain"),F("OK"));delay(200); ESP.restart(); });
    mws.addHandler("/api/reorder", HTTP_POST, []()
                   { DisplayManager.reorderApps(mws.webserver->arg("plain").c_str()); mws.webserver->send(200,F("text/plain"),F("OK")); });
    mws.addHandler("/api/settings", HTTP_GET, []()
                   { mws.webserver->send_P(200, "application/json", DisplayManager.getSettings().c_str()); });
    mws.addHandler("/api/lang", HTTP_GET, []()
                   { String j = "{\"lang\":" + String(MENU_LANGUAGE) + "}"; mws.webserver->send(200, "application/json", j); });
    mws.addHandler("/api/custom", HTTP_POST, []()
                   { 
                    if (DisplayManager.parseCustomPage(mws.webserver->arg("name"),mws.webserver->arg("plain").c_str(),false)){
                        mws.webserver->send(200,F("text/plain"),F("OK")); 
                    }else{
                        mws.webserver->send(500,F("text/plain"),F("ErrorParsingJson")); 
                    } });
    mws.addHandler("/api/stats", HTTP_GET, []()
                   { mws.webserver->send_P(200, "application/json", DisplayManager.getStats().c_str()); });
    mws.addHandler("/api/screen", HTTP_GET, []()
                   { mws.webserver->send_P(200, "application/json", DisplayManager.ledsAsJson().c_str()); });
    mws.addHandler("/api/indicator1", HTTP_POST, []()
                   { 
                    if (DisplayManager.indicatorParser(1,mws.webserver->arg("plain").c_str())){
                     mws.webserver->send(200,F("text/plain"),F("OK")); 
                    }else{
                         mws.webserver->send(500,F("text/plain"),F("ErrorParsingJson")); 
                    } });
    mws.addHandler("/api/indicator2", HTTP_POST, []()
                   { 
                    if (DisplayManager.indicatorParser(2,mws.webserver->arg("plain").c_str())){
                     mws.webserver->send(200,F("text/plain"),F("OK")); 
                    }else{
                         mws.webserver->send(500,F("text/plain"),F("ErrorParsingJson")); 
                    } });
    mws.addHandler("/api/indicator3", HTTP_POST, []()
                   { 
                    if (DisplayManager.indicatorParser(3,mws.webserver->arg("plain").c_str())){
                     mws.webserver->send(200,F("text/plain"),F("OK")); 
                    }else{
                         mws.webserver->send(500,F("text/plain"),F("ErrorParsingJson")); 
                    } });
    mws.addHandler("/api/doupdate", HTTP_POST, []()
                   { 
                    if (UpdateManager.checkUpdate(true)){
                        mws.webserver->send(200,F("text/plain"),F("OK"));
                        UpdateManager.updateFirmware();
                    }else{
                        mws.webserver->send(404,F("text/plain"),"NoUpdateFound");    
                    } });
    mws.addHandler("/api/r2d2", HTTP_POST, []()
                   { PeripheryManager.r2d2(mws.webserver->arg("plain").c_str()); mws.webserver->send(200,F("text/plain"),F("OK")); });

    // GET /api/config - return system config (DoNotTouch.json) for automation
    mws.addHandler("/api/config", HTTP_GET, []() {
        if (!LittleFS.exists("/DoNotTouch.json")) {
            mws.webserver->send(404, F("application/json"), F("{\"error\":\"No config\"}"));
            return;
        }
        File file = LittleFS.open("/DoNotTouch.json", "r");
        if (!file) {
            mws.webserver->send(500, F("application/json"), F("{\"error\":\"Cannot read\"}"));
            return;
        }
        String content = file.readString();
        file.close();
        mws.webserver->send(200, F("application/json"), content);
    });

    // POST /api/config - update system config (merge with existing), then reload
    mws.addHandler("/api/config", HTTP_POST, []() {
        String body = mws.webserver->arg("plain");
        if (body.length() == 0) {
            mws.webserver->send(400, F("text/plain"), F("Empty body"));
            return;
        }
        DynamicJsonDocument req(1024);
        if (deserializeJson(req, body) != DeserializationError::Ok || req.overflowed()) {
            mws.webserver->send(400, F("text/plain"), F("Invalid JSON"));
            return;
        }
        DynamicJsonDocument doc(1024);
        if (LittleFS.exists("/DoNotTouch.json")) {
            File f = LittleFS.open("/DoNotTouch.json", "r");
            if (f) {
                deserializeJson(doc, f);
                f.close();
            }
        }
        for (JsonPair kv : req.as<JsonObject>())
            doc[kv.key()] = kv.value();
        File f = LittleFS.open("/DoNotTouch.json", "w");
        if (!f) {
            mws.webserver->send(500, F("text/plain"), F("Cannot write"));
            return;
        }
        serializeJsonPretty(doc, f);
        f.close();
        ServerManager.loadSettings();
        mws.webserver->send(200, F("text/plain"), F("OK"));
    });

    // GET /api/backup - return JSON with all files (path + base64 content) for scripting
    mws.addHandler("/api/backup", HTTP_GET, []() {
        String out = F("{\"files\":[");
        bool first = true;
        File root = LittleFS.open("/", "r");
        if (!root || !root.isDirectory()) {
            mws.webserver->send(500, F("application/json"), F("{\"error\":\"FS\"}"));
            return;
        }
        std::function<void(File &, const String &)> collect = [&](File &dir, const String &path) {
            File entry = dir.openNextFile();
            while (entry) {
                String name = entry.name();
                if (name.lastIndexOf('/') >= 0) name = name.substring(name.lastIndexOf('/') + 1);
                String fullPath = path + name;
                if (entry.isDirectory()) {
                    File sub = LittleFS.open(fullPath, "r");
                    if (sub) { collect(sub, fullPath + "/"); sub.close(); }
                } else {
                    size_t len = entry.size();
                    if (len > 0 && len < 65536) {
                        uint8_t *buf = (uint8_t *)malloc(len);
                        if (buf) {
                            entry.read(buf, len);
                            size_t b64len = (len + 2) / 3 * 4;
                            char *b64 = (char *)malloc(b64len + 1);
                            if (b64) {
                                static const char enc[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
                                for (size_t i = 0, j = 0; i < len; i += 3, j += 4) {
                                    uint32_t v = buf[i] << 16;
                                    if (i + 1 < len) v |= buf[i + 1] << 8;
                                    if (i + 2 < len) v |= buf[i + 2];
                                    b64[j] = enc[(v >> 18) & 63]; b64[j + 1] = enc[(v >> 12) & 63];
                                    b64[j + 2] = (i + 1 < len) ? enc[(v >> 6) & 63] : '=';
                                    b64[j + 3] = (i + 2 < len) ? enc[v & 63] : '=';
                                }
                                b64[b64len] = '\0';
                                String p = fullPath; p.replace("\\", "\\\\"); p.replace("\"", "\\\"");
                                if (!first) out += ',';
                                out += "{\"path\":\"" + p + "\",\"content\":\"" + String(b64) + "\"}";
                                first = false;
                                free(b64);
                            }
                            free(buf);
                        }
                    }
                }
                entry.close();
                entry = dir.openNextFile();
            }
        };
        collect(root, "/");
        root.close();
        out += F("]}");
        mws.webserver->send(200, F("application/json"), out);
    });

    // POST /api/restore - accept JSON { "files": [ {"path":"/x","content":"base64"} ] } and write files
    mws.addHandler("/api/restore", HTTP_POST, []() {
        String body = mws.webserver->arg("plain");
        if (body.length() == 0) {
            mws.webserver->send(400, F("text/plain"), F("Empty body"));
            return;
        }
        DynamicJsonDocument doc(12288);
        if (deserializeJson(doc, body) != DeserializationError::Ok || doc.overflowed()) {
            mws.webserver->send(400, F("text/plain"), F("Invalid JSON"));
            return;
        }
        if (!doc["files"].is<JsonArray>()) {
            mws.webserver->send(400, F("text/plain"), F("Missing files array"));
            return;
        }
        JsonArray arr = doc["files"].as<JsonArray>();
        for (JsonObject obj : arr) {
            const char *path = obj["path"];
            const char *content = obj["content"];
            if (!path || !content) continue;
            String p(path);
            if (p.length() == 0 || p == "/") continue;
            if (!p.startsWith("/")) p = "/" + p;
            size_t clen = strlen(content);
            size_t outLen = (clen / 4) * 3;
            if (clen >= 4 && content[clen - 1] == '=') { outLen--; if (content[clen - 2] == '=') outLen--; }
            if (outLen > 0 && outLen < 32768) {
                uint8_t *buf = (uint8_t *)malloc(outLen);
                if (buf) {
                    static const int T[128] = {-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,62,-1,-1,-1,63,52,53,54,55,56,57,58,59,60,61,-1,-1,-1,-1,-1,-1,-1,0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,-1,-1,-1,-1,-1,-1,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51,-1,-1,-1,-1,-1};
                    size_t j = 0;
                    for (size_t i = 0; i < clen && j < outLen; i += 4) {
                        int a = (content[i] & 127) < 128 ? T[content[i] & 127] : -1;
                        int b = (i+1 < clen && (content[i+1] & 127) < 128) ? T[content[i+1] & 127] : -1;
                        int c = (i+2 < clen && (content[i+2] & 127) < 128) ? T[content[i+2] & 127] : -1;
                        int d = (i+3 < clen && (content[i+3] & 127) < 128) ? T[content[i+3] & 127] : -1;
                        if (a < 0 || b < 0) break;
                        buf[j++] = (a << 2) | (b >> 4);
                        if (j < outLen && c >= 0) buf[j++] = ((b & 15) << 4) | (c >> 2);
                        if (j < outLen && d >= 0) buf[j++] = ((c & 3) << 6) | d;
                    }
                    // Ensure parent directory (and any nested dirs) exist
                    for (int i = 1; i < (int)p.length(); i++) {
                        if (p[i] == '/') {
                            String parent = p.substring(0, i);
                            if (!LittleFS.exists(parent))
                                LittleFS.mkdir(parent);
                        }
                    }
                    File f = LittleFS.open(p, "w");
                    if (f) { f.write(buf, j); f.close(); }
                    free(buf);
                }
            }
        }
        mws.webserver->send(200, F("text/plain"), F("OK"));
    });

    // GET /api/dev - return dev.json for automation
    mws.addHandler("/api/dev", HTTP_GET, []() {
        if (!LittleFS.exists("/dev.json")) {
            mws.webserver->send(200, F("application/json"), F("{}"));
            return;
        }
        File file = LittleFS.open("/dev.json", "r");
        if (!file) {
            mws.webserver->send(500, F("application/json"), F("{\"error\":\"Cannot read\"}"));
            return;
        }
        String content = file.readString();
        file.close();
        mws.webserver->send(200, F("application/json"), content);
    });

    // PUT/POST /api/dev - merge and save dev.json (reboot may be required for some options)
    mws.addHandler("/api/dev", HTTP_PUT, []() {
        String body = mws.webserver->arg("plain");
        DynamicJsonDocument doc(1024);
        if (LittleFS.exists("/dev.json")) {
            File f = LittleFS.open("/dev.json", "r");
            if (f) { deserializeJson(doc, f); f.close(); }
        }
        if (body.length() > 0) {
            DynamicJsonDocument req(1024);
            if (deserializeJson(req, body) == DeserializationError::Ok && !req.overflowed())
                for (JsonPair kv : req.as<JsonObject>()) doc[kv.key()] = kv.value();
        }
        File f = LittleFS.open("/dev.json", "w");
        if (!f) {
            mws.webserver->send(500, F("text/plain"), F("Cannot write"));
            return;
        }
        serializeJsonPretty(doc, f);
        f.close();
        mws.webserver->send(200, F("text/plain"), F("OK"));
    });
    mws.addHandler("/api/dev", HTTP_POST, []() {
        String body = mws.webserver->arg("plain");
        DynamicJsonDocument doc(1024);
        if (LittleFS.exists("/dev.json")) {
            File f = LittleFS.open("/dev.json", "r");
            if (f) { deserializeJson(doc, f); f.close(); }
        }
        if (body.length() > 0) {
            DynamicJsonDocument req(1024);
            if (deserializeJson(req, body) == DeserializationError::Ok && !req.overflowed())
                for (JsonPair kv : req.as<JsonObject>()) doc[kv.key()] = kv.value();
        }
        File f = LittleFS.open("/dev.json", "w");
        if (!f) {
            mws.webserver->send(500, F("text/plain"), F("Cannot write"));
            return;
        }
        serializeJsonPretty(doc, f);
        f.close();
        mws.webserver->send(200, F("text/plain"), F("OK"));
    });

    mws.addHandler("/settings", HTTP_GET, []() { mws.webserver->send(200, F("text/html"), settings_html); });
}

void ServerManager_::setup()
{
    esp_wifi_set_max_tx_power(80); // 82 * 0.25 dBm = 20.5 dBm
    esp_wifi_set_ps(WIFI_PS_NONE); // Power Saving deaktivieren
    if (!local_IP.fromString(NET_IP) || !gateway.fromString(NET_GW) || !subnet.fromString(NET_SN) || !primaryDNS.fromString(NET_PDNS) || !secondaryDNS.fromString(NET_SDNS))
        NET_STATIC = false;
    if (NET_STATIC)
    {
        WiFi.config(local_IP, gateway, subnet, primaryDNS, secondaryDNS);
    }
    WiFi.setHostname(HOSTNAME.c_str()); // define hostname
    myIP = mws.startWiFi(AP_TIMEOUT * 1000, HOSTNAME.c_str(), "12345678");
    isConnected = !(myIP == IPAddress(192, 168, 4, 1));
    if (DEBUG_MODE)
        DEBUG_PRINTF("My IP: %d.%d.%d.%d", myIP[0], myIP[1], myIP[2], myIP[3]);
    mws.setAuth(AUTH_USER, AUTH_PASS);
    if (isConnected)
    {
        removeDuplicateConfigBoxKeys();
        mws.addOptionBox("Network");
        mws.addOption("Static IP", NET_STATIC);
        mws.addOption("Local IP", NET_IP);
        mws.addOption("Gateway", NET_GW);
        mws.addOption("Subnet", NET_SN);
        mws.addOption("Primary DNS", NET_PDNS);
        mws.addOption("Secondary DNS", NET_SDNS);
        mws.addOptionBox("MQTT");
        mws.addOption("Broker", MQTT_HOST);
        mws.addOption("Port", MQTT_PORT);
        mws.addOption("Username", MQTT_USER);
        mws.addOption("Password", MQTT_PASS);
        mws.addOption("Prefix", MQTT_PREFIX);
        mws.addOption("Homeassistant Discovery", HA_DISCOVERY);
        mws.addOptionBox("Time");
        mws.addOption("NTP Server", NTP_SERVER);
        mws.addOption("Timezone", NTP_TZ);
        mws.addHTML("<p>Find your timezone at <a href='https://github.com/nayarsystems/posix_tz_db/blob/master/zones.csv' target='_blank' rel='noopener noreferrer'>posix_tz_db</a>.</p>", "tz_link");
        mws.addOptionBox("Icons");
        mws.addHTML(custom_html, "icon_html");
        mws.addCSS(custom_css);
        mws.addJavascript(custom_script);
        mws.addOptionBox("Auth");
        mws.addOption("Auth Username", AUTH_USER);
        mws.addOption("Auth Password", AUTH_PASS);
        mws.addHandler("/save", HTTP_POST, saveHandler);
        addHandler();
        udp.begin(localUdpPort);
        if (DEBUG_MODE)
            DEBUG_PRINTLN(F("Webserver loaded"));
    }
    mws.addHandler("/version", HTTP_GET, versionHandler);
    mws.begin(WEB_PORT);

    if (!MDNS.begin(HOSTNAME))
    {
        if (DEBUG_MODE)
            DEBUG_PRINTLN(F("Error starting mDNS"));
    }
    else
    {
        MDNS.addService("http", "tcp", 80);
        MDNS.addService("awtrix", "tcp", 80);
        MDNS.addServiceTxt("awtrix", "tcp", "id", uniqueID);
        MDNS.addServiceTxt("awtrix", "tcp", "name", HOSTNAME.c_str());
        MDNS.addServiceTxt("awtrix", "tcp", "type", "awtrix3");
    }

    configTzTime(NTP_TZ.c_str(), NTP_SERVER.c_str());
    tm timeInfo;
    getLocalTime(&timeInfo);
    TCPserver.begin();
    TCPserver.setNoDelay(true);
}

void ServerManager_::tick()
{
    mws.run();

    if (!AP_MODE)
    {
        int packetSize = udp.parsePacket();
        if (packetSize)
        {
            int len = udp.read(incomingPacket, 255);
            if (len > 0)
            {
                incomingPacket[len] = 0;
            }
            if (strcmp(incomingPacket, "FIND_AWTRIX") == 0)
            {
                udp.beginPacket(udp.remoteIP(), 4211);
                if (WEB_PORT != 80)
                {
                    char buffer[128];
                    sprintf(buffer, "%s:%d", HOSTNAME.c_str(), WEB_PORT);
                    udp.printf(buffer);
                }
                else
                {
                    udp.printf(HOSTNAME.c_str());
                }

                udp.endPacket();
            }
        }
    }

    if (!currentClient || !currentClient.connected()) {
        if (TCPserver.hasClient()) {
            if (currentClient) {
                currentClient.stop();
                Serial.println("Vorheriger Client getrennt, um neuen Client zu akzeptieren.");
            }
            currentClient = TCPserver.available();
            Serial.println("Neuer Client verbunden.");
        }
    }

    if (currentClient && currentClient.connected()) {
        while (currentClient.available()) {
            char incomingByte = currentClient.read();            
            if (incomingByte == '\n') {
                dataBuffer[bufferIndex] = '\0';               
                GameManager.ControllerInput(dataBuffer);
                bufferIndex = 0;
            }
            else if (incomingByte != '\r') {
                if (bufferIndex < BUFFER_SIZE - 1) {
                    dataBuffer[bufferIndex++] = incomingByte;
                }
                else {
                    bufferIndex = 0;
                }
            }
        }
    }
}

void ServerManager_::sendTCP(String message)
{
    if (currentClient && currentClient.connected()) {
        currentClient.print(message);
    }
}

void ServerManager_::loadSettings()
{
    if (LittleFS.exists("/DoNotTouch.json"))
    {
        File file = LittleFS.open("/DoNotTouch.json", "r");
        DynamicJsonDocument doc(file.size() * 1.33);
        DeserializationError error = deserializeJson(doc, file);
        if (error)
        {
            file.close();
            return;
        }
        if (doc.overflowed())
        {
            file.close();
            return;
        }

        NTP_SERVER = doc["NTP Server"].as<String>();
        NTP_TZ = doc["Timezone"].as<String>();
        MQTT_HOST = doc["Broker"].as<String>();
        MQTT_PORT = doc["Port"].as<uint16_t>();
        MQTT_USER = doc["Username"].as<String>();
        MQTT_PASS = doc["Password"].as<String>();
        MQTT_PREFIX = doc["Prefix"].as<String>();
        MQTT_PREFIX.trim();
        NET_STATIC = doc["Static IP"];
        HA_DISCOVERY = doc["Homeassistant Discovery"];
        NET_IP = doc["Local IP"].as<String>();
        NET_GW = doc["Gateway"].as<String>();
        NET_SN = doc["Subnet"].as<String>();
        NET_PDNS = doc["Primary DNS"].as<String>();
        NET_SDNS = doc["Secondary DNS"].as<String>();
        if (doc["Auth Username"].is<String>())
            AUTH_USER = doc["Auth Username"].as<String>();
        if (doc["Auth Password"].is<String>())
            AUTH_PASS = doc["Auth Password"].as<String>();

        file.close();
        DisplayManager.applyAllSettings();
        if (DEBUG_MODE)
            DEBUG_PRINTLN(F("Webserver configuration loaded"));
        doc.clear();
        return;
    }
    else if (DEBUG_MODE)
        DEBUG_PRINTLN(F("Webserver configuration file not exist"));
    return;
}

void ServerManager_::sendButton(byte btn, bool state)
{
    if (BUTTON_CALLBACK == "")
        return;
    static bool btn0State, btn1State, btn2State;
    String payload;
    switch (btn)
    {
    case 0:
        if (btn0State != state)
        {
            btn0State = state;
            payload = "button=left&state=" + String(state) + "&uid=" + uniqueID;
        }
        break;
    case 1:
        if (btn1State != state)
        {
            btn1State = state;
            payload = "button=middle&state=" + String(state) + "&uid=" + uniqueID;
        }
        break;
    case 2:
        if (btn2State != state)
        {
            btn2State = state;
            payload = "button=right&state=" + String(state) + "&uid=" + uniqueID;
        }
        break;
    default:
        return;
    }
    if (!payload.isEmpty())
    {
        HTTPClient http;
        http.begin(BUTTON_CALLBACK);
        http.addHeader("Content-Type", "application/x-www-form-urlencoded");
        http.POST(payload);
        http.end();
    }
}
