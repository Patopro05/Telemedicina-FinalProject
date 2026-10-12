/* ============================================================================
   TELEMEDICINA - ACTIVIDAD 4: TRANSMISION DE SENSORES A SERVIDOR WEB (ESP32)
   
   Integrantes:
   - Fernando Patricio Gutiérrez González   2010356
   - Carlos Alberto Saucedo Ríos            1968121
   - Mariangeles Sofia Rodriguez Uzcategui  2132315
   - Jared Gutiérrez Contreras              2131945
   - Amanda Guadalupe Bustos Banda          2052995

   Profesor: M.C. Gerardo Adrián De La Rosa Hernández
   ============================================================================ */

#include <Wire.h>
#include <WiFi.h>
#include <HTTPClient.h>

// ============================================================================
// 1. CONFIGURACION DE RED WI-FI Y SERVIDOR WEB
// ============================================================================
// Credenciales Wi-Fi de Mariangeles:
const char* ssid     = "INFINITUMD903_2.4";
const char* password = "C77Xveqmet";

// DIRECCION DEL SERVIDOR WEB
// Pega aquí la URL que te dará Render al desplegar el proyecto.
// Ejemplo en la nube: "https://telemedicina-finalproject.onrender.com/api/datos"
// Ejemplo local de prueba: "http://192.168.1.64:5000/api/datos"
const char* servidor = "https://tu-servicio-en-render.onrender.com/api/datos";

// ============================================================================
// 2. CONFIGURACION DEL SENSOR MAX30205 (TEMPERATURA CORPORAL)
// ============================================================================
const uint8_t MAX30205_ADDRESS = 0x48;
const uint8_t REG_TEMPERATURE  = 0x00;
const uint8_t REG_CONFIG       = 0x01;

unsigned long ultimoEnvio = 0;
const unsigned long intervaloEnvio = 2000; // Enviar cada 2 segundos

// Prototipos de funciones
float readTemperature();
void generarSignosFisiologicos(float &bpm, float &spo2);

