/********************************
    Bibliotecas requeridas:
********************************/
#include <Wire.h>
#include <Servo.h>
#include <LiquidCrystal_I2C.h>

// Configuración del display I2C
// LiquidCrystal_I2C lcd(0x20, 16, 2); // Simulación
LiquidCrystal_I2C lcd(0x27, 16, 2); // Realidad

/********************************
    Constantes y variables 
    modificables:
********************************/
// Constantes de tiempo del ventilador
const unsigned long TIEMPO_INICIAL_VENTILADOR = 15000; // Tiempo que pasa encendido el ventilador al arrancar (15 segundos)
const unsigned long INTERVALO_MOVIMIENTO_AIRE = 180000; // Tiempo hasta que se encienda para mover el aire de la sala (esto solo funciona si el ventilador no se activa por temperatura) (3 minutos)
const unsigned long DURACION_MOVIMIENTO_AIRE = 25000; // Tiempo que pasa encendido el ventilador para mover el aire de la sala (25 segundos)
const unsigned long TIEMPO_MAXIMO_VENTILADOR = 300000; // Tiempo máximo que puede estar encendido el ventilador (5 minutos)
const unsigned long TIEMPO_DESCANSO_VENTILADOR = 30000; // Tiempo de descanso del ventilador tras estar encendido 5 minutos (30 segundos)

// Valores de temperatura del ventilador
const float TEMPERATURA_ENCENDIDO = 35.0; // Enciende a 35°C
const float TEMPERATURA_APAGADO = 30.0; // Apaga a 30°C

// Variables de tiempo del motor
unsigned long tiempoInicioMotor = 0; // Momento en que el motor comenzó a funcionar
unsigned long tiempoApagadoMotor = 0; // Momento en que el motor se apagó
unsigned long duracionMotorEncendido = 0; // Duración del motor encendido en milisegundos
bool motorEncendido = false; // Flag para indicar si el motor está encendido


// Definición de pines
const int ledCorte = 2, ledMotorOn = 3, ledStarting = 4, ledFan = 5, ledWaiting = 6, ledTransfer = 7;
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

// Variable de tiempo de inicio del generador
unsigned long tiempoInicioGenerador = 0;

// Variable para guardar el tiempo del corte de luz
unsigned long tiempoCorte = 0;
bool esperandoCorte = false; // Flag para indicar que estamos esperando para iniciar el generador

// Variables de temperatura
float temperaturaPCB;

// Variables de estado del ventilador
unsigned long tiempoInicioVentilador = 0;
unsigned long tiempoInicioMovimientoAire = 0;
unsigned long tiempoVentiladorEncendido = 0;
unsigned long tiempoProteccionVentilador = 0;
bool ventiladorEncendido = false;
bool movimientoAireActivo = false;
bool enPausaProteccion = false;

