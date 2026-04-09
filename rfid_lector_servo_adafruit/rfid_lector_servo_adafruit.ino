#include <SPI.h>
#include <Wire.h>
#include <Servo.h>
#include <Adafruit_PN532.h>

#define PN532_IRQ 2
#define PN532_RESET -1
#define LED_VERDE 6
#define LED_ROJO 7
#define PIN_SERVO 3
#define PIN_PARLANTE 8
#define ID_LAB 1        // Número de laboratorio/puerta
#define TIMEOUT_MS 8000 // Tiempo máximo esperando respuesta del bridge

#define DO 262
#define RE 294
#define MI 330
#define FA 349
#define SOL 392
#define LA 440
#define SI 494
#define DO2 523

int melodiaOk[] = {DO, RE, MI, FA, SOL, LA, SI, DO2};
int duracionesOk[] = {300, 300, 300, 300, 300, 300, 300, 500};
int totalNotasOk = 8;

Adafruit_PN532 nfc(PN532_IRQ, PN532_RESET);
Servo miServo;
String BufferID = "";

// ── Helpers de actuadores ──────────────────────────────

void accesoPermitido()
{
  digitalWrite(LED_VERDE, HIGH);
  digitalWrite(LED_ROJO, LOW);

  // Melodía de éxito
  miServo.detach();
  for (int i = 0; i < totalNotasOk; i++)
  {
    tone(PIN_PARLANTE, melodiaOk[i], duracionesOk[i]);
    delay(duracionesOk[i] + 10);
  }
  noTone(PIN_PARLANTE);
  miServo.attach(PIN_SERVO);

  miServo.write(90);
  delay(3000);
  miServo.write(0);
  delay(500);

  digitalWrite(LED_VERDE, LOW);
}

void accesoDenegado()
{
  digitalWrite(LED_VERDE, LOW);
  digitalWrite(LED_ROJO, HIGH);

  // Sonido de error
  miServo.detach();
  tone(PIN_PARLANTE, 300, 400);
  delay(450);
  tone(PIN_PARLANTE, 200, 400);
  delay(450);
  noTone(PIN_PARLANTE);
  miServo.attach(PIN_SERVO);

  delay(500);
  digitalWrite(LED_ROJO, LOW);
}

// ── Setup ──────────────────────────────────────────────

void setup()
{
  Serial.begin(9600);
  SPI.begin();
  nfc.begin();

  pinMode(LED_VERDE, OUTPUT);
  pinMode(LED_ROJO, OUTPUT);
  pinMode(PIN_PARLANTE, OUTPUT);

  miServo.attach(PIN_SERVO);
  miServo.write(0);

  uint32_t versiondata = nfc.getFirmwareVersion();
  if (!versiondata)
  {
    Serial.println("ERROR:PN532_NO_ENCONTRADO");
    while (1)
      ;
  }

  nfc.SAMConfig();
  Serial.println("READY"); // bridge.js puede esperar este mensaje al iniciar
}

// ── Loop ───────────────────────────────────────────────

void loop()
{
  uint8_t uid[7];
  uint8_t uidLength;

  bool cardFound = nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLength, 1000);

  if (!cardFound)
    return;

  // 1. Construir UID como string hex
  BufferID = "";
  for (uint8_t i = 0; i < uidLength; i++)
  {
    if (uid[i] < 0x10)
      BufferID += "0";
    BufferID += String(uid[i], HEX);
  }

  // 2. Enviar al bridge: "cardUuid,labId\n"
  Serial.print(BufferID);
  Serial.print(",");
  Serial.println(ID_LAB); // println agrega \n automáticamente

  // 3. Esperar respuesta del bridge con timeout
  String respuesta = "";
  unsigned long inicio = millis();

  while (millis() - inicio < TIMEOUT_MS)
  {
    if (Serial.available())
    {
      respuesta = Serial.readStringUntil('\n');
      respuesta.trim();
      break;
    }
  }

  // 4. Actuar según respuesta
  if (respuesta == "1")
  {
    accesoPermitido();
  }
  else
  {
    // "0" explícito o timeout → denegar
    accesoDenegado();
  }

  // 5. Pausa anti-rebote antes de leer otra tarjeta
  delay(1500);
}