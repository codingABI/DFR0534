/*
 * Example for using the DFR0534 for playing audio files by file number
 *
 * This code was made for Arduino Uno/Nano. For ESP32 you have the change the code to use HardwareSerial
 * instead of SoftwareSerial (see https://github.com/codingABI/DFR0534#hardwareserial-for-esp32)
 */

#include <SoftwareSerial.h>
#include <DFR0534.h>

#define TX_PIN A0
#define RX_PIN A1
SoftwareSerial g_serial(RX_PIN, TX_PIN);
DFR0534 g_audio(g_serial);

void setup() {
  // Serial for console output
  Serial.begin(9600);
  // Software serial for communication to DFR0534 module
  g_serial.begin(9600);

  // Set volume
  g_audio.setVolume(18);

  // Show some device infos
  Serial.print("Ready drives: ");
  byte drive = g_audio.getDrivesStates();
  if (((drive >> DFR0534::DRIVEUSB) & 1) == 1) Serial.print("USB ");
  if (((drive >> DFR0534::DRIVESD) & 1) == 1) Serial.print("SD ");
  if (((drive >> DFR0534::DRIVEFLASH) & 1) == 1) Serial.print("FLASH ");
  Serial.println();

  Serial.print("Current playing drive: ");
  switch(g_audio.getDrive()) {
    case DFR0534::DRIVEUSB:
      Serial.println("USB");
      break;
    case DFR0534::DRIVESD:
      Serial.println("SD");
      break;
    case DFR0534::DRIVEFLASH:
      Serial.println("FLASH");
      break;
    case DFR0534::DRIVENO:
      Serial.println("No drive");
      break;
    default:
      Serial.println("Unknown");
      break;
  }

  Serial.print("Total files: ");
  Serial.println(g_audio.getTotalFiles());
  Serial.print("Total files in directory: ");
  Serial.println(g_audio.getTotalFilesInCurrentDirectory());

  Serial.print("First file: ");
  Serial.println(g_audio.getFirstFileNumberInCurrentDirectory());

  // Play the first audio file copied to the DFR0534
  // (Second file copied to the DFR0534 would be number 2...)
  g_audio.playFileByNumber(1);
}

void loop() {
  static unsigned long lastDisplayMS = millis()-500;
  char name[12];

  // Show information about current track once per second
  if (millis()-lastDisplayMS > 1000) {
    Serial.print("number: ");
    word fileNumber = g_audio.getFileNumber();
    if (fileNumber > 0) Serial.print(fileNumber); else Serial.print("--");

    Serial.print(" name: ");
    if (g_audio.getFileName(name)) Serial.print(name);

    Serial.print(" status: ");
    switch (g_audio.getStatus()) {
      case DFR0534::STOPPED:
        Serial.println("Stopped");
        break;
      case DFR0534::PAUSED:
        Serial.println("Paused");
        break;
      case DFR0534::PLAYING:
        Serial.println("Playing");
        break;
      case DFR0534::STATUSUNKNOWN:
        Serial.println("Unknown");
        break;
    }
    lastDisplayMS = millis();
  }
}