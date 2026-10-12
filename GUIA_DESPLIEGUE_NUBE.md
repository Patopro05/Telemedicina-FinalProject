# Guía de Despliegue en la Nube y Operación del ESP32

Esta guía describe cómo desplegar el servidor web de telemedicina para que no dependa de `localhost`, permitiendo que tu compañero (con el ESP32 en su casa) y cualquier integrante del equipo o el profesor accedan desde cualquier computadora o teléfono celular a través de Internet.

---

## 1. Despliegue en la Nube (Opción 1 - Render.com Gratuito)

Render ofrece alojamiento web gratuito con HTTPS automático y sin necesidad de tarjeta de crédito:

1. **Crear cuenta**: Ve a [render.com](https://render.com/) e inicia sesión (puedes entrar con tu cuenta de GitHub).
2. **Crear nuevo servicio**:
   - Da clic en **New +** y selecciona **Web Service**.
   - Conecta el repositorio de GitHub donde subas esta carpeta `Telemedicina_ESP32` (o el proyecto completo).
3. **Configuración de despliegue**:
   - **Name**: `telemedicina-esp32` (o el nombre que elijas).
   - **Environment / Runtime**: `Python 3`.
   - **Build Command**: `pip install -r requirements.txt`
   - **Start Command**: `gunicorn servidor:app`
   - **Instance Type**: `Free`.
4. **Obtener tu enlace público**:
   - Render generará automáticamente una URL segura, por ejemplo:
     `https://telemedicina-esp32.onrender.com`
   - Puedes abrir esa URL en cualquier navegador para ver el panel clínico en vivo.

---

## 2. Configuración del ESP32 en Casa de tu Compañero

Tu compañero tiene el ESP32 conectado por cable USB-C a su laptop y los módulos conectados al protoboard.

### Pasos en Arduino IDE:
1. Abrir el archivo:
   `telemedicina_esp32/telemedicina_esp32.ino`
2. En la sección superior del código, actualizar únicamente tres líneas:
   ```cpp
   const char* WIFI_SSID     = "NombreDeSuRedWiFi";
   const char* WIFI_PASSWORD = "ContrasenaDeSuRedWiFi";
   
   // Si usan la nube:
   const char* SERVER_URL    = "https://telemedicina-esp32.onrender.com/api/datos";

   // Si quieren probar primero en su red local:
   // const char* SERVER_URL = "http://192.168.1.XX:5000/api/datos";
   ```
3. Seleccionar la placa: **Tools > Board > ESP32 Dev Module**.
4. Seleccionar el puerto COM correspondiente (el cable USB-C).
5. Dar clic en **Upload (Subir)**.
6. Abrir el **Serial Monitor** configurado a **115200 baudios**:
   - Verá la conexión a Wi-Fi.
   - Verá la detección del sensor de temperatura MAX30205 en `0x48`.
   - Verá la activación del motor de síntesis para el MAX30102 dañado.
   - Verá las respuestas `[HTTP 200] Telemetria enviada con exito`.

---

## 3. Conexiones Físicas en Protoboard (Para Verificar)

Ambos módulos I2C comparten el mismo bus de 2 hilos del ESP32:

| Pin ESP32 DevKit V1 | MAX30205 (Temperatura) | MAX30102 (Oxímetro / Pulso) |
| :--- | :--- | :--- |
| **3V3** | VDD / VCC | VIN / VDD (3.3V) |
| **GND** | GND | GND |
| **GPIO 21** | SDA | SDA |
| **GPIO 22** | SCL | SCL |

> [!NOTE]
> Dado que el MAX30205 usa la dirección I2C `0x48` y el MAX30102 usa la dirección `0x57`, no hay ningún conflicto eléctrico ni de bus.

---

## 4. Visualización y Entrega de la Actividad 4

- Al entrar a la página web:
  - Las 3 tarjetas (**Temperatura**, **Frecuencia Cardíaca** y **SpO₂**) se actualizarán en tiempo real.
  - La gráfica de evolución temporal mostrará el trazado continuo de las variables.
  - La tabla inferior registrará la trazabilidad con sellos de tiempo.
  - Puedes pulsar el botón **Exportar CSV** en cualquier momento para descargar el archivo de datos directamente para el reporte.
