#include "wifi_manager.h"
#include <Preferences.h>

// ---- Constantes de configuración AP ----
static const char* AP_SSID     = "ESP32_Config";
static const char* AP_PASS     = "";       // AP abierto; pon una contraseña si lo deseas
static const byte  DNS_PORT    = 53;

// ---- Definición de variables globales ----
String    savedSSID = "";
String    savedPass = "";
DNSServer dnsServer;

// ---- Objeto de preferencias (solo usado en este módulo) ----
static Preferences preferences;

// ------------------------------------------------------------------
// Credenciales
// ------------------------------------------------------------------

void saveCredentials(const String &ssid, const String &pass) {
  preferences.begin("wifi", false);
  preferences.putString("ssid", ssid);
  preferences.putString("pass", pass);
  preferences.end();
  savedSSID = ssid;
  savedPass = pass;
  Serial.println("Credenciales guardadas.");
}

void clearCredentials() {
  preferences.begin("wifi", false);
  preferences.remove("ssid");
  preferences.remove("pass");
  preferences.end();
  savedSSID = "";
  savedPass = "";
  Serial.println("Credenciales borradas.");
}

void loadCredentials() {
  preferences.begin("wifi", true);
  savedSSID = preferences.getString("ssid", "");
  savedPass = preferences.getString("pass", "");
  preferences.end();

  if (savedSSID != "") {
    Serial.print("Credencial cargada: ");
    Serial.println(savedSSID);
  } else {
    Serial.println("No hay credenciales guardadas.");
  }
}

// ------------------------------------------------------------------
// Conexión
// ------------------------------------------------------------------

void startAPMode() {
  Serial.println("Iniciando AP modo de configuración...");
  WiFi.mode(WIFI_AP);
  WiFi.softAP(AP_SSID, AP_PASS);

  IPAddress myIP = WiFi.softAPIP();
  Serial.print("AP IP: ");
  Serial.println(myIP);

  // Redirige todos los dominios al ESP (captive portal)
  dnsServer.start(DNS_PORT, "*", myIP);
}

bool connectToSavedWiFi(unsigned long timeoutMs) {
  if (savedSSID == "") return false;

  Serial.printf("Conectando a %s ...\n", savedSSID.c_str());
  WiFi.mode(WIFI_STA);
  WiFi.begin(savedSSID.c_str(), savedPass.c_str());

  unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED && (millis() - start) < timeoutMs) {
    delay(250);
    Serial.print(".");
  }
  Serial.println();

  if (WiFi.status() == WL_CONNECTED) {
    Serial.print("Conectado! IP: ");
    Serial.println(WiFi.localIP());
    return true;
  }

  Serial.println("No se pudo conectar con credenciales guardadas.");
  return false;
}