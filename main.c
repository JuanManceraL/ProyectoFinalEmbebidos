/*
 * Archivo: main.c
 * Proyecto: Capa de Percepción (PIC16F877A + HC-SR501 + HC-12)
 * Compilador: XC8
 * Asume: Cristal de 4MHz
 */

#pragma config FOSC = HS  
#pragma config WDTE = OFF 
#pragma config PWRTE = OFF
#pragma config BOREN = ON
#pragma config LVP = OFF
#pragma config CPD = OFF
#pragma config WRT = OFF
#pragma config CP = OFF
#include <xc.h>
#include <stdio.h>
#include <stdlib.h>

// --- Configuración Básica ---
// ¡IMPORTANTE! Debes configurar los FUSES (bits de configuración)
// en las propiedades de tu proyecto en MPLAB X.
// Ejemplo: FOSC = XT, WDT = OFF, PWRTE = ON, LVP = OFF
#define _XTAL_FREQ 4000000 

// --- Pines ---
#define SENSOR_PIN PORTCbits.RC4  // Pin de entrada del sensor HC-SR501
#define SENSOR_TRIS TRISCbits.TRISC4 // Registro TRIS para ese pin
#define LED_C5 PORTCbits.RC5  // 
#define LED_TRIS TRISCbits.TRISC5 // LED verificacion del sensor
#define LED_C3 PORTCbits.RC3  // 
#define LEDA_TRIS TRISCbits.TRISC3 // LED verificacion del sensor

/* Teclado matricial 4x4 en PUERTO B */
//Lo cambie de pueto debido a que el teclado necesita resistencia pull-up  
#define FILA1 LATBbits.LATB0
#define FILA2 LATBbits.LATB1
#define FILA3 LATBbits.LATB2
#define FILA4 LATBbits.LATB3

#define COL1 PORTBbits.RB4
#define COL2 PORTBbits.RB5
#define COL3 PORTBbits.RB6
#define COL4 PORTBbits.RB7

// --- Prototipos de Funciones ---
void UART_Init(void);
void UART_Send_Char(char data);
void UART_Send_String(const char* str);
void configuracion(void);

void USART_TxChar(char data);
void USART_TxString(const char *str);

char teclado(void);
void temporizador(void);
void enviar(void);
void leeradc(void);
void ADC_Init(void);

char tecla_ant;

volatile unsigned char cont0_env;
volatile unsigned char cont1_sns;
volatile unsigned char cont2_sns_an;
volatile unsigned char cont4_adc;
volatile unsigned char tmrFlags;

volatile unsigned char sen_adc_8bits;

// --- Programa Principal ---
void main(void) {
    // 1. Inicializar Hardware
    configuracion();
    UART_Init();
    ADC_Init();
    
    cont0_env = 0;
    cont1_sns = 0;
    tecla_ant = 'a';

    // Pequeña espera para que todo se estabilice
    __delay_ms(500);

    // 2. Loop Principal (Capa de Percepción)
    while (1) {
        temporizador();
        leeradc();
        enviar();
        
        char tecla = teclado();
        if (tecla && tecla != tecla_ant) {
            char buffer[10];
            sprintf(buffer, "T%d\n", tecla);
            USART_TxString(buffer);
            
        }
        tecla_ant = tecla;
        
        
        // Intervalo de envío (Paso 5 del proyecto)
        // Ajusta este delay al "cierto intervalo de tiempo" que necesites
        // (Ejemplo: 1000ms = 1 segundo)
        __delay_ms(100); //1000 
        //QUITAR ESTO PARA QUEE VAYA MÁS RÁPIDO
    }
}

// --- Definición de Funciones ---

void configuracion(void) {
    // Configurar el pin del sensor (RC0) como entrada digital
    SENSOR_TRIS = 1; 
    LED_TRIS = 0;
    LED_C5 = 0;
    LEDA_TRIS = 0;
    LED_C3 = 0;
    // Configurar teclado 4x4 en PORTB
    TRISB = 0b11110000;     // RB0-RB3 salidas, RB4-RB7 entradas
    PORTB = 0x0F;            // Filas inicialmente en 1
    OPTION_REGbits.nRBPU = 0; // Habilitar resistencias pull-up internas
    
    // Configurar OPTION_REG
    OPTION_REGbits.T0CS = 0; // Bit 5: Timer0 usa reloj interno
    OPTION_REGbits.PSA  = 0; // Bit 3: Prescaler asignado a Timer0
    OPTION_REGbits.PS2  = 1; // Bits 2-0: Prescaler = 111 ? 1:256
    OPTION_REGbits.PS1  = 1;
    OPTION_REGbits.PS0  = 1;    
}

void variables(void){
    // Declaración de variables en direcciones específicas de RAM
    cont0_env = 0;
    cont1_sns = 0;
    cont2_sns_an = 0;
    cont4_adc = 20;
    tmrFlags = 0;

}

