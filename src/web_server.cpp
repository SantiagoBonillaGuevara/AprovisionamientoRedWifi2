#include "web_server.h"
#include "wifi_manager.h"
#include "html_page.h"

// ---- Definición de variables globales ----
WebServer server(80);
bool shouldRestartAfterConnect = false;

// ------------------------------------------------------------------
// Helpers de parseo JSON manual (evita dependencia de librería extra)
// ------------------------------------------------------------------

static String extractJsonString(const String &body, const String &key) {
  int keyIdx = body.indexOf("\"" + key + "\"");
  if (keyIdx < 0) return "";
  int colon = body.indexOf(':', keyIdx);
  int q1    = body.indexOf('"', colon + 1);
  int q2    = body.indexOf('"', q1 + 1);
  if (q1 < 0 || q2 <= q1) return "";
  return body.substring(q1 + 1, q2);
}

// ------------------------------------------------------------------
// Handlers
// ------------------------------------------------------------------

void handleRoot() {
  server.send(200, "text/html", indexPage());
}

void handleScan() {
  Serial.println("Escaneando redes WiFi...");
  int n = WiFi.scanNetworks();
  Serial.printf("Encontradas %d redes\n", n);

  String json = "[";
  for (int i = 0; i < n; ++i) {
    String ssid = WiFi.SSID(i);
    ssid.replace("\"", "");           // evitar comillas que rompan el JSON
    json += "\"" + ssid + "\"";
    if (i < n - 1) json += ",";
  }
  json += "]";

  server.send(200, "application/json", json);
}

void handleConnect() {
  if (server.method() != HTTP_POST) {
    server.send(405, "text/plain", "Usar POST con JSON {ssid, pass}");
    return;
  }

  String body = server.arg("plain");
  Serial.print("POST /connect body: ");
  Serial.println(body);

  String ssid = extractJsonString(body, "ssid");
  String pass = extractJsonString(body, "pass");

  if (ssid.length() == 0) {
    server.send(400, "application/json", "{\"success\":false,\"message\":\"SSID vacío\"}");
    return;
  }

  // Intentar conectar con las credenciales recibidas
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid.c_str(), pass.c_str());

  unsigned long start = millis();
  bool connected = false;
  while (millis() - start < 10000) {
    if (WiFi.status() == WL_CONNECTED) { connected = true; break; }
    delay(200);
  }

  if (connected) {
    saveCredentials(ssid, pass);
    String ip   = WiFi.localIP().toString();
    String resp = "{\"success\":true,\"ip\":\"" + ip + "\"}";
    server.send(200, "application/json", resp);
    Serial.println("Conexión exitosa desde /connect. IP: " + ip);

    // Apagar AP y DNS para que el dispositivo opere como STA
    dnsServer.stop();
    delay(200);
    WiFi.softAPdisconnect(true);
    shouldRestartAfterConnect = true;
  } else {
    server.send(200, "application/json",
      "{\"success\":false,\"message\":\"No se pudo conectar con esas credenciales\"}");
  }
}

void handleStatus() {
  bool   connected = (WiFi.status() == WL_CONNECTED);
  String ss        = connected ? WiFi.SSID()             : "";
  String ip        = connected ? WiFi.localIP().toString() : "";
  String json = "{\"connected\":" + String(connected ? "true" : "false") +
                ",\"ssid\":\""    + ss + "\",\"ip\":\"" + ip + "\"}";
  server.send(200, "application/json", json);
}

void handleForget() {
  clearCredentials();
  server.send(200, "text/plain", "Credenciales borradas. Reiniciando...");
  delay(2000);
  ESP.restart();
}

// ------------------------------------------------------------------
// Registro de rutas
// ------------------------------------------------------------------

void setupRoutes() {
  server.on("/",        HTTP_GET,  handleRoot);
  server.on("/scan",    HTTP_GET,  handleScan);
  server.on("/connect", HTTP_POST, handleConnect);
  server.on("/status",  HTTP_GET,  handleStatus);
  server.on("/forget",  HTTP_GET,  handleForget);

  // Cualquier ruta desconocida devuelve el index (captive portal / SPA)
  server.onNotFound(handleRoot);

  server.begin();
  Serial.println("Servidor HTTP iniciado.");
}