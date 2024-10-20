/*
 * Example for using the DFR0534 for playing combined audio files like a playlist
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

  /* The parameter string for the playCombined function is just
   * a concatenation of all files in the desired order without
   * path and without extension.
   * All files have to be in the folder /ZH and the each
   * file has to have a length (without extension) of two chars.
   *
   * You can get example files from https://github.com/codingABI/DFR0534/tree/main/assets/exampleContent
   */

  /* Plays files the custom order, like a playlist and stops after the last file:
   * /ZH/05.wav
   * /ZH/04.wav
   * /ZH/03.wav
   * /ZH/02.wav
   * /ZH/01.wav
   * /ZH/0A.wav
   */
  g_audio.playCombined("05040302010A");
}

void loop() {
  static unsigned long lastDisplayMS = millis();
  char name[12];

  // Show information about current track every 500ms
  if (millis()-lastDisplayMS > 500) {
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