# ESP32 - Aprovisionamiento de Red WiFi
## Integrantes de trabajo
- Wilson Santiago Bonilla Guevara

Este proyecto implementa un **captive portal en un ESP32** que permite aprovisionar la conexión WiFi de manera sencilla.  
El dispositivo levanta un **Access Point (AP)** y un **servidor HTTP**, mostrando una página web de configuración donde el usuario puede:
- Escanear redes disponibles.
- Seleccionar un SSID.
- Enviar credenciales (SSID, contraseña).
- Guardarlas en memoria no volátil.
- Conectarse automáticamente a la red indicada.
- Consultar el estado de la conexión o borrar credenciales.

---

## 📝 Explicación breve del código

- Se usa la librería **WiFi.h** para manejar la conexión a redes.
- **WebServer** levanta un servidor HTTP en el puerto 80.
- **DNSServer** redirige todo tráfico al ESP32 (captive portal).
- **Preferences** guarda de forma persistente las credenciales WiFi (`ssid` y `pass`).
- Al final de la ejecución del codigo se imprime la ip asignada al ESP32.
- El flujo de arranque es:
  1. Revisar si existen credenciales guardadas.
  2. Intentar conexión a la red.
  3. Si falla, crear un AP abierto llamado `ESP32_Config`.
  4. Mostrar página web de configuración en `http://[IP_ESP32]/`.

---

## 🗂️ Arquitectura del código

El código fue refactorizado desde un único archivo `.ino` a una estructura modular en PlatformIO, dividida en los siguientes archivos dentro de `src/`:

```
src/
├── main.cpp          ← Punto de entrada: setup() y loop()
├── wifi_manager.h    ← Declaraciones: credenciales y conexión WiFi
├── wifi_manager.cpp  ← Implementación: Preferences, AP, STA, reconexión
├── web_server.h      ← Declaraciones: handlers HTTP y setupRoutes()
├── web_server.cpp    ← Implementación: endpoints REST y parseo JSON
└── html_page.h       ← HTML del captive portal embebido como string
```

### Responsabilidades por módulo

| Módulo | Responsabilidad |
|---|---|
| `main.cpp` | Orquesta el arranque y el loop principal. No contiene lógica de negocio. |
| `wifi_manager` | Manejo de credenciales (guardar/cargar/borrar con `Preferences`), conexión STA y modo AP con captive portal DNS. |
| `web_server` | Registro de rutas HTTP, handlers de cada endpoint y parseo manual de JSON. |
| `html_page.h` | Página web de configuración embebida como `inline String`. No requiere sistema de archivos (SPIFFS). |

### Variables globales compartidas

| Variable | Definida en | Usada en |
|---|---|---|
| `savedSSID`, `savedPass` | `wifi_manager.cpp` | `web_server.cpp`, `main.cpp` |
| `dnsServer` | `wifi_manager.cpp` | `main.cpp`, `web_server.cpp` |
| `server` | `web_server.cpp` | `main.cpp` |
| `shouldRestartAfterConnect` | `web_server.cpp` | `main.cpp` |

---

## Endpoints de la API

### `GET /scan`
- Escanea las redes WiFi disponibles y devuelve un array JSON con los SSID.  
- **Ejemplo de respuesta:**
  ```json
  [
    "Red1",
    "Red2",
    "Red3"
  ]
  ```

### `POST /connect`
- Intenta conectar a la red usando las credenciales recibidas en formato JSON.
- Guarda credenciales si la conexión es exitosa.

Request body:
  ```json
  {
    "ssid": "MiRed",
    "pass": "mi_contraseña"
  }
  ```

## Respuestas:

#### Éxito:
```json
{
  "success": true,
  "ip": "172.20.10.2"
}
```

#### Error:
```json
{
  "success": false,
  "message": "No se pudo conectar con esas credenciales"
}
```

### `GET /status`
- Devuelve el estado actual de la conexión.
- **Ejemplo de respuesta:**
```json
{
  "connected": true,
  "ssid": "MiRed",
  "ip": "172.20.10.2"
}
```

### `GET /forget`
- Borra las credenciales guardadas y reinicia el dispositivo.
- **Ejemplo de respuesta:**
```
Credenciales borradas. Reiniciando...
```

---

## Diagramas

