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

#define DO 262
#define RE 294
#define MI 330
#define FA 349
#define SOL 392
#define LA 440
#define SI 494
#define DO2 523

// Melodía: notas y duraciones (ms)
int melodia[] = {DO, RE, MI, FA, SOL, LA, SI, DO2};
int duraciones[] = {400, 400, 400, 400, 400, 400, 400, 600};
int totalNotas = 8;

Adafruit_PN532 nfc(PN532_IRQ, PN532_RESET);
Servo miServo;

String uidValido = "4606f460";
String BufferID = "";

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
    Serial.println("ERROR: PN532 no encontrado");
    while (1)
      ;
  }
  Serial.println("PN532 detectado OK!");
  nfc.SAMConfig();
}

void loop()
{
  uint8_t uid[7];
  uint8_t uidLength;

  // Espera hasta 1 segundo por una tarjeta
  bool cardFound = nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLength, 1000);

  if (cardFound)
  {
    BufferID = "";

    Serial.print("Card UID: ");
    for (uint8_t i = 0; i < uidLength; i++)
    {
      if (uid[i] < 0x10)
        BufferID += "0";
      BufferID += String(uid[i], HEX);
    }
    Serial.println(BufferID);

    if (BufferID == uidValido)
    {
      Serial.println("ACCESO PERMITIDO");
      digitalWrite(LED_VERDE, HIGH);
      digitalWrite(LED_ROJO, LOW);
      miServo.detach();
      for (int i = 0; i < totalNotas; i++)
      {
        tone(PIN_PARLANTE, melodia[i], duraciones[i]);
        delay(duraciones[i] + 10);
      }
      noTone(PIN_PARLANTE);
      miServo.attach(PIN_SERVO);
      miServo.write(90);
      delay(3000);
      miServo.write(0);
      digitalWrite(LED_VERDE, LOW);
    }
    else
    {
      Serial.println("ACCESO DENEGADO");
      digitalWrite(LED_VERDE, LOW);
      digitalWrite(LED_ROJO, HIGH);
      miServo.detach();
      tone(PIN_PARLANTE, 300, 500);
      delay(300);
      tone(PIN_PARLANTE, 200, 500);
      delay(300);
      noTone(PIN_PARLANTE);
      miServo.attach(PIN_SERVO);
      delay(800);
      digitalWrite(LED_ROJO, LOW);
    }

    delay(1000);
  }
}
