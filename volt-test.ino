// Archivo con función de arranque

// Función de contacto invertida

/********************************
    Bibliotecas requeridas:
********************************/
#include <Wire.h>
#include <Servo.h>
#include <LiquidCrystal_I2C.h>

// Configuración del display I2C
LiquidCrystal_I2C lcd(0x27, 16, 2); // Realidad

// Definición de pines
const int ledCorte = 2, ledMotorOn = 3, ledArranque = 4, ledFan = 5, ledWaiting = 6, ledTransfer = 7;
const int releContacto = 8, releFan = 10, releTransfer = 11;
const int releStarter = 12, ledFallo = 1;
// const int monitorArranque = A2; // Eliminado
const int detectorCorte = 13;
const int buzzer = 0;
const int sensorPCB = A1;   // Pin analógico para el sensor de PCB
const int sensorAuxiliar = A2;  // Pin analógico para el sensor auxiliar SIN USO

Servo servoChoke;  // Crear objeto Servo para controlar el choke

// Sensores TEMP
const float vout0 = 400; // Valor de salida del sensor a 0 grados Celsius
const float tc = 19.50; // Coeficiente de temperatura del sensor
float vout;

// Variables de estado
bool generadorEnMarcha = false;
bool luzPrevia = true; // Asumimos que inicialmente hay luz
int intentosArranque = 0;
int arranqueRestart = 0;
float voltajeBateria = 0.0;

unsigned long tiempoInicioGenerador = 0; // Almacenar el tiempo de inicio del generador

void setup() {
  pinMode(ledCorte, OUTPUT);
  pinMode(ledMotorOn, OUTPUT);
  pinMode(ledArranque, OUTPUT);
  pinMode(ledFan, OUTPUT);
  pinMode(ledWaiting, OUTPUT);
  pinMode(ledTransfer, OUTPUT);
  pinMode(ledFallo, OUTPUT);
  
  pinMode(releContacto, OUTPUT);
  pinMode(releFan, OUTPUT);
  pinMode(releTransfer, OUTPUT);
  pinMode(releStarter, OUTPUT);
  // pinMode(monitorArranque, INPUT); // Eliminado
  pinMode(detectorCorte, INPUT);
  pinMode(buzzer, OUTPUT);
  
  digitalWrite(ledWaiting, HIGH); // LED WAITING encendido inicialmente

  // Iniciar LCD
  lcd.init();
  lcd.backlight();
  servoChoke.attach(9);  // Adjuntar el servo al pin 9
}

void loop() {
  voltajeBateria = ((analogRead(A0) / 1023.0) * 5.0 * (22000.0 + 10000.0) / 10000.0) -2;
  lcd.setCursor(0, 1);
  lcd.print("                ");
  lcd.setCursor(0, 1);
  lcd.print("Bateria: ");
  lcd.print(voltajeBateria);
  lcd.print("V");

  // Si una variable tiene diferencia negativa de 2v o mas, enceder el led de starter, cuando vuelva a subir 1,5 voltios apagar el led y enceder el led engOn
  // Primero guardar el voltaje actual
  double voltajeAnterior = voltajeBateria;
  // Despues, si el voltaje baja 2v o mas, encender el led de starter y en un tiempo maximo de 5 segundos, esperar a que el voltaje suba 1,5v
  if (voltajeAnterior - voltajeBateria >= 2) {
    digitalWrite(ledArranque, HIGH);
    unsigned long tiempoInicio = millis();
    while (millis() - tiempoInicio < 5000) {
      voltajeBateria = ((analogRead(A0) / 1023.0) * 5.0 * (22000.0 + 10000.0) / 10000.0) -2;
      if (voltajeBateria - voltajeAnterior >= 1.5) {
        digitalWrite(ledFallo, LOW);
        digitalWrite(ledMotorOn, HIGH);
        digitalWrite(ledArranque, LOW);
        break;
      }
    }
  }
  
  delay(50);
}

// Función para emitir pitidos
void beep(int pin, int frequency, int duration, int pause) {
  tone(pin, frequency, duration);
  delay(duration + pause);
  noTone(pin);
}

// Funciones específicas para diferentes tipos de avisos
void beepInfo() {
  beep(buzzer, 1000, 200, 100);
}

void beepWarning() {
  for (int i = 0; i < 2; i++) {
    beep(buzzer, 1500, 200, 100);
  }
}

void beepError() {
  for (int i = 0; i < 3; i++) {
    beep(buzzer, 2000, 500, 300);
  }
}