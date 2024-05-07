#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// Configuración del display I2C
LiquidCrystal_I2C lcd(0x20, 16, 2); // Ajusta la dirección de tu display si es diferente

// Definición de pines
const int ledRojo = 2, ledVerde = 3, ledArranque = 4, ledFan = 5, ledWaiting = 6, ledTodoOk = 7;
const int releLlave = 8, releChoke = 9, releVentilacion = 10, releTransfer = 11;
const int releMotorArranque = A1, monitorArranque = A2;
const int sensorLuz = 13;
const int buzzer = 12;

// Variables de estado
bool generadorEnMarcha = false;
bool luzPrevia = true; // Asumimos que inicialmente hay luz
int intentosArranque = 0;
int arranqueRestart = 0;

unsigned long tiempoInicioGenerador = 0; // Almacenar el tiempo de inicio del generador

void setup() {
  pinMode(ledRojo, OUTPUT);
  pinMode(ledVerde, OUTPUT);
  pinMode(ledArranque, OUTPUT);
  pinMode(ledFan, OUTPUT);
  pinMode(ledWaiting, OUTPUT);
  pinMode(ledTodoOk, OUTPUT);
  
  pinMode(releLlave, OUTPUT);
  pinMode(releChoke, OUTPUT);
  pinMode(releVentilacion, OUTPUT);
  pinMode(releTransfer, OUTPUT);
  pinMode(releMotorArranque, OUTPUT);
  
  pinMode(monitorArranque, INPUT);
  pinMode(sensorLuz, INPUT);
  pinMode(buzzer, OUTPUT);
  
  digitalWrite(ledWaiting, HIGH); // LED WAITING encendido inicialmente
  
  // Iniciar LCD
  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("Esperando corte");
  lcd.setCursor(0, 1);
  lcd.print("de suministro...");
}

void loop() { ///////////////REVISAR
  bool luzActual = digitalRead(sensorLuz);
  static unsigned long luzVueltaTiempo = 0; // Almacenar cuándo la luz regresó por primera vez
  bool luzEstable = false;

  // Detectar corte de luz
  if (!luzActual && luzPrevia) {
    if(arranqueRestart > 0) {
    alertaCorteLuz();
    iniciarGenerador();
    arranqueRestart++;
    }
    luzVueltaTiempo = 0; // Reiniciar el contador porque la luz se fue
  }

  // Detectar el retorno de la luz
  if (luzActual && !luzPrevia) {
    // Marcar el tiempo inicial cuando la luz regresa
    luzVueltaTiempo = millis();
  }

  // Verificar si la luz ha vuelto y se ha mantenido por al menos 5 segundos
  if (luzActual && luzVueltaTiempo != 0 && (millis() - luzVueltaTiempo > 5000)) {
    luzEstable = true;
  }

  // Restablecimiento de la luz si ha estado estable durante al menos 5 segundos
  if (luzEstable && luzPrevia) {
    restablecerSistema();
    luzVueltaTiempo = 0; // Reiniciar el tiempo de restablecimiento de luz
  }

  luzPrevia = luzActual; // Actualizar el estado de la luz para la próxima iteración
}


void alertaCorteLuz() {
  digitalWrite(ledRojo, HIGH);
  beep(buzzer, 3, 200); // 3 pitidos cortos
  lcd.clear();
  lcd.print("Corte detectado!");
}

void iniciarGenerador() {
  if (!generadorEnMarcha) {
    digitalWrite(ledWaiting, LOW); // Apagar LED WAITING
    digitalWrite(releLlave, HIGH); // Activar contacto
    lcd.setCursor(0, 1);
    lcd.print("Preparando...");
    delay(1000); // Esperar 1 segundo
    lcd.setCursor(0, 1);
    lcd.print("                ");
    lcd.setCursor(0, 1);
    lcd.print("Esperando...");
    delay(500);
    digitalWrite(releChoke, HIGH);
    beep(buzzer, 1, 200); // 1 pitido corto para Aire
    lcd.setCursor(0, 1);
    lcd.print("Aire cerrado");
    intentarArrancar();
  }
}

void intentarArrancar() {
  intentosArranque = 0;
  bool arranqueExitoso = false;

  while (intentosArranque < 3 && !arranqueExitoso) {
    digitalWrite(releMotorArranque, HIGH); // Activa el motor de arranque
    digitalWrite(ledArranque, HIGH);
    lcd.setCursor(0, 1);
    lcd.print("Arrancando motor...");
    
    unsigned long startTime = millis(); // Tiempo inicial
    while (millis() - startTime < 5000) { // Período de 5 segundos para intentar arrancar
      if (digitalRead(monitorArranque)) {
        arranqueExitoso = true;
        break; // Sale del ciclo si el motor arranca
      }
      delay(100); // Pequeña pausa para no saturar la lectura del pin
    }

    digitalWrite(releMotorArranque, LOW); // Desactiva el motor de arranque
    digitalWrite(ledArranque, LOW);

    if (!arranqueExitoso) {
      intentosArranque++;
      lcd.setCursor(0, 1);
      lcd.print("                ");
      lcd.setCursor(0, 1);
      lcd.print("Fallo arranque ");
      lcd.setCursor(15,1);
      lcd.print(intentosArranque);
      delay(5000); // Espera antes del próximo intento
    }
  }

  if (arranqueExitoso) {
    operacionNormal();
  } else {
    estadoError(); // Va al estado de error si no arranca después de 3 intentos
  }
}


