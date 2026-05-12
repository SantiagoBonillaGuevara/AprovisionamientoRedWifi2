#include <Arduino.h>
#include "wifi_manager.h"
#include "web_server.h"

// Intervalo de reintento de reconexión (ms)
static const unsigned long RECONNECT_INTERVAL = 10000;

void setup() {
  Serial.begin(115200);
  delay(200);

  // 1. Cargar credenciales guardadas en flash
  loadCredentials();

  // 2. Intentar conectar; si falla o no hay credenciales, iniciar AP
  bool connected = false;
  if (savedSSID != "") {
    connected = connectToSavedWiFi(8000);
  }

  if (!connected) {
    startAPMode();
  } else {
    Serial.println("Modo STA operativo.");
  }

  // 3. Registrar rutas y arrancar servidor HTTP
  setupRoutes();
}

void loop() {
  // Atender peticiones DNS (captive portal) y HTTP
  dnsServer.processNextRequest();
  server.handleClient();

  // Reiniciar si se conectó vía /connect (limpia el estado del AP)
  if (shouldRestartAfterConnect) {
    Serial.println("Reiniciando para completar la conexión...");
    delay(1000);
    ESP.restart();
  }

  // Reconexión automática si se perdió la conexión en modo STA
  static unsigned long lastReconnectAttempt = 0;
  if (WiFi.status() != WL_CONNECTED && savedSSID != "") {
    if (millis() - lastReconnectAttempt > RECONNECT_INTERVAL) {
      lastReconnectAttempt = millis();
      Serial.println("Intentando reconectar a WiFi guardada...");
      if (connectToSavedWiFi(8000)) {
        Serial.println("Reconexión exitosa.");
      } else {
        Serial.println("Fallo reconexión: vuelvo a AP para reconfiguración.");
        startAPMode();
      }
    }
  }

  delay(2);
}