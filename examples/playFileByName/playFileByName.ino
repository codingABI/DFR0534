/*
 * Example for using the DFR0534 for playing audio files by file name
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

  /* The file name/path for the function playFileByName() is the
   * full path of the audio file to be played in format which looks like
   * a special unix 8+3 format:
   * - Without a dot between file name and file extension
   * - All characters in upper case
   * - Maximal 8 characters for file name
   * - Every file and folder whose name length is shorter then 8 chars
   *   must be filled up to the 8 chars length by space chars
   * - Must end with WAV or MP3
   * - Only WAV and MP3 files are supported
   * - Wildcards * (=multiple arbitrary characters) and ? (=one single arbitrary character)
   *   are allowed and can be used to reduce the filling space chars
   *
   * Valid examples:
   * - "/01      WAV" for a file '/01.wav'
   * - "/99-AFR~1MP3" for a file '/99-Africa.mp3'
   * - "/SUN*MP3" for first file matching '/sun*.mp3', for example '/sun.mp3'
   * - "/99-AFR*MP3" for first file matching '/99-Afr*.mp3'
   * - "/10*" for first audio file matching '/10*.*'
   * - "/10      /20      WAV" for the file '/10/20.wav'
   * (first means first in "file copy order")
   *
   * You can get example files from
   * https://github.com/codingABI/DFR0534/tree/main/assets/exampleContent
   */

  // Play the file "test.wav"
  g_audio.playFileByName("/TEST    WAV");
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