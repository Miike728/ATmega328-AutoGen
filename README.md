# Sistema Automático de Control para Generador (Prototipo v1)
### ⚠ En desarrollo versión 2, con novedades y preparada para uso real. ⚠

## Descripción
Este proyecto consiste en un sistema de control automático para un generador eléctrico que utiliza el microcontrolador ATmega328 (como Arduino Uno). Está diseñado para detectar cortes de energía y activar automáticamente el generador en respuesta, además de gestionar el apagado del mismo una vez que la energía eléctrica se restablece. El sistema incluye un display LCD para informar sobre el estado y la duración del funcionamiento del generador.
 
## Características
- **Detección automática de cortes de luz**: El sistema utiliza un sensor de voltaje para detectar la pérdida de energía eléctrica y activar el generador.
- **Control de arranque y parada**: Gestiona el arranque del generador mediante relés y monitorea el estado del motor para asegurar que arranque y se detenga correctamente.
- **Seguridad y reintentos de arranque**: Intenta arrancar el generador hasta tres veces en caso de fallo; si no arranca, entra en un estado de error para proteger el motor de arranque.
- **Control del voltaje de batería**: Tiene en cuenta el voltaje de la batería, si este es muy bajo no arrancará por seguridad. Utiliza un divisor de voltaje con resistencias para este fin.
- **Información en tiempo real**: Muestra el estado actual del sistema y el tiempo de funcionamiento del generador en un display LCD.
- **Restablecimiento automático**: Desactiva el generador y vuelve al estado de espera una vez que la energía eléctrica se ha restablecido y se mantiene estable durante 5 segundos (modificable).

## Componentes
- ATmega328 y programador o Arduino Uno
- Display LCD I2C
- Relés para control de motor y otros componentes
- Contactor para el transfer
- LEDs para indicación de estado
- Zumbador pasivo para alertas auditivas
- Sensores para detección de estado del generador y presencia de energía eléctrica
- Sensor **MCP9701-E/TO** para la temperatura
- Resistencias 22k y 10k para medir el voltaje de la batería

## Esquema del circuito
El esquema del circuito incluye conexiones entre el Arduino Uno y los relés, sensores, LEDs, y el display LCD. (Aún no disponible)

## Código
El código para Arduino está escrito en C++ y utiliza varias librerías para manejar el hardware conectado. Incluye rutinas para la gestión de eventos (como la detección de cortes de luz y el arranque del generador), así como la interfaz de usuario en el LCD.

## PCB
#### ⚠ PCB prototipo v1, no es la versión final.
![PCB-v1-Cara](https://github.com/user-attachments/assets/ef147093-b9d2-4ea9-86e5-d7124e487b01)

<!---
## Configuración y uso
Para utilizar este sistema:
1. Conecte todos los componentes según el esquema del circuito.
2. Suba el código proporcionado al Arduino Uno.
3. Asegúrese de que el sensor de luz esté bien posicionado para detectar la luz ambiente.
4. Al detectar un corte de luz, el sistema automáticamente intentará arrancar el generador y, si es exitoso, mostrará el estado en el display LCD.
--->

## Contribuir
Este proyecto es abierto a contribuciones. Si deseas mejorar el código o sugerir cambios en el hardware, siéntete libre de hacer un fork del repositorio y enviar pull requests.

## Licencia
Este proyecto está licenciado bajo la Licencia MIT - vea el archivo LICENSE.md para más detalles.
