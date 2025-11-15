import serial
import paho.mqtt.client as mqtt
import time
from logica_seguridad import logica_seguridad, ingreso_tecla, movimiento_detectado

# --- Configuración ---
SERIAL_PORT = '/dev/serial0'  # Puerto serial de la Raspberry Pi
BAUD_RATE = 19200              # Debe coincidir con tu HC-12 (9600 es el default)

MQTT_BROKER = 'localhost'     # O 'mosquitto' si usas el nombre del servicio Docker
MQTT_PORT = 1883
MQTT_TOPIC = 'sensor/pir'     # Tema MQTT para tu sensor HC-SR501

MQTT_USER = 'juan'
MQTT_PSSW = 'cont123'
# ---------------------

def on_connect(client, userdata, flags, rc):
    """Callback que se ejecuta al conectarse a MQTT."""
    if rc == 0:
        print(f"Conectado exitosamente al Broker MQTT en {MQTT_BROKER}")
    else:
        print(f"Error al conectar, código de retorno: {rc}")

def setup_mqtt_client():
    """Configura y conecta el cliente MQTT."""
    client = mqtt.Client()
    client.on_connect = on_connect
    try:
        client.username_pw_set(username=MQTT_USER, password=MQTT_PSSW)
        client.connect(MQTT_BROKER, MQTT_PORT, 60)
        client.loop_start()  # Inicia el loop en un hilo separado
        return client
    except Exception as e:
        print(f"Error al conectar con MQTT: {e}")
        return None

def main():
    print("Iniciando script de la capa de borde...")
    
    # 1. Conectar a Mosquitto (Paso 5 del proyecto)
    mqtt_client = setup_mqtt_client()
    if not mqtt_client:
        print("No se pudo conectar a Mosquitto. Saliendo.")
        return

    # 2. Conectar al puerto serial (HC-12) (Paso 3 del proyecto)
    try:
        ser = serial.Serial(SERIAL_PORT, BAUD_RATE, timeout=1)
        print(f"Escuchando en el puerto serial {SERIAL_PORT}...")
    except serial.SerialException as e:
        print(f"Error al abrir el puerto serial: {e}")
        mqtt_client.loop_stop()
        return

    #mqtt_client.publish(MQTT_TOPIC, "Prueba de tacos")
    #print(f"Publicando en {MQTT_TOPIC}, el dato: Prueba de tacos")

    # 3. Loop principal: Leer de HC-12 y publicar en MQTT
    try:
        while True:
            #Instrucciones main()
            logica_seguridad(ser, mqtt_client)

            #Dato leído
            if ser.in_waiting > 0:
                # Leer una línea completa (hasta el \n)
                linea_recibida = ser.readline()
                
                try:
                    # Decodificar de bytes a string (utf-8) y limpiar espacios
                    dato = linea_recibida.decode('utf-8').strip()

                    # Tu sensor HC-SR501 enviará "1" (movimiento) o "0" (sin movimiento)
                    if dato == "1" or dato == "0":
                        print(f"Dato recibido de RF: {dato}")
                        dato = float(dato)
                        # Publicar en Mosquitto (Paso 5)
                        mqtt_client.publish(MQTT_TOPIC, dato)
                        print(f"Dato '{dato}' publicado en el topic '{MQTT_TOPIC}'")

                        if  dato == 1:
                            movimiento_detectado(mqtt_client)

                    elif dato[0] == "T":
                        tecla = chr(int(dato[1:]))
                        print(f"Dato de teclado recibido por RF: {tecla}")
                        ingreso_tecla(tecla, mqtt_client)
                        # Publicar en Mosquitto (Paso 5) (Solo poner "Resultados")")
                    elif dato: # Imprimir si es otro dato (para depuración)
                        print(f"Dato no reconocido recibido: {dato}")

                except UnicodeDecodeError:
                    print(f"Error de decodificación. Dato crudo: {linea_recibida}")
            
            time.sleep(0.1) # Pequeña pausa para no saturar el CPU

    except KeyboardInterrupt:
        print("\nCerrando programa. (Ctrl+C presionado)")
    finally:
        # Limpieza al salir
        ser.close()
        mqtt_client.loop_stop()
        print("Puerto serial y conexión MQTT cerrados.")

if __name__ == "__main__":
    main()
