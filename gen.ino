#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// Configuración del display I2C
LiquidCrystal_I2C lcd(0x20, 16, 2);

// Definición de pines
const int ledCorte = 2, ledVerde = 3, ledArranque = 4, ledFan = 5, ledWaiting = 6, ledTransfer = 7;
const int releContacto = 8, releAire = 9, releFan = 10, releTransfer = 11;
const int releMotorArranque = A1, monitorArranque = A2, ledFallo = A3;
const int sensorLuz = 13;
const int buzzer = 12;

// Variables de estado
bool generadorEnMarcha = false;
bool luzPrevia = true; // Asumimos que inicialmente hay luz
int intentosArranque = 0;
int arranqueRestart = 0;

unsigned long tiempoInicioGenerador = 0; // Almacenar el tiempo de inicio del generador

void setup() {
  pinMode(ledCorte, OUTPUT);
  pinMode(ledVerde, OUTPUT);
  pinMode(ledArranque, OUTPUT);
  pinMode(ledFan, OUTPUT);
  pinMode(ledWaiting, OUTPUT);
  pinMode(ledTransfer, OUTPUT);
  pinMode(ledFallo, OUTPUT);
  
  pinMode(releContacto, OUTPUT);
  pinMode(releAire, OUTPUT);
  pinMode(releFan, OUTPUT);
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
  lcd.print("Esperando fallo");
  lcd.setCursor(0, 1);
  lcd.print("de suministro...");
}

void loop() { ///////////////REVISAR
  bool luzActual = digitalRead(sensorLuz);
  static unsigned long luzVueltaTiempo = 0; // Almacenar cuándo la luz regresó por primera vez
  bool luzEstable = false;

  // Detectar corte de luz
  if (!luzActual && luzPrevia) {
    if(arranqueRestart < 1) {
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
  digitalWrite(ledCorte, HIGH);
  beep(buzzer, 3, 200); // 3 pitidos cortos
  lcd.clear();
  lcd.print("Fallo detectado!");
}

void iniciarGenerador() {
  if (!generadorEnMarcha) {
    tiempoInicioGenerador = millis(); // Guardar el tiempo de inicio del generador
    digitalWrite(ledWaiting, LOW); // Apagar LED WAITING
    digitalWrite(releContacto, HIGH); // Activar contacto
    lcd.setCursor(0, 1);
    lcd.print("Preparando...");
    delay(1000); // Esperar 1 segundo antes de intentar arrancar
    digitalWrite(releAire, HIGH);
    beep(buzzer, 1, 200); // 1 pitido corto para Aire
    lcd.setCursor(0, 1);
    lcd.print("Aire cerrado");
    delay(500);
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
  digitalWrite(releAire, LOW); // Abrir aire
  digitalWrite(ledArranque, LOW);
  lcd.setCursor(0, 1);
  lcd.print("                ");
  beep(buzzer, 1, 200); // 1 pitido corto para avisar
  lcd.setCursor(0, 1);
  lcd.print("Esperando aire...");
  delay(750); // Pequeña pausa antes de abrir el aire para evitar que se apague
  lcd.print("Aire abierto");
  delay(250);
  
  beep(buzzer, 2, 200); // Confirmación de arranque
  digitalWrite(releFan, HIGH);
  digitalWrite(ledFan, HIGH);
  lcd.setCursor(0, 1);
  lcd.print("                ");
  lcd.setCursor(0, 1);
  lcd.print("Ventilacion ON");
  delay(5000);
  
  digitalWrite(releTransfer, HIGH);
  digitalWrite(ledTransfer, HIGH);
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
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("ERROR! Revisar!");
  lcd.setCursor(0, 1);
  lcd.print("Motor no arranca");
  digitalWrite(releAire, LOW);
  digitalWrite(releContacto, LOW);
  
  while (true) {
    // Mantiene el sistema en un estado de error
    digitalWrite(ledFallo, HIGH);
    beep(buzzer, 1, 1000); // Emite un pitido largo continuamente
    digitalWrite(ledFallo, LOW);
    delay(2000); // Espera entre pitidos
  }
}

void restablecerSistema() {
  if (generadorEnMarcha) {
    beep(buzzer, 5, 200); // 5 pitidos cortos
    digitalWrite(ledCorte, LOW); // Apagar LED de alerta de corte
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Suministro elec.");
    lcd.setCursor(0, 1);
    lcd.print("restablecido!");    
    delay(2000); // Esperar 2 segundos
    lcd.setCursor(0, 0);
    lcd.print("Hay suministro! ");
    lcd.setCursor(0, 1);
    lcd.print("Esperando 10s...");
    delay(10000); // Esperar 10 segundos antes de apagar el generador
    digitalWrite(releTransfer, LOW);
    digitalWrite(ledTransfer, LOW);
    lcd.setCursor(0, 1);
    lcd.print("                ");
    lcd.setCursor(0, 1);
    lcd.print("Transfer OFF");
    
    delay(5000); // Otros 5 segundos de espera
    digitalWrite(releFan, LOW);
    digitalWrite(ledFan, LOW);
    lcd.setCursor(0, 1);
    lcd.print("                ");
    lcd.setCursor(0, 1);
    lcd.print("Ventilacion OFF");
    
    delay(500); // Pequeña pausa antes de apagar el motor
    digitalWrite(releContacto, LOW); // Apagar contacto
    lcd.setCursor(0, 1);
    lcd.print("                ");
    lcd.setCursor(0, 1);
    lcd.print("Contacto OFF");
    delay(250);
    
    // Espera activa hasta que el motor se haya apagado
    while (digitalRead(monitorArranque)) { // Mientras esté encendido, esperar
      lcd.setCursor(0, 1);
      lcd.print("Esperando motor");
      delay(100); // Pequeña pausa para no saturar el bucle
    }
    
    digitalWrite(ledWaiting, HIGH); // Encender LED WAITING
    digitalWrite(ledVerde, LOW); // Apagar LED motor arrancado
    lcd.clear();
    lcd.setCursor(0, 1);
    lcd.print("Motor apagado");
    beep(buzzer, 3, 1000); // 3 pitidos largos
     // Función para mostrar el tiempo de funcionamiento del generador al apagarse
    unsigned long tiempoFuncionamiento = (millis() - tiempoInicioGenerador) / 1000; // Tiempo en segundos
    lcd.setCursor(0, 0);
    lcd.print("Tiempo: ");
    if (tiempoFuncionamiento < 60) {
      // Mostrar solo segundos si el tiempo es menos de 60 segundos
      lcd.print(tiempoFuncionamiento);
      lcd.print("s");
    } else {
      // Calcular minutos y segundos si el tiempo es 60 segundos o más
      unsigned long minutos = tiempoFuncionamiento / 60; // Dividir por 60 para obtener los minutos
      unsigned long segundos = tiempoFuncionamiento % 60; // Usar módulo para obtener los segundos restantes
      lcd.print(minutos);
      lcd.print("m ");
      lcd.print(segundos);
      lcd.print("s");
    }
    delay(10000); // Esperar 10 segundos para poder leer la pantalla
    
    generadorEnMarcha = false;
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Esperando fallo");
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