### Diagrama de componentes
![diagramaDeComponentes](https://github.com/user-attachments/assets/39a2c1c7-9cd6-4ef2-90fa-6c2d14a8b0ae)

### Diagrama de estados
![DiagramaDeEstados](https://github.com/user-attachments/assets/e72f9932-9ffa-4ffb-b51e-0e7829f06a92)

### Diagrama de secuencias
![diagramaUmlDeSecuencia](https://github.com/user-attachments/assets/42fe364e-a9fb-4d18-8f88-087d5750e6ec)

---

## ❓ Preguntas de investigación

### 1. ¿Es posible conectarse a redes WiFi con seguridad PEAP Enterprise con el ESP32? ¿Qué se necesita?

**Sí, es posible.** El ESP32 soporta WPA2-Enterprise con PEAP de forma nativa a través del stack ESP-IDF, pero **únicamente en modo Station (STA)**, no como AP.

Para implementarlo se necesita:

- Las cabeceras `esp_wpa2.h` y `esp_wifi.h` (incluidas en el core de ESP-IDF / arduino-esp32).
- Los siguientes datos de la red corporativa:
  - **Identity** (usuario o dirección de correo institucional)
  - **Username** (puede coincidir con la identity)
  - **Password**
  - **Certificado CA del servidor RADIUS** (opcional pero recomendado)

El flujo de código es:

```cpp
#include "esp_wpa2.h"
#include <WiFi.h>

WiFi.disconnect(true);
esp_wifi_sta_wpa2_ent_set_identity((uint8_t *)EAP_IDENTITY, strlen(EAP_IDENTITY));
esp_wifi_sta_wpa2_ent_set_username((uint8_t *)EAP_USERNAME, strlen(EAP_USERNAME));
esp_wifi_sta_wpa2_ent_set_password((uint8_t *)EAP_PASSWORD, strlen(EAP_PASSWORD));
// Opcional: esp_wifi_sta_wpa2_ent_set_ca_cert(cert_pem, cert_len);
esp_wpa2_config_t config = WPA2_CONFIG_INIT_DEFAULT();
esp_wifi_sta_wpa2_ent_enable(&config);
WiFi.begin(ssid);
```

> ⚠️ La conexión PEAP puede tardar hasta 6 segundos. Si el certificado CA del servidor es muy grande (>20 KB), puede agotarse el heap disponible durante el handshake TLS. La implementación actual de este proyecto **no soporta PEAP** ya que usa `WiFi.begin(ssid, pass)` estándar (WPA2-PSK), pero podría extenderse con el módulo `wifi_manager.cpp` añadiendo una función `connectToEnterpriseWiFi()`.

---

### 2. ¿Cuántas conexiones/clientes simultáneos soporta la librería WebServer? ¿Qué alternativas hay?

La librería **WebServer** incluida en el core de arduino-esp32 es **síncrona y de un solo cliente a la vez**: atiende una petición, bloquea el loop hasta responder, y solo entonces acepta la siguiente. En la práctica esto equivale a **1 cliente activo simultáneo**, aunque el sistema operativo puede encolar algunas conexiones TCP a nivel de socket.

| Librería | Clientes simultáneos | Modelo | Dependencias extra |
|---|---|---|---|
| **WebServer** (usada en este proyecto) | ~1 (síncrono) | Bloqueante | Ninguna (incluida en el core) |
| **ESPAsyncWebServer** | Múltiples | Asíncrono (FreeRTOS) | `AsyncTCP` |
| **PsychicHttp** | Múltiples | Asíncrono (esp_http_server) | Ninguna extra |
| **esp_http_server** (IDF nativo) | Configurable | Asíncrono | Ninguna extra |

**ESPAsyncWebServer** es la alternativa más popular: permite manejar múltiples conexiones al mismo tiempo sin bloquear el loop principal, y soporta WebSockets para comunicación en tiempo real. La desventaja es que consume más RAM y requiere la librería `AsyncTCP`.

Para un captive portal de aprovisionamiento como este, **WebServer es suficiente** ya que se espera solo un usuario configurando el dispositivo a la vez.

---

### 3. Comparación de memoria Flash usada por esta implementación vs. el ejemplo "Basic" de la librería WiFiManager

La librería **WiFiManager (tzapu)** es una solución lista para usar pero más pesada porque incluye su propio servidor web, páginas HTML preconstruidas, lógica de escaneo y portal cautivo todo integrado. El ejemplo `Basic` de WiFiManager se reduce a:

```cpp
WiFiManager wifiManager;
wifiManager.autoConnect("AP-Name", "password");
```

Comparación aproximada de uso de Flash (valores representativos compilando para ESP32 con arduino-esp32):

| Implementación | Flash aprox. | RAM aprox. | Observaciones |
|---|---|---|---|
| **Este proyecto (v2, modular)** | ~700–800 KB | ~40–50 KB | Usa `Preferences`, `WebServer`, `DNSServer`. Sin dependencias externas. |
| **WiFiManager Basic** | ~950 KB – 1.1 MB | ~55–70 KB | Incluye ArduinoJson, HTML embebido propio, lógica de portal más completa. |

> Los valores exactos varían según la versión del core de ESP32 y las optimizaciones del compilador. Para obtener los números reales de tu proyecto, PlatformIO muestra el uso de Flash y RAM al final de cada compilación.

**Ventajas de este proyecto frente a WiFiManager:**
- Menor tamaño en Flash al no incluir dependencias externas como ArduinoJson.
- Control total sobre la interfaz web y los endpoints REST.
- La lógica de credenciales usa `Preferences` (NVS) directamente, sin depender de SPIFFS ni EEPROM emulada.

**Ventajas de WiFiManager:**
- Menor código a mantener (3 líneas para un portal funcional).
- Integración directa con ArduinoOTA.
- Soporte nativo para parámetros personalizados adicionales.

---

## Conversaciones con ChatGPT
Parte de la documentación y el diseño de la API fueron asistidos con ChatGPT.

[Ir a conversacion con chatGPT](https://chatgpt.com/share/68dd807a-ae50-8011-96c7-04e7e8ea28c2)

## GitHub Pages
Se puede desplegar la documentación de la API (OpenAPI/Swagger UI) en GitHub Pages para visualización pública.

[Ir a GitHub Pages](https://santiagobonillaguevara.github.io/aprovisionamientoRedWifi/)

## Recursos
- Código fuente: `src/`
- Colección Postman: `ESP32-Aprovicionamiento_de_red_WIFI.postman_collection.json`
- OpenAPI Spec: `docs/openAPI.yaml`