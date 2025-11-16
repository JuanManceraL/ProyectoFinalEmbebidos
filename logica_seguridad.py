import time
import serial
import paho.mqtt.client as mqtt

MQTT_TOPIC = 'seguridad'     # Tema MQTT

Estado = 1 #0-Prendido      1-Apagado       2-Alarma
claves = {
    "PASSWORD":"5A8C",
    "ACTIVATE":"A12#"
}

mensajes = {
    0: f",0,Alarma Desactivada",
    1: f",1,Contraseña Errónea",
    2: f",2,Alarma Activada",
    3: f",3,Exceso de contraseñas incorrectas",
    4: f",4,🔔 Alerta 🔔",
    5: f",5,Movimiento Detectado"
}

cantErrores = 0
alarma_enviada = 0

cant_max_errores = 3
tiempo_max = 5  #Segundos de espera
tiempo_max = 5  #Segundos de espera
tiempo_inicio = 0
tiempo_transcurrido = 0

def ingreso_tecla(tecla, mqtt_client):
    global KeyCode, Estado, cantErrores, tiempo_inicio, tiempo_transcurrido, alarma_enviada
    KeyCode += tecla
    print(F"x:{tecla}  KeyCode:{KeyCode}")
    if len(KeyCode)>=4:
        print(f"KeyCODE: {KeyCode}")
        if(Estado == 0):    #0-Prendido
            if (KeyCode == claves["PASSWORD"]):
                print("Desactivar alarma")
                Estado = 1
                cantErrores = 0
                mqtt_client.publish(MQTT_TOPIC, mensajes[0])
            else:
                cantErrores+=1
                print(f"Clave ingresada erronea, cant errores: {cantErrores}")
                mqtt_client.publish(MQTT_TOPIC, mensajes[1])
        elif(Estado == 1):    #1-Apagado
            if (KeyCode == claves["ACTIVATE"]):
                print("Prendiendo Alarma") #Deberia haber un timer para salir
                Estado = 0
                tiempo_inicio = time.time()
                tiempo_transcurrido = 0
                mqtt_client.publish(MQTT_TOPIC, mensajes[2])
            else:
                print(f"Comando desconocido")
        elif(Estado == 2):    #2-Alarma
            if (KeyCode == claves["PASSWORD"]):
                print("Desactivando alarma")
                mqtt_client.publish(MQTT_TOPIC, mensajes[0])
                Estado = 1
                cantErrores = 0
            else:
                print(f"Error, contraseña invalida {KeyCode}")
            
        if(cantErrores >= cant_max_errores):
            Estado=2
            alarma_enviada = 0
            mqtt_client.publish(MQTT_TOPIC, mensajes[3])
            #tiempo_inicio = time.time()  #Inicia temporizador de tolerancia para ingresar contraseña
            #tiempo_transcurrido = 0

        KeyCode = ""
        print(f"Estado actual:{Estado}")
        #ser.write(b'Hello from Python\n')
        #Escribir por mosquitto
        return
    
def logica_seguridad(ser, mqtt_client):
    global Estado, tiempo_inicio, tiempo_transcurrido, alarma_enviada
    #Calcular tiempo transcurrido si está en espera
    if (tiempo_transcurrido < tiempo_max+0.5):
        if (Estado == 0 or Estado == 2):
            #print(f"Contando tiempo :), llevo:{tiempo_transcurrido}")
            tiempo_transcurrido = time.time() - tiempo_inicio
    else:
        if (Estado == 2 and alarma_enviada == 0):
            print("Alarma encendida Wee dooh 🔔")
            mqtt_client.publish(MQTT_TOPIC, mensajes[4])
            ser.write(b'1\n')
            alarma_enviada = 1

    return

def movimiento_detectado(mqtt_client):
    global Estado, tiempo_inicio, tiempo_transcurrido
    #print(f"Detecto algo, el estado es:{Estado}, y el tiempo transcurrido es {tiempo_transcurrido}")
    if(Estado == 0 and tiempo_transcurrido>=tiempo_max): #Si terminó el tiempo de tolerancia y detecta un movimiento
        #print("Detecto tiempo fuera de :(")
        mqtt_client.publish(MQTT_TOPIC, mensajes[5])
        Estado = 2 #Pasar a estado alarma
        tiempo_inicio = time.time()  #Inicia temporizador de tolerancia para ingresar contraseña
        tiempo_transcurrido = 0
        alarma_enviada = 0