void setup() {
  pinMode(ledCorte, OUTPUT);
  pinMode(ledMotorOn, OUTPUT);
  pinMode(ledStarting, OUTPUT);
  pinMode(ledFan, OUTPUT);
  pinMode(ledWaiting, OUTPUT);
  pinMode(ledTransfer, OUTPUT);
  pinMode(ledFallo, OUTPUT);
  
  pinMode(releContacto, OUTPUT);
  pinMode(releFan, OUTPUT);
  pinMode(releTransfer, OUTPUT);
  pinMode(releStarter, OUTPUT);
  pinMode(detectorCorte, INPUT);
  pinMode(buzzer, OUTPUT);
  
  digitalWrite(ledWaiting, HIGH); // LED WAITING encendido inicialmente

  // Iniciar LCD
  lcd.init();
  lcd.backlight();

  // Test inicial de componentes
  lcd.setCursor(0, 0);
  lcd.print("Inicializando...");
  servoChoke.attach(9);  // Adjuntar el servo al pin 9
  delay(1000); // Pequeña pausa para permitir que el servo se inicialice
  servoChoke.write(150); // Probar con el choke abierto
  delay(500);
  servoChoke.write(50);   // Empezar con el choke cerrado
  delay(1000);
  servoChoke.write(150); // Dejar abierto
  servoChoke.detach(); // Desconectar el servo para evitar desgaste
  digitalWrite(ledWaiting, HIGH); // Encender LED WAITING
  delay(250);
  digitalWrite(ledWaiting, LOW); // Apagar LED WAITING
  digitalWrite(ledCorte, HIGH); // Encender LED de alerta de corte
  delay(250);
  digitalWrite(ledCorte, LOW); // Apagar LED de alerta de corte
  digitalWrite(ledStarting, HIGH); // Encender LED de motor arrancado
  delay(250);
  digitalWrite(ledStarting, LOW); // Apagar LED de motor arrancado
  digitalWrite(ledMotorOn, HIGH); // Encender LED de arranque
  delay(250);
  digitalWrite(ledMotorOn, LOW); // Apagar LED de arranque
  digitalWrite(ledFan, HIGH); // Encender LED de ventilación
  delay(250);
  digitalWrite(ledFan, LOW); // Apagar LED de ventilación
  digitalWrite(ledTransfer, HIGH); // Encender LED de transferencia
  delay(250);
  digitalWrite(ledTransfer, LOW); // Apagar LED de transferencia
  digitalWrite(ledFallo, HIGH); // Encender LED de fallo
  delay(250);
  digitalWrite(ledFallo, LOW); // Apagar LED de fallo
  beepInfo(); // Aviso sonoro de información
  
  digitalWrite(releContacto, HIGH); // Apagar contacto (funciona al revés)
  digitalWrite(ledWaiting, HIGH); // Encender LED WAITING

  // Leer temperatura actual del PCB
  float temperaturaPCB = leerTemperaturaPCB();

  // Mostrar mensaje de espera
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Esperando corte");
  
  // Apagar backlight del LCD para ahorrar
  lcd.noBacklight();
}

