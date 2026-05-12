#pragma once
#include <Arduino.h>
#include <WiFi.h>
#include <DNSServer.h>

// ---- Variables globales accesibles desde otros módulos ----
extern String savedSSID;
extern String savedPass;
extern DNSServer dnsServer;

// ---- Funciones de credenciales ----
void saveCredentials(const String &ssid, const String &pass);
void clearCredentials();
void loadCredentials();

// ---- Funciones de conexión ----
void startAPMode();
bool connectToSavedWiFi(unsigned long timeoutMs = 10000);