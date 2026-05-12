#pragma once
#include <Arduino.h>
#include <WebServer.h>

// ---- Variable global accesible desde main ----
extern WebServer server;
extern bool shouldRestartAfterConnect;

// ---- Registra todas las rutas y arranca el servidor ----
void setupRoutes();

// ---- Handlers individuales (declarados por si se necesitan externamente) ----
void handleRoot();
void handleScan();
void handleConnect();
void handleStatus();
void handleForget();