void loop() {
  bool luzActual = digitalRead(detectorCorte);
  static unsigned long luzVueltaTiempo = 0; // Almacenar cuándo la luz regresó por primera vez
  bool luzEstable = false;

  // Lectura de sensores
  temperaturaPCB = leerTemperaturaPCB();
  // Obsoleto / Eliminar:
  // lecturaAuxiliar = analogRead(sensorAuxiliar);
  // float temperaturaAuxiliar = convertirTemperatura(lecturaAuxiliar); 
  lcd.setCursor(0, 1);
  lcd.print(temperaturaPCB);
  lcd.setCursor(6, 1);
  lcd.print("C");

  controlarVentilador(); // Control del ventilador basado en la temperatura

  // Detectar corte de luz
  if (!luzActual && luzPrevia) {
    if (arranqueRestart < 1) {
    alertaCorteLuz();
    tiempoCorte = millis();
    esperandoCorte = true;
    }
  }

  // Si estamos esperando para iniciar el generador y han pasado 3 segundos desde el corte
  if (esperandoCorte && millis() - tiempoCorte >= 3000) {
    if (!luzActual) {
      // Si la luz no ha vuelto, iniciar el generador
      if (arranqueRestart < 1) {
        iniciarGenerador();
        arranqueRestart++;
      }
    } else {
      // Si la luz volvió, cancelar la alerta
      cancelarAlertaCorteLuz();
    }
    esperandoCorte = false; // Resetear el flag de espera
  }

  // Si la luz vuelve y estábamos esperando
  if (luzActual && esperandoCorte) {
    cancelarAlertaCorteLuz();
    esperandoCorte = false;
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
  delay(50); // Pequeña pausa para no saturar el bucle
}

void alertaCorteLuz() {
  digitalWrite(ledCorte, HIGH);
  beepWarning(); // Aviso sonoro de advertencia
  lcd.clear();
  lcd.print("Corte detectado!");
  lcd.backlight(); // Encender backlight del LCD
}

void cancelarAlertaCorteLuz() {
  digitalWrite(ledCorte, LOW);
  beepInfo(); // Aviso sonoro de información
  lcd.clear();
  lcd.print("Corte cancelado!");
  delay(250);
  lcd.noBacklight(); // Apagar backlight del LCD
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Esperando corte");
}

void iniciarGenerador() {
  if (!generadorEnMarcha) {
    tiempoInicioGenerador = millis(); // Guardar el tiempo de inicio del generador
    digitalWrite(ledWaiting, LOW); // Apagar LED WAITING
    digitalWrite(releContacto, LOW); // Activar contacto (inverso)
    lcd.setCursor(0, 1);
    lcd.print("Preparando...");
    delay(1000); // Esperar 1 segundo antes de intentar arrancar
    cerrarAire(); // Cerrar el aire
    beepInfo(); // Aviso sonoro de información
    lcd.setCursor(0, 1);
    lcd.print("Aire cerrado");
    delay(250);
    intentarArrancar();
  }
}

void intentarArrancar() {
  intentosArranque = 0;
  bool arranqueExitoso = false;
  unsigned long tiempoDesdeApagado = millis() - tiempoApagadoMotor;

  // Leer voltaje inicial de la batería antes de intentar arrancar
  double voltajeBateria = ((analogRead(A0) / 1023.0) * 5.0 * (22000.0 + 10000.0) / 10000.0);
  double voltajeAnterior = voltajeBateria;

  while (intentosArranque < 3 && !arranqueExitoso) {
    // Verifica el voltaje de la batería antes de intentar arrancar
    if (verificarBateriaBaja()) {
      estadoErrorBateriaBaja();
      return; // Sale de la función si la batería está baja
    }

    ajustarAireSegunTemperatura(tiempoDesdeApagado, duracionMotorEncendido);  // Ajustar el aire según la temperatura

    beepArranque(); // Aviso sonoro de arranque

    digitalWrite(releStarter, HIGH); // Activa el motor de arranque
    digitalWrite(ledStarting, HIGH);

    lcd.setCursor(0, 1);
    lcd.print("Arrancando motor...");
    
    unsigned long startTime = millis(); // Tiempo inicial
    bool motorGirando = false; // Flag para indicar si el motor de arranque está girando
    
    while (millis() - startTime < 2000) { // Período de 2 segundos para intentar arrancar
      voltajeBateria = ((analogRead(A0) / 1023.0) * 5.0 * (22000.0 + 10000.0) / 10000.0);
      
      // Si el voltaje cae 2V o más desde el valor inicial, el motor está girando
      if (!motorGirando && (voltajeAnterior - voltajeBateria >= 2.0)) {
        motorGirando = true;
        digitalWrite(ledStarting, HIGH); // Encender LED de arranque
        voltajeAnterior = voltajeBateria;
      }
      
      // Si el voltaje sube 1.5V desde el valor de caída, el motor ha arrancado
      if (motorGirando && (voltajeBateria - voltajeAnterior >= 1.0)) {
        arranqueExitoso = true;
        digitalWrite(ledStarting, LOW); // Apagar LED de arranque
        digitalWrite(ledMotorOn, HIGH); // Encender LED de motor en marcha
        break; // Salir del ciclo si el motor arranca
      }
      
      delay(100); // Pequeña pausa para no saturar la lectura del pin
    }

    digitalWrite(releStarter, LOW); // Desactiva el motor de arranque
    digitalWrite(ledStarting, LOW); // Apagar LED de arranque

    if (arranqueExitoso) {
          tiempoInicioMotor = millis(); // Registro del tiempo de inicio del motor
          motorEncendido = true; // Indicar que el motor está encendido
    } else {
        intentosArranque++;
        digitalWrite(releContacto, HIGH); // Apagar contacto (inverso) (por si arrancó y no se detectó)
        limpiarSegundaLineaLCD();
        lcd.setCursor(0, 1);
        lcd.print("Fallo arranque ");
        lcd.setCursor(15, 1);
        lcd.print(intentosArranque);
        beepWarning(); // Aviso sonoro de advertencia
        delay(5000); // Espera antes del próximo intento
        digitalWrite(releContacto, LOW); // Encender contacto (inverso) para el próximo intento
    }
  }
    //AÑADIDO DE VUELTA
    if (arranqueExitoso) {
        operacionNormal(); // Va al estado normal si arranca
    } else {
        estadoError(); // Va al estado de error si no arranca después de 3 intentos
    }
}


void operacionNormal() {
  limpiarSegundaLineaLCD();
  lcd.setCursor(0, 1);
  lcd.print("Motor arrancado");
  digitalWrite(ledMotorOn, HIGH);
  digitalWrite(ledStarting, LOW);
  limpiarSegundaLineaLCD();
  beepInfo(); // Aviso sonoro de información

  lcd.setCursor(0, 1);
  lcd.print("Esperando aire...");
  delay(2000); // Pequeña pausa antes de abrir el aire para evitar que se apague
  abrirAire(); // Abrir el aire después de arrancar
  delay(250);
  beepInfo(); // Aviso sonoro de información
  digitalWrite(releFan, HIGH);
  digitalWrite(ledFan, HIGH);
  limpiarSegundaLineaLCD();
  lcd.setCursor(0, 1);
  lcd.print("Ventilador ON");

  // Espera para calentar un poco el motor antes de transferir la carga
  delay(1000);
  lcd.setCursor(0, 1);
  lcd.print("Calentando...");
  delay(9000);
  avisoTransfer(); // Aviso de transferencia

  digitalWrite(releTransfer, HIGH); // Transferir la carga
  digitalWrite(ledTransfer, HIGH);
  limpiarSegundaLineaLCD();
  lcd.setCursor(0, 1);
  lcd.print("Transfer OK");
  delay(1000);
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Generador ON");
  limpiarSegundaLineaLCD();
  generadorEnMarcha = true;
}

void estadoError() {
  lcd.clear();
  abrirAire(); // Abrir el aire en caso de error
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("ERROR! Revisar!");
  lcd.setCursor(0, 1);
  lcd.print("Motor no arranca");
  digitalWrite(releContacto, HIGH); // Apagar contacto (inverso)
  
  while (true) {
    // Mantiene el sistema en un estado de error
    digitalWrite(ledFallo, HIGH);
    lcd.backlight(); // Encender backlight del LCD
    beepError(); // Aviso sonoro de error grave
    digitalWrite(ledFallo, LOW);
    lcd.noBacklight(); // Apagar backlight del LCD
    delay(2000); // Espera entre pitidos
  }
}

void restablecerSistema() {
  if (generadorEnMarcha) {
    beepInfo(); // Aviso sonoro de información
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
    limpiarSegundaLineaLCD();
    lcd.setCursor(0, 1);
    lcd.print("Transfer OFF");
    delay(1000);
    limpiarSegundaLineaLCD();
    lcd.setCursor(0, 1);
    lcd.print("Enfriando...");
    delay(15000); // 15 segundos de enfriamiento antes de apagar el ventilador
    digitalWrite(releFan, LOW);
    digitalWrite(ledFan, LOW);
    limpiarSegundaLineaLCD();
    lcd.setCursor(0, 1);
    lcd.print("Ventilacion OFF");
    delay(1000); // Pequeña pausa antes de apagar el motor
    digitalWrite(releContacto, HIGH); // Apagar contacto (inverso)
    limpiarSegundaLineaLCD();
    lcd.setCursor(0, 1);
    lcd.print("Contacto OFF");
    delay(1500); // Pequeña pausa
    digitalWrite(ledWaiting, HIGH); // Encender LED WAITING
    digitalWrite(ledMotorOn, LOW); // Apagar LED motor arrancado
    apagarMotor(); // Calcular la duración del motor encendido
    lcd.clear();
    lcd.setCursor(0, 1);
    lcd.print("Motor apagado");
    beepInfo(); // Aviso sonoro de información
    delay(100);
    beepInfo(); // Aviso sonoro de información
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
    lcd.print("Esperando corte");
    limpiarSegundaLineaLCD();
    lcd.noBacklight(); // Apagar backlight del LCD

    arranqueRestart = 0; // Reiniciar el contador de arranques para la próxima vez
  }
}

// Eliminar: 
/**
float convertirTemperatura(int lectura) {
  float temperatura = lectura * (5.0 / 1024.0); // Convertir lectura a voltaje (tensión)
  temperatura = (temperatura - 0.5) / 0.01; // Convertir voltaje a temperatura en grados Celsius
  return temperatura;
} 
**/

float leerTemperaturaPCB() {
  vout = analogRead(sensorPCB) * (4976.30 / 1023);
  float temperatura = ((vout - vout0) / tc) - 5;
  return temperatura; // Devuelve la temperatura en grados Celsius
}

// Función para emitir pitidos
void beep(int pin, int frequency, int duration, int pause) {
  tone(pin, frequency, duration);
  delay(duration + pause);
  noTone(pin);
}

// Funciones específicas para diferentes tipos de avisos
void beepInfo() {
  beep(buzzer, 1000, 200, 100); // 1000Hz, 200ms sonido, 100ms pausa
}

void beepArranque() {
  for (int i = 0; i < 3; i++) {
    beep(buzzer, 2000, 850, 250);
  }
}

void beepWarning() {
  for (int i = 0; i < 2; i++) {
    beep(buzzer, 1500, 200, 100);
  }
}

void beepFan() {
  for (int i = 0; i < 3; i++) {
  beep(buzzer, 2000, 250, 100);
  }
}

void beepError() {
  for (int i = 0; i < 3; i++) {
    beep(buzzer, 2000, 500, 300);
  }
}

void avisoTransfer() {
  for (int i = 0; i < 2; i++) {
    digitalWrite(ledTransfer, HIGH);
    beep(buzzer, 1000, 500, 250); // Buzzer a 2000Hz, 200ms sonido, 100ms pausa
    digitalWrite(ledTransfer, LOW);
  }
}

bool verificarBateriaBaja() {
  voltajeBateria = (analogRead(A0) / 1023.0) * 5.0 * (22000.0 + 10000.0) / 10000.0;
  return voltajeBateria < 11.4;
}

void estadoErrorBateriaBaja() {
  lcd.clear();
  abrirAire(); // Abrir el aire en caso de error
  lcd.setCursor(0, 0);
  lcd.print("ERROR! Revisar!");
  lcd.setCursor(0, 1);
  lcd.print("Bateria baja.");
  digitalWrite(releContacto, HIGH); // Apagar contacto (inverso)
  
  while (true) {
    digitalWrite(ledFallo, HIGH);
    lcd.backlight(); // Encender backlight del LCD
    beepError(); // Aviso sonoro de error grave
    digitalWrite(ledFallo, LOW);
    lcd.noBacklight(); // Apagar backlight del LCD
    delay(2000); // Espera entre pitidos
  }
}

// Funciones para controlar el aire
void cerrarAire() {
 servoChoke.attach(9);  // Adjuntar el servo al pin 9
  servoChoke.write(50);
  delay(750); // Pequeña pausa para permitir que el servo se mueva
  servoChoke.detach(); // Desconectar el servo
  limpiarSegundaLineaLCD();
  lcd.setCursor(0, 1);
  lcd.print("Aire cerrado");
}

void abrirAire() {
  servoChoke.attach(9);  // Adjuntar el servo al pin 9
  servoChoke.write(150);
  delay(750); // Pequeña pausa para permitir que el servo se mueva
  servoChoke.detach(); // Desconectar el servo
  limpiarSegundaLineaLCD();
  lcd.setCursor(0, 1);
  lcd.print("Aire abierto");
}

// Función para controlar el ventilador
void controlarVentilador() {
  if (!generadorEnMarcha) {
    // Si el generador no está en marcha, apagamos el ventilador y reiniciamos contadores
    digitalWrite(releFan, LOW);
    digitalWrite(ledFan, LOW);
    ventiladorEncendido = false;
    movimientoAireActivo = false;
    enPausaProteccion = false;
    tiempoInicioVentilador = 0;
    tiempoInicioMovimientoAire = 0;
    tiempoVentiladorEncendido = 0;
    tiempoProteccionVentilador = 0;
    return;
  }

  // Leer temperatura actual del PCB
  temperaturaPCB = leerTemperaturaPCB();

  // Control inicial: encender el ventilador al arrancar por el tiempo definido
  if (tiempoInicioVentilador == 0) {
    digitalWrite(releFan, HIGH);
    digitalWrite(ledFan, HIGH);
    // beepFan(); // Aviso sonoro de ventilador
    tiempoInicioVentilador = millis();
    ventiladorEncendido = true;
  } else if (millis() - tiempoInicioVentilador > TIEMPO_INICIAL_VENTILADOR && !enPausaProteccion) {
    digitalWrite(releFan, LOW);
    digitalWrite(ledFan, LOW);
    ventiladorEncendido = false;
    tiempoInicioVentilador = millis(); // Reiniciar tiempo inicial para la siguiente operación
  }

  // Protección del ventilador: Apagar si ha estado encendido más de 5 minutos
  if (ventiladorEncendido && millis() - tiempoVentiladorEncendido > TIEMPO_MAXIMO_VENTILADOR) {
    digitalWrite(releFan, LOW);
    digitalWrite(ledFan, LOW);
    ventiladorEncendido = false;
    enPausaProteccion = true;
    tiempoProteccionVentilador = millis(); // Guardar el tiempo de inicio de la pausa
  }

  // Control de la pausa de protección
  if (enPausaProteccion && millis() - tiempoProteccionVentilador >= TIEMPO_DESCANSO_VENTILADOR) {
    enPausaProteccion = false; // Finalizar la pausa de protección
    tiempoVentiladorEncendido = millis(); // Resetear el contador del tiempo encendido
  }

  // Control de temperatura
  if (!enPausaProteccion && temperaturaPCB > TEMPERATURA_ENCENDIDO) {
    // Si la temperatura supera los 40°C, encender el ventilador
    digitalWrite(releFan, HIGH);
    digitalWrite(ledFan, HIGH);
    // beepFan(); // Aviso sonoro de ventilador
    ventiladorEncendido = true;
    movimientoAireActivo = false; // Resetear movimiento de aire si está encendido por temperatura
    tiempoVentiladorEncendido = millis();
  } else if (temperaturaPCB < TEMPERATURA_APAGADO && ventiladorEncendido && !movimientoAireActivo && !enPausaProteccion) {
    // Apagar el ventilador si la temperatura baja de 35°C
    digitalWrite(releFan, LOW);
    digitalWrite(ledFan, LOW);
    ventiladorEncendido = false;
    tiempoInicioMovimientoAire = millis(); // Reiniciar el contador de movimiento de aire
  }

  // Movimiento de aire cada 3 minutos
  if (!ventiladorEncendido && millis() - tiempoInicioMovimientoAire > INTERVALO_MOVIMIENTO_AIRE && !enPausaProteccion) {
    digitalWrite(releFan, HIGH);
    digitalWrite(ledFan, HIGH);
    // beepFan(); // Aviso sonoro de ventilador
    movimientoAireActivo = true;
    tiempoInicioMovimientoAire = millis(); // Reiniciar el contador
  } else if (movimientoAireActivo && millis() - tiempoInicioMovimientoAire > DURACION_MOVIMIENTO_AIRE) {
    digitalWrite(releFan, LOW);
    digitalWrite(ledFan, LOW);
    movimientoAireActivo = false;
    tiempoInicioMovimientoAire = millis(); // Reiniciar el contador
  }
}

void apagarMotor() {
  // Código para apagar el motor
  motorEncendido = false;
  tiempoApagadoMotor = millis();
  duracionMotorEncendido = tiempoApagadoMotor - tiempoInicioMotor; // Calcula la duración del motor encendido
}

void ajustarAireSegunTemperatura(unsigned long tiempoDesdeApagado, unsigned long duracionMotorEncendido) {
    if (duracionMotorEncendido < 15000 && tiempoDesdeApagado < 60000) {
        // Motor funcionó menos de 15 segundos y se apagó hace menos de 1 minuto
        cerrarAire();
        limpiarSegundaLineaLCD();
        lcd.setCursor(0, 1);
        lcd.print("Aire cerrado");
    } else if (tiempoDesdeApagado < 60000) {
        // Motor se apagó hace menos de 1 minuto
        abrirAire();
        limpiarSegundaLineaLCD();
        lcd.setCursor(0, 1);
        lcd.print("Aire abierto");
    } else {
        // Motor se apagó hace más de 1 minuto
        cerrarAire();
        limpiarSegundaLineaLCD();
        lcd.setCursor(0, 1);
        lcd.print("Aire cerrado");
    }
}

// Función para limpiar la segunda línea del LCD
void limpiarSegundaLineaLCD() {
  lcd.setCursor(0, 1);
  lcd.print("                ");
}
