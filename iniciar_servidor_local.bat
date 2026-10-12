@echo off
title Servidor de Telemedicina ESP32
echo ==============================================================
echo    SISTEMA DE TELEMEDICINA - SERVIDOR CLINICO LOCAL / CLOUD
echo ==============================================================
echo.
echo Verificando dependencias...
python -m pip install -r requirements.txt --quiet
echo.
echo Obteniendo la direccion IP local para conectar el ESP32:
for /f "tokens=4" %%a in ('route print^|find " 0.0.0.0"') do (
    echo Tu IP local es: %%a
    echo El ESP32 puede conectarse a: http://%%a:5000/api/datos
    goto :ip_encontrada
)
:ip_encontrada
echo.
echo Iniciando servidor en el puerto 5000...
echo Presiona Ctrl+C para detener el servidor.
echo ==============================================================
python servidor.py
pause