void UART_Init(void) {
    // --- Configuración para 9600 baudios con cristal de 4MHz ---
    
    // Configurar pines de EUSART
    TRISCbits.TRISC6 = 0; // TX como salida
    TRISCbits.TRISC7 = 1; // RX como entrada

    // Configurar registros de TX
    TXSTAbits.TXEN = 1;   // Habilitar transmisión
    TXSTAbits.SYNC = 0;   // Modo asíncrono
    TXSTAbits.BRGH = 1;   // Alta velocidad de baudios (para 9600 con 4MHz)

    // Configurar registros de RX
    RCSTAbits.SPEN = 1;   // Habilitar puerto serial
    RCSTAbits.CREN = 1;   // Habilitar recepción continua (no se usa aquí, pero es buena práctica)

    // Configurar generador de baudios
    // Fórmula: SPBRG = [FOSC / (16 * BaudRate)] - 1
    // SPBRG = [4,000,000 / (16 * 9600)] - 1 = 25.04 -> 25
    //SPBRG = 25;           
    SPBRG = 12; // Para Fosc = 4MHz y BRGH = 1 ? 19200 baudios
}

void ADC_Init(void) {
    // Velocidad de conversión
    ADCON1bits.ADCS2 = 0; 
    ADCON0bits.ADCS1 = 1;
    ADCON0bits.ADCS0 = 0;
    // Enciende módulo ADC
    ADCON0bits.ADON = 1; 
    // 0: 10 bits cargados a la izquierda 8MSB + 2LSB + 6ceros
    ADCON1bits.ADFM = 0; 
    // Seleccionando pines y V de referencia.
    // 0100: AN0, AN1, AN3. Vref+ = VDD. Vref- = VSS.
    ADCON1bits.PCFG3 = 0;
    ADCON1bits.PCFG2 = 1;  
    ADCON1bits.PCFG1 = 0;
    ADCON1bits.PCFG0 = 0;    
    // Inicializando con el canal 0 seleccionado
    ADCON0bits.CHS0 = 0;
    ADCON0bits.CHS0 = 0;
    ADCON0bits.CHS0 = 0;    
}

char teclado(void) {
    const char keys[4][4] = {
        {'1','2','3','A'},
        {'4','5','6','B'},
        {'7','8','9','C'},
        {'*','0','#','D'}
    };

    for (int fila = 0; fila < 4; fila++) {
        PORTB = 0x0F;                      // Todas las filas en 1
        PORTB &= ~(1 << fila);             // Activa fila en 0

        __delay_ms(10);                   // Anti-rebote

        if (!COL1) return keys[fila][0];
        if (!COL2) return keys[fila][1];
        if (!COL3) return keys[fila][2];
        if (!COL4) return keys[fila][3];
    }

    return 0; // Sin tecla presionada
}

void temporizador(void){
    if (INTCONbits.TMR0IF == 1){
        INTCONbits.TMR0IF = 0;
        cont0_env++;
        if (cont0_env > 15) {
            cont0_env = 0;
            tmrFlags |= (1<<0); // Activa el bit 0  
        }
        cont1_sns++;
        if (cont1_sns>40) {
            cont1_sns = 0;
             tmrFlags |= (1<<1); // Activa el bit 1  (enviar dato mov)          
        }
        cont2_sns_an++;
        if (cont2_sns_an>15) {
            cont2_sns_an = 0;
             tmrFlags |= (1<<2); // Activa el bit 2 (enviar dato adc)          
        }
        cont4_adc++;
        if (cont4_adc>15) {
            cont4_adc = 0;
                tmrFlags |= (1<<4); // Activa el bit 4    (leer adc)
        }
    }
}

void enviar(void){
    if (tmrFlags & (1)){
        tmrFlags &= ~(1); // Limpia el bit 0         
        
        if (SENSOR_PIN == 1) {
            // Movimiento detectado
            UART_Send_String("1\n"); // Envía "1" y un salto de línea
            USART_TxString("Mov Det\r\n");
            LED_C5 = 1;
            
        } else {
            // Sin movimiento
            UART_Send_String("0\n"); // Envía "0" y un salto de línea
            USART_TxString("Sn Mov\r\n");
            LED_C5 = 0;
        }
    }
    if (tmrFlags & (2)){
        tmrFlags &= ~(2); // Limpia el bit 2
        char buffer[20];
        sprintf(buffer, "A%d\n", sen_adc_8bits);
        USART_TxString(buffer);
    }
}

void leeradc(void) {
     if (tmrFlags & (1<<4)){
        tmrFlags &= ~(1<<4); // Limpia el bit 1
        ADCON0bits.GO_nDONE = 1;  
        ADCON0bits.CHS0 = 0; //Canal 0
        while(ADCON0bits.GO_nDONE);
        sen_adc_8bits = ADRESH; // Mapeo truncado
     }
}

void UART_Send_Char(char data) {
    while (!TXSTAbits.TRMT); // Espera a que el buffer de transmisión esté vacío
    TXREG = data;             // Carga el dato a enviar
}

void UART_Send_String(const char* str) {
    // Itera sobre el string y envía cada caracter
    while (*str) {
        UART_Send_Char(*str++);
    }
}

void USART_TxChar(char data) {
    while (!TXIF);
    TXREG = data;
}

void USART_TxString(const char *str) {
    while (*str){
        USART_TxChar(*str);
        str++;
    }
}
