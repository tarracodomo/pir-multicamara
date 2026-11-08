# PIR Multicámara

@iceman — Tarraco Domo
https://tarracodomo.com

Prototipo educativo: NodeMCU ESP8266 con PIR HC-SR501 envía una orden HTTP a tres ESP32-CAM AI-Thinker. Cada cámara guarda tres fotografías JPEG en su propia microSD. Sin servicios de pago ni envío a terceros.

## Estado y alcance
Preparado con asistencia de IA. No compilado ni probado en hardware en esta publicación. No sustituye una alarma certificada ni garantiza capturas o entrega de eventos. No incluye Telegram, nube, Home Assistant, streaming, interfaz web ni descarga remota de imágenes. Las fotos se recuperan de la microSD con el equipo apagado. No instalar en equipos operativos sin respaldar sus configuraciones.

## Materiales
- Un NodeMCU ESP8266 y un PIR HC-SR501.
- Tres ESP32-CAM AI-Thinker con OV2640 y PSRAM. No asumir mismo pinout en otras placas.
- Una microSD FAT32 por cámara; guardar copia de sus datos antes de formatear.
- Alimentación regulada de 5 V adecuada para cada cámara (prever al menos 1 A por unidad y comprobar el módulo), cables cortos y adaptador USB-UART con señales de 3,3 V para programar.
- Red Wi-Fi 2,4 GHz y ordenador con PlatformIO Core o extensión PlatformIO para VS Code.

## Conexiones
NodeMCU: PIR OUT a D1/GPIO5, GND común y VCC del HC-SR501 a 5 V de alimentación regulada según ficha del módulo. La salida del PIR debe ser compatible con 3,3 V: verificar antes de conectarla; nunca aplicar 5 V al GPIO. El LED integrado se enciende al conectar a Wi-Fi; su color depende de la placa.

ESP32-CAM: alimentar por pin 5V y GND, siguiendo serigrafía y documentación de la placa. Para grabar: TX del adaptador a U0R, RX a U0T, GND común y GPIO0 a GND solo durante la carga; retirar ese puente y reiniciar después. No conectar dos fuentes que puedan realimentarse. No usar la débil salida 3V3 de un USB-UART para alimentar la cámara. Insertar microSD con alimentación desconectada. El ejemplo usa SD_MMC en modo de un bit y mantiene el flash apagado. No conectar nada a la red eléctrica.

## Preparación y carga
1. Clonar el repo y abrir su carpeta en PlatformIO.
2. Copiar include/config.example.h como include/config.h. Sustituir red, contraseña y token. Generar un token propio con openssl rand -hex 32; usar el mismo en controlador y cámaras. No compartir config.h.
3. Compilar desde la raíz: pio run -e nodemcu y pio run -e camara. Las plataformas están fijadas en platformio.ini; instalar sus dependencias requiere Internet. Si hay errores, detenerse y revisarlos antes de cargar.
4. Conectar una cámara, seleccionar su puerto y cargar: pio run -e camara -t upload --upload-port PUERTO. Retirar puente GPIO0-GND y reiniciar. Leer registros con pio device monitor -p PUERTO -b 115200. Repetir en las otras dos.
5. En el router, reservar por MAC una IP DHCP para cada cámara. El ejemplo usa 192.168.1.100, .101 y .102; cambiar por direcciones de tu propia red. El firmware usa DHCP, no fija esas IP por sí solo.
6. Actualizar CAMERA_URLS en include/config.h si cambia alguna reserva.
7. Cargar NodeMCU: pio run -e nodemcu -t upload --upload-port PUERTO. Observar registros a 115200 baudios.
8. Esperar 60 segundos de calentamiento PIR, dejar que OUT vuelva a LOW y después provocar movimiento. Ajustar potenciómetros y modo del HC-SR501 según su documentación.

PUERTO es un marcador: por ejemplo /dev/ttyUSB0 en Linux o COM5 en Windows. Elegir el puerto real, no copiarlo a ciegas.

## Comportamiento y respuestas
El controlador detecta flanco LOW a HIGH y deja 30 segundos entre eventos. Contacta las cámaras de forma secuencial: no hay sincronización exacta ni reintentos ni cola persistente. Si no hay Wi-Fi, el evento se pierde. Un PIR sostenido en HIGH no genera fotos periódicas.

Cada cámara requiere X-Capture-Token en POST /capture. Devuelve 401 si falta o no coincide, 503 sin cámara o SD, 429 durante enfriamiento y 202 si acepta. Un 202 NO confirma fotos guardadas: comprobar el resumen serial y los JPEG en SD. La captura bloquea temporalmente el servidor; no es un servidor concurrente. Se descartan frames anteriores y se usa CAMERA_GRAB_LATEST; verificar frescura física, no se garantiza latencia.

Las imágenes están en /capturas con identificador aleatorio de arranque y secuencia, no con fecha real. Si un nombre ya existe no se sobrescribe. No hay borrado automático ni cuota: vigilar el espacio libre; SD llena, pérdidas de alimentación o fallos de sensor pueden impedir grabar.

## Prueba manual de cada cámara
Desde una terminal de confianza: curl -i -X POST -H "X-Capture-Token: TU_TOKEN_LOCAL" http://IP_CAMARA/capture. No pegar el token real en chats ni capturas; el comando puede quedar en el historial. Usar solo la IP reservada propia. Después comprobar que aparecen tres JPEG legibles en la SD.

## Lista de pruebas pendientes
- Compilar ambos entornos y anotar versiones/resultados.
- Verificar que falta de token devuelve 401 y token correcto 202.
- Probar sin SD (503), SD llena, cámara desconectada y reinicio del router.
- Confirmar tres fotos recientes por evento y enfriamiento de 30 segundos.
- Probar las tres cámaras y dejar el conjunto 24 horas antes de considerarlo estable.

## Seguridad y privacidad
HTTP no cifra el token: usar únicamente una LAN de confianza, preferiblemente una red IoT aislada. No publicar puertos en Internet, ni usar UPnP para exponerlos. El token no evita escuchas ni repetición de órdenes por quien lo capture. No es un diseño para redes hostiles; para ese caso requiere transporte cifrado y autenticación más robusta.

No subir fotos, contraseñas, tokens ni direcciones privadas reales al repositorio. .gitignore no borra secretos ya versionados. Si se filtra el token, cambiarlo y volver a cargar los cuatro dispositivos. Revisar requisitos de privacidad y consentimiento de las personas captadas; orientar las cámaras solo hacia zonas donde se tenga autorización.

## Diagnóstico
Reinicios/brownout: comprobar fuente y cableado, no desactivar protección de brownout. Error cámara: comprobar modelo, PSRAM y flex con alimentación desconectada. SD: comprobar FAT32, contactos y espacio. HTTP negativo en NodeMCU: comprobar IP, Wi-Fi y conectividad; no equivale a un código de respuesta del servidor.

## Referencias
Driver de cámara: https://github.com/espressif/esp32-camera
ESP8266 Arduino: https://github.com/esp8266/Arduino
PlatformIO: https://docs.platformio.org/

## Autoría y derechos
Preparado para @iceman — Tarraco Domo con asistencia de IA. Derechos reservados sobre contenido original; sin licencia de redistribución concedida por ahora. Las bibliotecas conservan sus licencias. No se incluyen fotos ni afirmaciones de pruebas no realizadas.
