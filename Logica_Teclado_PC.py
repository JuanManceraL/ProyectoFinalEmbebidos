

a = "Taco"
if a[0]=="T":
    # TODO: write code...
    print(a[1:])
    
rec = "79"
x = chr(int(rec))
print(x)

rec = "65"
x = chr(int(rec))
print(x)

Estado = 0 #0-Prendido      1-Apagado       2-Alarma        3-Espera
claves = {
    "PASSWORD":"5A8C",
    "ACTIVATE":"A12#"
}    

cantErrores = 0

KeyCode = ""

while(1):
    x = input()
    KeyCode += x
    print(F"x:{x}  KeyCode:{KeyCode}")
    if len(KeyCode)>=4:
        print(f"KeyCODE: {KeyCode}")
        if(Estado == 0):    #0-Prendido
            if (KeyCode == claves["PASSWORD"]):
                print("Desactivar alarma")
                Estado = 1
                cantErrores = 0
            else:
                cantErrores+=1
                print(f"Errores: {cantErrores}")
        elif(Estado == 1):    #1-Apagado
            if (KeyCode == claves["ACTIVATE"]):
                print("Prendiendo Alarma") #Deberia haber un timer para salir
                Estado = 0
            else:
                print(f"Comando desconocido")
        elif(Estado == 2 or Estado == 3):    #2-Alarma   3-Espera
            if (KeyCode == claves["PASSWORD"]):
                print("Desactivando")
                Estado = 1
                cantErrores = 0
            else:
                print(f"Error, contraseña invalida {KeyCode}")
        
        if(cantErrores >= 3):
            Estado=2
        KeyCode = ""
        print(f"Estado actual:{Estado}")
        #ser.write(b'Hello from Python\n')
        #Escribir por mosquitto