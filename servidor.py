import csv
from datetime import datetime
import math
import os
from threading import Lock
from flask import Flask, jsonify, render_template, request, send_file

app = Flask(__name__)
lock = Lock()

BASE_DIR = os.path.dirname(os.path.abspath(__file__))
ARCHIVO_CSV = os.path.join(BASE_DIR, "mediciones.csv")

# Estado en memoria con la última lectura y diagnósticos
lecturas = {
    "temperatura": None,
    "bpm": None,
    "spo2": None,
    "actualizado": None,
    "estado_temp": "Sin datos",
    "estado_bpm": "Sin datos",
    "estado_spo2": "Sin datos",
    "nivel_alerta": "normal"  # 'normal', 'alerta', 'critico'
}

def diagnosticar(temp, bpm, spo2):
    est_temp = "Sin datos"
    est_bpm = "Sin datos"
    est_spo2 = "Sin datos"
    nivel = "normal"

    if temp is not None:
        if temp < 35.0:
            est_temp = "Hipotermia"
            nivel = "alerta"
        elif 35.0 <= temp <= 37.5:
            est_temp = "Normotermia"
        elif 37.5 < temp <= 38.3:
            est_temp = "Febrícula"
            nivel = "alerta"
        else:
            est_temp = "Fiebre alta"
            nivel = "critico"

    if bpm is not None:
        if bpm < 60:
            est_bpm = "Bradicardia"
            if nivel != "critico": nivel = "alerta"
        elif 60 <= bpm <= 100:
            est_bpm = "Ritmo normal"
        else:
            est_bpm = "Taquicardia"
            if nivel != "critico": nivel = "alerta"

    if spo2 is not None:
        if spo2 >= 95.0:
            est_spo2 = "Normoxemia"
        elif 90.0 <= spo2 < 95.0:
            est_spo2 = "Hipoxia leve"
            if nivel != "critico": nivel = "alerta"
        else:
            est_spo2 = "Hipoxia severa"
            nivel = "critico"

    return est_temp, est_bpm, est_spo2, nivel

def leer_ultimas_mediciones(limite=20):
    if not os.path.isfile(ARCHIVO_CSV) or os.path.getsize(ARCHIVO_CSV) == 0:
        return []
    
    filas = []
    try:
        with open(ARCHIVO_CSV, "r", encoding="utf-8-sig") as f:
            lector = csv.DictReader(f)
            for fila in lector:
                try:
                    filas.append({
                        "fecha": fila.get("Fecha", ""),
                        "hora": fila.get("Hora", ""),
                        "temperatura": float(fila["Temperatura_C"]) if fila.get("Temperatura_C") else None,
                        "bpm": float(fila["BPM"]) if fila.get("BPM") else None,
                        "spo2": float(fila["SpO2_porcentaje"]) if fila.get("SpO2_porcentaje") else None
                    })
                except (ValueError, KeyError):
                    continue
    except Exception as e:
        print("Error al leer historial CSV:", e)
        return []

    return filas[-limite:]

@app.after_request
def agregar_cabeceras(response):
    response.headers["Access-Control-Allow-Origin"] = "*"
    response.headers["Access-Control-Allow-Headers"] = "Content-Type"
    response.headers["Access-Control-Allow-Methods"] = "GET, POST, OPTIONS"
    return response

@app.get("/")
def inicio():
    return render_template("index.html")

@app.get("/api/datos")
def consultar():
    with lock:
        return jsonify(lecturas.copy())

@app.get("/api/historial")
def historial():
    with lock:
        datos = leer_ultimas_mediciones(25)
        return jsonify(datos)

@app.get("/descargar-csv")
def descargar_csv():
    if not os.path.isfile(ARCHIVO_CSV):
        return jsonify(error="No hay archivo de mediciones disponible"), 404
    return send_file(
        ARCHIVO_CSV,
        mimetype="text/csv",
        as_attachment=True,
        download_name=f"mediciones_telemedicina_{datetime.now().strftime('%Y%m%d_%H%M%S')}.csv"
    )

@app.post("/api/datos")
def recibir():
    datos = request.get_json(silent=True)

    if not isinstance(datos, dict):
        return jsonify(error="Se requiere objeto JSON"), 400

    try:
        nuevos = {}
        for clave in ("temperatura", "bpm", "spo2"):
            valor = datos.get(clave)
            if valor is None or valor == "":
                nuevos[clave] = None
            else:
                numero = float(valor)
                if not math.isfinite(numero):
                    raise ValueError("Valor no finito")
                nuevos[clave] = round(numero, 2)

    except (ValueError, TypeError):
        return jsonify(error="Valores numéricos inválidos"), 400

    ahora = datetime.now()
    est_temp, est_bpm, est_spo2, nivel = diagnosticar(
        nuevos["temperatura"], nuevos["bpm"], nuevos["spo2"]
    )

    with lock:
        existe = os.path.isfile(ARCHIVO_CSV)

        with open(ARCHIVO_CSV, "a", newline="", encoding="utf-8-sig") as archivo:
            escritor = csv.writer(archivo)

            if not existe or os.path.getsize(ARCHIVO_CSV) == 0:
                escritor.writerow([
                    "Fecha", "Hora",
                    "Temperatura_C",
                    "BPM", "SpO2_porcentaje"
                ])

            escritor.writerow([
                ahora.strftime("%Y-%m-%d"),
                ahora.strftime("%H:%M:%S"),
                nuevos["temperatura"] if nuevos["temperatura"] is not None else "",
                nuevos["bpm"] if nuevos["bpm"] is not None else "",
                nuevos["spo2"] if nuevos["spo2"] is not None else ""
            ])

        lecturas.update(nuevos)
        lecturas["actualizado"] = ahora.strftime("%H:%M:%S")
        lecturas["estado_temp"] = est_temp
        lecturas["estado_bpm"] = est_bpm
        lecturas["estado_spo2"] = est_spo2
        lecturas["nivel_alerta"] = nivel

    print(f"[{ahora.strftime('%H:%M:%S')}] Telemetría recibida:", nuevos, flush=True)
    return jsonify(ok=True)

if __name__ == "__main__":
    puerto = int(os.environ.get("PORT", 5000))
    print(f"Iniciando servidor de Telemedicina en http://0.0.0.0:{puerto}")
    app.run(host="0.0.0.0", port=puerto, debug=False)