void operacionNormal() {
  digitalWrite(ledArranque, HIGH);
  lcd.setCursor(0, 1);
  lcd.print("                ");
  lcd.setCursor(0, 1);
  lcd.print("Motor arrancado");
  digitalWrite(ledVerde, HIGH);
  digitalWrite(releChoke, LOW); // Abrir choke
  digitalWrite(ledArranque, LOW);
  lcd.setCursor(0, 1);
  lcd.print("                ");
  beep(buzzer, 1, 200); // 1 pitido corto para Choke
  lcd.setCursor(0, 1);
  lcd.print("Aire abierto");
  delay(300);
  
  beep(buzzer, 2, 200); // Confirmación de arranque
  lcd.setCursor(0, 1);
  lcd.print("                ");
  lcd.setCursor(0, 1);
  lcd.print("Motor ON");
  digitalWrite(releVentilacion, HIGH);
  digitalWrite(ledFan, HIGH);
  lcd.setCursor(0, 1);
  lcd.print("                ");
  lcd.setCursor(0, 1);
  lcd.print("Ventilacion ON");
  delay(5000);
  
  digitalWrite(releTransfer, HIGH);
  digitalWrite(ledTodoOk, HIGH);
  lcd.setCursor(0, 1);
  lcd.print("                ");
  lcd.setCursor(0, 1);
  lcd.print("Transfer OK");
  delay(1000);
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Sin suministro!");
  lcd.setCursor(0, 1);
  lcd.print("Generador ON");
  generadorEnMarcha = true;
}

void estadoError() {
  lcd.setCursor(0, 1);
  lcd.print("                ");
  lcd.setCursor(0, 1);
  lcd.print("ERROR! Revisar!");
  digitalWrite(releChoke, LOW);
  digitalWrite(releLlave, LOW);
  
  while (true) {
    // Mantiene el sistema en un estado de error
    beep(buzzer, 1, 1000); // Emite un pitido largo continuamente
    delay(2000); // Espera entre pitidos
  }
}

void restablecerSistema() {
  if (generadorEnMarcha) {
    beep(buzzer, 5, 200); // 5 pitidos cortos
    digitalWrite(ledRojo, LOW); // Apagar LED de alerta de corte
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Luz restablecida");
    
    delay(5000); // Esperar en generador 5 segundos
    digitalWrite(releTransfer, LOW);
    digitalWrite(ledTodoOk, LOW);
    lcd.setCursor(0, 1);
    lcd.print("                ");
    lcd.setCursor(0, 1);
    lcd.print("Transfer OFF");
    
    delay(5000); // Otros 5 segundos de espera
    digitalWrite(releVentilacion, LOW);
    digitalWrite(ledFan, LOW);
    lcd.setCursor(0, 1);
    lcd.print("                ");
    lcd.setCursor(0, 1);
    lcd.print("Ventilacion OFF");
    
    digitalWrite(releLlave, LOW); // Apagar contacto
    lcd.setCursor(0, 1);
    lcd.print("                ");
    lcd.setCursor(0, 1);
    lcd.print("Contacto OFF");
    delay(250);
    
    // Espera activa hasta que el motor se haya apagado
    while (digitalRead(monitorArranque)) { // Mientras esté encendido, esperar
      lcd.setCursor(0, 1);
      lcd.print("Esperando apagado");
      delay(100); // Pequeña pausa para no saturar el bucle
    }
    
    digitalWrite(ledWaiting, HIGH); // Encender LED WAITING
    digitalWrite(ledVerde, LOW); // Apagar LED motor arrancado
    lcd.setCursor(0, 1);
    lcd.print("                ");
    lcd.setCursor(0, 1);
    lcd.print("Motor apagado");
    beep(buzzer, 3, 1000); // 3 pitidos largos
    
    generadorEnMarcha = false;
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Esperando corte");
    lcd.setCursor(0, 1);
    lcd.print("de suministro...");

    arranqueRestart = 0; // Reiniciar el contador de arranques para la próxima vez
  }
}

void beep(int pin, int count, int duration) {
  for (int i = 0; i < count; i++) {
    digitalWrite(pin, HIGH);
    delay(duration);
    digitalWrite(pin, LOW);
    delay(200);
  }
}