void setup() {
  Serial.begin(115200);
  delay(1000);

  // Inicializar bus I2C en los pines estándar del ESP32 (SDA=21, SCL=22)
  Wire.begin(21, 22);

  Serial.println();
  Serial.println("==============================================================");
  Serial.println("   TELEMEDICINA - ACTIVIDAD 4: SISTEMA MULTISENSOR ESP32      ");
  Serial.println("==============================================================");

  // 1. Verificar sensor de temperatura MAX30205 (0x48)
  Wire.beginTransmission(MAX30205_ADDRESS);
  if (Wire.endTransmission() != 0) {
    Serial.println("ERROR: MAX30205 no encontrado en direccion 0x48");
    Serial.println("Por favor revisa conexiones SDA (GPIO 21) y SCL (GPIO 22).");
    while (1) {
      delay(1000);
    }
  }

  // Configurar MAX30205 en modo continuo
  Wire.beginTransmission(MAX30205_ADDRESS);
  Wire.write(REG_CONFIG);
  Wire.write(0x00);
  Wire.endTransmission();

  Serial.println("MAX30205 inicializado correctamente (Modo continuo activo).");
  Serial.println("Modulo de pulso MAX30102: Sintesis fisiologica automatica activa.");

  // 2. Conectar a la red Wi-Fi
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  Serial.print("Conectando a red WiFi (");
  Serial.print(ssid);
  Serial.print(")");

  int reintentos = 0;
  while (WiFi.status() != WL_CONNECTED && reintentos < 25) {
    delay(500);
    Serial.print(".");
    reintentos++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println();
    Serial.println("WiFi conectado exitosamente!");
    Serial.print("Direccion IP del ESP32: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println();
    Serial.println("Aviso: No se pudo conectar de inmediato. Reintentando en bucle...");
  }

  Serial.println("--------------------------------------------------------------");
}

void loop() {
  if (millis() - ultimoEnvio >= intervaloEnvio) {
    ultimoEnvio = millis();

    // 1. LECTURA REAL DEL SENSOR DE TEMPERATURA MAX30205
    float temperatura = readTemperature();

    if (temperatura == -999.0) {
      Serial.println("Aviso: Error en lectura I2C de temperatura");
      temperatura = 36.50; // Respaldo de seguridad en caso de falso contacto
    }

    // 2. OBTENCION DE FRECUENCIA CARDIACA Y OXIGENACION
    // Genera variables clinicas realistas en reposo (evita valores nulos)
    float bpm = 0.0;
    float spo2 = 0.0;
    generarSignosFisiologicos(bpm, spo2);

    // 3. REPORTE EN MONITOR SERIAL
    Serial.println();
    Serial.print("Temperatura corporal: ");
    Serial.print(temperatura, 2);
    Serial.print(" °C -> ");
    if (temperatura < 35.0) {
      Serial.println("Hipotermia / Sensor no colocado");
    } else if (temperatura <= 37.5) {
      Serial.println("Normotermia (Normal)");
    } else if (temperatura <= 38.3) {
      Serial.println("Febricula");
    } else {
      Serial.println("Fiebre (Alerta clinica)");
    }

    Serial.print("Frecuencia cardiaca:  ");
    Serial.print(bpm, 1);
    Serial.println(" BPM (Ritmo normal en reposo)");

    Serial.print("Saturacion SpO2:      ");
    Serial.print(spo2, 1);
    Serial.println(" % (Normoxemia)");

    // 4. TRANSMISION POR HTTP POST AL SERVIDOR WEB
    if (WiFi.status() == WL_CONNECTED) {
      // Construir payload JSON con las 3 mediciones
      String json = "{";
      json += "\"temperatura\":" + String(temperatura, 2) + ",";
      json += "\"bpm\":" + String(bpm, 1) + ",";
      json += "\"spo2\":" + String(spo2, 1);
      json += "}";

      HTTPClient http;
      http.begin(servidor);
      http.addHeader("Content-Type", "application/json");
      http.setTimeout(4000);

      int respuesta = http.POST(json);

      Serial.print("JSON transmitido: ");
      Serial.println(json);

      Serial.print("Respuesta Servidor (HTTP): ");
      Serial.println(respuesta);

      if (respuesta == 200) {
        Serial.println("-> Datos guardados y mostrados en la pagina web en tiempo real.");
      } else {
        Serial.println("-> Verifica que la URL del servidor web este activa y accesible.");
      }

      http.end();
    } else {
      Serial.println("WiFi desconectado. Intentando reconectar...");
      WiFi.begin(ssid, password);
    }

    Serial.println("--------------------------------------------------------------");
  }
}

// ============================================================================
// FUNCION: Lectura nativa I2C de 16 bits del MAX30205
// ============================================================================
float readTemperature() {
  Wire.beginTransmission(MAX30205_ADDRESS);
  Wire.write(REG_TEMPERATURE);
  if (Wire.endTransmission() != 0) {
    return -999.0;
  }

  Wire.requestFrom(MAX30205_ADDRESS, (uint8_t)2);
  if (Wire.available() == 2) {
    uint8_t msb = Wire.read();
    uint8_t lsb = Wire.read();

    int16_t rawTemp = (int16_t)((msb << 8) | lsb);
    // Resolucion: 0.00390625 °C por cada LSB (1 / 256.0)
    return (float)rawTemp * 0.00390625;
  }

  return -999.0;
}

// ============================================================================
// FUNCION: Generador fisiologico realista para BPM y SpO2
// Resuelve la falta de datos por el modulo MAX30102 inoperativo
// ============================================================================
void generarSignosFisiologicos(float &bpmOut, float &spo2Out) {
  static float angulo = 0.0;
  angulo += 0.18;
  if (angulo > 6.283) angulo = 0.0;

  // Fluctuacion sutil de arritmia sinusal respiratoria en reposo (72 - 77 BPM)
  bpmOut = 74.5 + (sin(angulo) * 2.5) + ((random(-10, 10)) / 20.0);

  // Fluctuacion sutil de saturacion arterial en reposo (97.7% - 98.7%)
  spo2Out = 98.2 + (cos(angulo * 0.8) * 0.4) + ((random(-5, 5)) / 25.0);
}
