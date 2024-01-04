/*
 * Class: DFR0534
 *
 * Description:
 * Class for controlling a DFR0534 audio module (https://wiki.dfrobot.com/Voice_Module_SKU__DFR0534)
 * by SoftwareSerial
 *
 * License: 2-Clause BSD License
 * Copyright (c) 2024 codingABI
 * For details see: LICENSE.txt
 *
 * Notes for DFR0534 audio module:
 * - Consumes about 20mA when idle (Vcc = 5V)
 * - Creates a short "click" noise, when Vcc is powered on
 * - Should be used with a 1k resistor on TX when your MCU runs on 5V,
 *   because the DFR0534 uses 3.3V logic (and 5V on TX causes clicks/noise)
 * - Can be controlled by a RX/TX serial connection (9600 baud) or one-wire protocol
 * - Can play WAV and MP3 audiofiles
 * - Can "insert" audiofiles while another audiofile is running.
 *   In this case to original audiofile is paused and will be
 *   resumed after the "inserted" audiofile
 * - Can play files in a playlist like mode called "combined"
 *   for files stored in a directory /ZH
 * - Can select the file to play by a file number* or file name**
 *   *File number is independent from file name. The first WAV or MP3 copied to
 *   the DFR0534 gets file number 1 and so on. To play a file by number
 *   use playFileByNumber()
 *   **File name is a little bit like a 8+3 file path and
 *   can be used with playFileByName(), but have special rules (see playFileByName() for details)
 * - Can send automatically the file runtime every second (when enabled)
 * - Has a NS8002 amplifier, JQ8400 Audio chip, W25Q64JVSIQ flash memory
 * - Has a Sleep mode 0x1B and this mode only works with one-wire protocol (https://github.com/arduino12/mp3_player_module_wire)
 *   and does not work for me without additional electric modifications (e.g. disconnecting speakers)
 *   => Switching off DFR0534 with a FET is a better solution
 */

#include "DFR0534.h"

// Get module status
// Returns STATUSUNKNOWN in case of an error
byte DFR0534::getStatus() {
  #define COMMAND 0x01
  #define RECEIVEBYTETIMEOUTMS 100
  #define RECEIVEGLOBALTIMEOUTMS 500
  #define RECEIVEFAILED STATUSUNKNOWN
  #define RECEIVEHEADERLENGTH 2 // startingcode+command

  if (m_ptrStream == NULL) return RECEIVEFAILED; // Should not happen
  sendStartingCode();
  sendDataByte(COMMAND);;
  sendDataByte(0x00);;
  sendCheckSum();

  // Receive
  int i=0;
  byte data, firstByte = 0, sum, length=0xff, result = 0;
  unsigned long receiveStartMS = millis();
  do {
    byte dataReady = 0;
    unsigned long lastMS = millis();
    // Wait for response or timeout
    while ((millis()-lastMS < RECEIVEBYTETIMEOUTMS) && (dataReady==0)) dataReady = m_ptrStream->available();

    if (dataReady == 0) return RECEIVEFAILED; // Timeout
    data = m_ptrStream->read();

    if (i==0) { // Begin of transmission
      firstByte=data;
      sum = 0;
    }
    if ((i == 1) && (data != COMMAND)) {
      // Invalid signal => reset receive
      i=0;
      firstByte = 0;
    }
    if (i == RECEIVEHEADERLENGTH) {
      length = data; // Length of receiving data
      if (length != 1) {
        // Invalid length => reset receive
        i=0;
        firstByte = 0;
      }
    }
    if ((i > RECEIVEHEADERLENGTH) && (i-RECEIVEHEADERLENGTH-1<length)) {
      result = data;
    }
    if (firstByte == STARTINGCODE) {
      if (i-RECEIVEHEADERLENGTH<=length) sum+=data; // Update checksum
      i++;
    }
    if (millis()-receiveStartMS > RECEIVEGLOBALTIMEOUTMS) return RECEIVEFAILED; // Timeout
  } while (i<length+RECEIVEHEADERLENGTH+2);

  if (data != sum) return RECEIVEFAILED; // Does checksum matches?
  return result;
}

// Set equalizer to NORMAL, POP, ROCK, JAZZ or CLASSIC
void DFR0534::setEqualizer(byte mode) {
  if (m_ptrStream == NULL) return; // Should not happen
  if (mode >= EQUNKNOWN) return;
  sendStartingCode();
  sendDataByte(0x1A);
  sendDataByte(0x01);
  sendDataByte(mode);
  sendCheckSum();
}

// Play audio file by number (number depends on the order of copying files to the current drive. First audio file copied to the drive gets number 1, second audio file copied gets  number 2... )
void DFR0534::playFileByNumber(word track) {
  if (m_ptrStream == NULL) return; // Should not happen
  if (track <=0) return;
  sendStartingCode();
  sendDataByte(0x07);
  sendDataByte(0x02);
  sendDataByte((track >> 8) & 0xff);
  sendDataByte(track & 0xff);
  sendCheckSum();
}

// Set volume to 0-30
void DFR0534::setVolume(byte volume) {
  if (m_ptrStream == NULL) return; // Should not happen
  if (volume > 30) volume = 30;
  sendStartingCode();
  sendDataByte(0x13);
  sendDataByte(0x01);
  sendDataByte(volume);
  sendCheckSum();
}

// Play the current selected file
void DFR0534::play() {
  if (m_ptrStream == NULL) return; // Should not happen
  sendStartingCode();
  sendDataByte(0x02);
  sendDataByte(0x00);
  sendCheckSum();
}

// Pause the current file
void DFR0534::pause() {
  if (m_ptrStream == NULL) return; // Should not happen
  sendStartingCode();
  sendDataByte(0x03);
  sendDataByte(0x00);
  sendCheckSum();
}

// Stop the current file
void DFR0534::stop() {
  if (m_ptrStream == NULL) return; // Should not happen
  sendStartingCode();
  sendDataByte(0x04);
  sendDataByte(0x00);
  sendCheckSum();
}

// Play previous file (in file copy order)
void DFR0534::playPrevious() {
  if (m_ptrStream == NULL) return; // Should not happen
  sendStartingCode();
  sendDataByte(0x05);
  sendDataByte(0x00);
  sendCheckSum();
}

// Play next file (in file copy order)
void DFR0534::playNext() {
  if (m_ptrStream == NULL) return; // Should not happen
  sendStartingCode();
  sendDataByte(0x06);
  sendDataByte(0x00);
  sendCheckSum();
}

/* Play audio file by file name/path
 * "file name" is like a 8+3 format, but has special naming conventions:
 * - the dot "." is" not part of the name and every name is always 8+3 char long, for example
 *   the real file "TEST.WAV" would have the name "TEST    WAV"
 * - You can use wildcards * (=multiple arbitrary character) and ? (=one single arbitrary character) in the "name"
 * - When wildcards are used the first matching file is used
 * - Chars seems to be used in upper case
 * - Files with extensions other then WAV or MP3 will be ignored
 * - The leading / is needed
 * - When using subfolders, the folder names must also have 8 char length (space char filled or you can use wildcards * or ?), for example
 *   "/TEST    " or "/TEST*" would be valid folder names   
 *
 * Valid examples:
 * "/01      WAV" for file 01.wav
 * "/99-AFR~1MP3" for a file /99-Africa.mp3
 * "/99-AFR*MP3" for first file matching /99-Afr*.mp3
 * "/10*" for first file matching /10*.*
 * "/10      /20      WAV" for the file /10/20.wav
 */
void DFR0534::playFileByName(char *path, byte drive) {
  if (m_ptrStream == NULL) return; // Should not happen
  if (path == NULL) return;
  if (drive >= DRIVEUNKNOWN) return;
  sendStartingCode();
  sendDataByte(0x08);
  sendDataByte(strlen(path)+1);
  sendDataByte(drive);
  for (int i=0;i<strlen(path);i++) {
    sendDataByte(path[i]);
  }
  sendCheckSum();
}

// Get bit pattern that shows which drives are ready/online
// Returns DRIVEUNKNOWN in case of an error
byte DFR0534::getDrivesStates() {
  #define COMMAND 0x09
  #define RECEIVEBYTETIMEOUTMS 100
  #define RECEIVEGLOBALTIMEOUTMS 500
  #define RECEIVEFAILED DRIVEUNKNOWN
  #define RECEIVEHEADERLENGTH 2 // startingcode+command

  if (m_ptrStream == NULL) return RECEIVEFAILED; // Should not happen
  sendStartingCode();
  sendDataByte(COMMAND);
  sendDataByte(0x00);
  sendCheckSum();

  // Receive
  int i=0;
  byte data, firstByte = 0, sum, length=0xff, result = 0;
  unsigned long receiveStartMS = millis();
  do {
    byte dataReady = 0;
    unsigned long lastMS = millis();
    // Wait for response or timeout
    while ((millis()-lastMS < RECEIVEBYTETIMEOUTMS) && (dataReady==0)) dataReady = m_ptrStream->available();

    if (dataReady == 0) return RECEIVEFAILED; // Timeout
    data = m_ptrStream->read();

    if (i==0) { // Begin of transmission
      firstByte=data;
      sum = 0;
    }
    if ((i == 1) && (data != COMMAND)) {
      // Invalid signal => reset receive
      i=0;
      firstByte = 0;
    }
    if (i == RECEIVEHEADERLENGTH) {
      length = data; // Length of receiving data
      if (length != 1) {
        // Invalid length => reset receive
        i=0;
        firstByte = 0;
      }
    }
    if ((i > RECEIVEHEADERLENGTH) && (i-RECEIVEHEADERLENGTH-1<length)) {
      result = data;
    }
    if (firstByte == STARTINGCODE) {
      if (i-RECEIVEHEADERLENGTH<=length) sum+=data; // Update checksum
      i++;
    }
    if (millis()-receiveStartMS > RECEIVEGLOBALTIMEOUTMS) return RECEIVEFAILED; // Timeout
  } while (i<length+RECEIVEHEADERLENGTH+2);

  if (data != sum) return RECEIVEFAILED; // Does checksum matches?
  return result;
}

// Get current drive
// Returns DRIVEUNKNOWN in case of an error
byte DFR0534::getDrive() {
  #define COMMAND 0x0A
  #define RECEIVEBYTETIMEOUTMS 100
  #define RECEIVEGLOBALTIMEOUTMS 500
  #define RECEIVEFAILED DRIVEUNKNOWN
  #define RECEIVEHEADERLENGTH 2 // startingcode+command

  if (m_ptrStream == NULL) return RECEIVEFAILED; // Should not happen
  sendStartingCode();
  sendDataByte(COMMAND);
  sendDataByte(0x00);
  sendCheckSum();

  // Receive
  int i=0;
  byte data, firstByte = 0, sum, length=0xff, result = 0;
  unsigned long receiveStartMS = millis();
  do {
    byte dataReady = 0;
    unsigned long lastMS = millis();
    // Wait for response or timeout
    while ((millis()-lastMS < RECEIVEBYTETIMEOUTMS) && (dataReady==0)) dataReady = m_ptrStream->available();

    if (dataReady == 0) return RECEIVEFAILED; // Timeout
    data = m_ptrStream->read();

    if (i==0) { // Begin of transmission
      firstByte=data;
      sum = 0;
    }
    if ((i == 1) && (data != COMMAND)) {
      // Invalid signal => reset receive
      i=0;
      firstByte = 0;
    }
    if (i == RECEIVEHEADERLENGTH) {
      length = data; // Length of receiving data
      if (length != 1) {
        // Invalid length => reset receive
        i=0;
        firstByte = 0;
      }
    }
    if ((i > RECEIVEHEADERLENGTH) && (i-RECEIVEHEADERLENGTH-1<length)) {
      result = data;
    }
    if (firstByte == STARTINGCODE) {
      if (i-RECEIVEHEADERLENGTH<=length) sum+=data; // Update checksum
      i++;
    }
    if (millis()-receiveStartMS > RECEIVEGLOBALTIMEOUTMS) return RECEIVEFAILED; // Timeout
  } while (i<length+RECEIVEHEADERLENGTH+2);

  if (data != sum) return RECEIVEFAILED; // Does checksum matches?
  return result;
}

// Switch to drive
void DFR0534::setDrive(byte drive) {
  if (m_ptrStream == NULL) return; // Should not happen
  if (drive >= DRIVEUNKNOWN) return;
  sendStartingCode();
  sendDataByte(0x0B);
  sendDataByte(0x01);
  sendDataByte(drive);
  sendCheckSum();
}

// Get number of current file
// Returns 0 in case of an error
word DFR0534::getFileNumber() {
  #define COMMAND 0x0D
  #define RECEIVEFAILED 0
  #define RECEIVEBYTETIMEOUTMS 100
  #define RECEIVEGLOBALTIMEOUTMS 500
  #define RECEIVEHEADERLENGTH 2 // startingcode+command

  if (m_ptrStream == NULL) return RECEIVEFAILED; // Should not happen
  sendStartingCode();
  sendDataByte(COMMAND);
  sendDataByte(0x00);
  sendCheckSum();

  // Receive
  int i=0;
  byte data, firstByte = 0, sum, length=0xff;
  word result = 0;
  unsigned long receiveStartMS = millis();
  do {
    byte dataReady = 0;
    unsigned long lastMS = millis();
    // Wait for response or timeout
    while ((millis()-lastMS < RECEIVEBYTETIMEOUTMS) && (dataReady==0)) dataReady = m_ptrStream->available();

    if (dataReady == 0) return RECEIVEFAILED; // Timeout
    data = m_ptrStream->read();

    if (i==0) { // Begin of transmission
      firstByte=data;
      sum = 0;
    }
    if ((i == 1) && (data != COMMAND)) {
      // Invalid signal => reset receive
      i=0;
      firstByte = 0;
    }
    if (i == RECEIVEHEADERLENGTH) {
      length = data; // Length of receiving data
      if (length != 2) {
        // Invalid length => reset receive
        i=0;
        firstByte = 0;
      }
    }
    if ((i > RECEIVEHEADERLENGTH) && (i-RECEIVEHEADERLENGTH-1<length)) {
      switch (i-RECEIVEHEADERLENGTH-1) {
        case 0:
          result=data<<8;
          break;
        case 1:
          result+=data;
          break;
      }
    }
    if (firstByte == STARTINGCODE) {
      if (i-RECEIVEHEADERLENGTH<=length) sum+=data; // Update checksum
      i++;
    }
    if (millis()-receiveStartMS > RECEIVEGLOBALTIMEOUTMS) return RECEIVEFAILED; // Timeout
  } while (i<length+RECEIVEHEADERLENGTH+2);

  if (data != sum) return RECEIVEFAILED; // Does checksum matches?
  return result;
}

// Get total numbers for supported audio files on current drive
// Returns -1 in case of an error
int DFR0534::getTotalFiles() {
  #define COMMAND 0x0C
  #define RECEIVEFAILED -1
  #define RECEIVEBYTETIMEOUTMS 100
  #define RECEIVEGLOBALTIMEOUTMS 500
  #define RECEIVEHEADERLENGTH 2 // startingcode+command

  if (m_ptrStream == NULL) return RECEIVEFAILED; // Should not happen
  sendStartingCode();
  sendDataByte(COMMAND);
  sendDataByte(0x00);
  sendCheckSum();

  // Receive
  int i=0;
  byte data, firstByte = 0, sum, length=0xff;
  word result = 0;
  unsigned long receiveStartMS = millis();
  do {
    byte dataReady = 0;
    unsigned long lastMS = millis();
    // Wait for response or timeout
    while ((millis()-lastMS < RECEIVEBYTETIMEOUTMS) && (dataReady==0)) dataReady = m_ptrStream->available();

    if (dataReady == 0) return RECEIVEFAILED; // Timeout
    data = m_ptrStream->read();

    if (i==0) { // Begin of transmission
      firstByte=data;
      sum = 0;
    }
    if ((i == 1) && (data != COMMAND)) {
      // Invalid signal => reset receive
      i=0;
      firstByte = 0;
    }
    if (i == RECEIVEHEADERLENGTH) {
      length = data; // Length of receiving data
      if (length != 2) {
        // Invalid length => reset receive
        i=0;
        firstByte = 0;
      }
    }
    if ((i > RECEIVEHEADERLENGTH) && (i-RECEIVEHEADERLENGTH-1<length)) {
      switch (i-RECEIVEHEADERLENGTH-1) {
        case 0:
          result=data<<8;
          break;
        case 1:
          result+=data;
          break;
      }
    }
    if (firstByte == STARTINGCODE) {
      if (i-RECEIVEHEADERLENGTH<=length) sum+=data; // Update checksum
      i++;
    }
    if (millis()-receiveStartMS > RECEIVEGLOBALTIMEOUTMS) return RECEIVEFAILED; // Timeout
  } while (i<length+RECEIVEHEADERLENGTH+2);

  if (data != sum) return RECEIVEFAILED; // Does checksum matches?
  return result;
}

// Play last file in directory (in file copy order)
void DFR0534::playLastInDirectory() {
  if (m_ptrStream == NULL) return; // Should not happen
  sendStartingCode();
  sendDataByte(0x0E);
  sendDataByte(0x00);
  sendCheckSum();
}

// Play first file im next directory (in file copy order)
void DFR0534::playNextDirectory() {
  if (m_ptrStream == NULL) return; // Should not happen
  sendStartingCode();
  sendDataByte(0x0F);
  sendDataByte(0x00);
  sendCheckSum();
}

// Get number of first file in current directory
// Returns -1 in case of an error
int DFR0534::getFirstFileNumberInCurrentDirectory() {
  #define COMMAND 0x11
  #define RECEIVEFAILED -1
  #define RECEIVEBYTETIMEOUTMS 100
  #define RECEIVEGLOBALTIMEOUTMS 500
  #define RECEIVEHEADERLENGTH 2 // startingcode+command

  if (m_ptrStream == NULL) RECEIVEFAILED; // Should not happen
  sendStartingCode();
  sendDataByte(COMMAND);
  sendDataByte(0x00);
  sendCheckSum();

  // Receive
  int i=0;
  byte data, firstByte = 0, sum, length=0xff;
  word result = 0;
  unsigned long receiveStartMS = millis();
  do {
    byte dataReady = 0;
    unsigned long lastMS = millis();
    // Wait for response or timeout
    while ((millis()-lastMS < RECEIVEBYTETIMEOUTMS) && (dataReady==0)) dataReady = m_ptrStream->available();

    if (dataReady == 0) return RECEIVEFAILED; // Timeout
    data = m_ptrStream->read();

    if (i==0) { // Begin of transmission
      firstByte=data;
      sum = 0;
    }
    if ((i == 1) && (data != COMMAND)) {
      // Invalid signal => reset receive
      i=0;
      firstByte = 0;
    }
    if (i == RECEIVEHEADERLENGTH) {
      length = data; // Length of receiving data
      if (length != 2) {
        // Invalid length => reset receive
        i=0;
        firstByte = 0;
      }
    }
    if ((i > RECEIVEHEADERLENGTH) && (i-RECEIVEHEADERLENGTH-1<length)) {
      switch (i-RECEIVEHEADERLENGTH-1) {
        case 0:
          result=data<<8;
          break;
        case 1:
          result+=data;
          break;
      }
    }
    if (firstByte == STARTINGCODE) {
      if (i-RECEIVEHEADERLENGTH<=length) sum+=data; // Update checksum
      i++;
    }
    if (millis()-receiveStartMS > RECEIVEGLOBALTIMEOUTMS) return RECEIVEFAILED; // Timeout
  } while (i<length+RECEIVEHEADERLENGTH+2);

  if (data != sum) return RECEIVEFAILED; // Does checksum matches?
  return result;
}

// Get total numbers for audio files for the current directory
// Returns -1 in case of an error
int DFR0534::getTotalFilesInCurrentDirectory() {
  #define COMMAND 0x12
  #define RECEIVEFAILED -1
  #define RECEIVEBYTETIMEOUTMS 100
  #define RECEIVEGLOBALTIMEOUTMS 500
  #define RECEIVEHEADERLENGTH 2 // startingcode+command

  if (m_ptrStream == NULL) RECEIVEFAILED; // Should not happen
  sendStartingCode();
  sendDataByte(COMMAND);
  sendDataByte(0x00);
  sendCheckSum();

  // Receive
  int i=0;
  byte data, firstByte = 0, sum, length=0xff;
  word result = 0;
  unsigned long receiveStartMS = millis();
  do {
    byte dataReady = 0;
    unsigned long lastMS = millis();
    // Wait for response or timeout
    while ((millis()-lastMS < RECEIVEBYTETIMEOUTMS) && (dataReady==0)) dataReady = m_ptrStream->available();

    if (dataReady == 0) return RECEIVEFAILED; // Timeout
    data = m_ptrStream->read();

    if (i==0) { // Begin of transmission
      firstByte=data;
      sum = 0;
    }
    if ((i == 1) && (data != COMMAND)) {
      // Invalid signal => reset receive
      i=0;
      firstByte = 0;
    }
    if (i == RECEIVEHEADERLENGTH) {
      length = data; // Length of receiving data
      if (length != 2) {
        // Invalid length => reset receive
        i=0;
        firstByte = 0;
      }
    }
    if ((i > RECEIVEHEADERLENGTH) && (i-RECEIVEHEADERLENGTH-1<length)) {
      switch (i-RECEIVEHEADERLENGTH-1) {
        case 0:
          result=data<<8;
          break;
        case 1:
          result+=data;
          break;
      }
    }
    if (firstByte == STARTINGCODE) {
      if (i-RECEIVEHEADERLENGTH<=length) sum+=data; // Update checksum
      i++;
    }
    if (millis()-receiveStartMS > RECEIVEGLOBALTIMEOUTMS) return RECEIVEFAILED; // Timeout
  } while (i<length+RECEIVEHEADERLENGTH+2);

  if (data != sum) return RECEIVEFAILED; // Does checksum matches?
  return result;
}

// Increase volume by one step
void DFR0534::increaseVolume() {
  if (m_ptrStream == NULL) return; // Should not happen
  sendStartingCode();
  sendDataByte(0x14);
  sendDataByte(0x00);
  sendCheckSum();
}

// Decrease volume by one step
void DFR0534::decreaseVolume() {
  if (m_ptrStream == NULL) return; // Should not happen
  sendStartingCode();
  sendDataByte(0x15);
  sendDataByte(0x00);
  sendCheckSum();
}

/* Pause current files and play this file by number (order is file copy order)
 * Continue original track when this track stops
 */
void DFR0534::insertFileByNumber(word track, byte drive) {
  if (m_ptrStream == NULL) return; // Should not happen
  if (drive >= DRIVEUNKNOWN) return;
  sendStartingCode();
  sendDataByte(0x16);
  sendDataByte(0x03);
  sendDataByte(drive);
  sendDataByte((track >> 8) & 0xff);
  sendDataByte(track & 0xff);
  sendCheckSum();
}

// Stop inserted file
void DFR0534::stopInsertedFile() {
  if (m_ptrStream == NULL) return; // Should not happen
  sendStartingCode();
  sendDataByte(0x10);
  sendDataByte(0x00);
  sendCheckSum();
}

// Does not work for me
void DFR0534::setDirectory(char *path, byte drive) {
  if (m_ptrStream == NULL) return; // Should not happen
  if (path == NULL) return;
  if (drive >= DRIVEUNKNOWN) return;
  sendStartingCode();
  sendDataByte(0x17);
  sendDataByte(strlen(path)+1);
  sendDataByte(drive);
  for (int i=0;i<strlen(path);i++) {
    sendDataByte(path[i]);
  }
  sendCheckSum();
}

// Set loop mode
void DFR0534::setLoopMode(byte mode) {
  if (m_ptrStream == NULL) return; // Should not happen
  if (mode >= PLAYMODEUNKNOWN) return;
  sendStartingCode();
  sendDataByte(0x18);
  sendDataByte(0x01);
  sendDataByte(mode);
  sendCheckSum();
}

// Set repeat loops for loop modes LOOPBACKALL, SINGLEAUDIOLOOP and DIRECTORYLOOP
void DFR0534::setRepeatLoops(word loops) {
  if (m_ptrStream == NULL) return; // Should not happen
  sendStartingCode();
  sendDataByte(0x19);
  sendDataByte(0x02);
  sendDataByte((loops >> 8) & 0xff);
  sendDataByte(loops & 0xff);
  sendCheckSum();
}

/* Combined/concatenated play of files 
 * like a playlist, for example playCombined("0103") for
 * the two files 01 and 03. 
 * The Filenames must be two chars long and the files must
 * be in a directory called /ZH
 * Combined playback ignores loop mode and stops after last file.
 */
void DFR0534::playCombined(char* list) {
  if (m_ptrStream == NULL) return; // Should not happen
  if (list == NULL) return;
  if ((strlen(list) % 2) != 0) return;

  sendStartingCode();
  sendDataByte(0x1B);
  sendDataByte(strlen(list));
  for (int i=0;i<strlen(list);i++) {
    sendDataByte(list[i]);
  }
  sendCheckSum();
}

// Stop playlist (combined playback)
void DFR0534::stopCombined() {
  if (m_ptrStream == NULL) return; // Should not happen
  sendStartingCode();
  sendDataByte(0x1C);
  sendDataByte(0x00);
  sendCheckSum();
}

// Set output channel to SP, DAC or both
void DFR0534::setChannel(byte channel) {
  if (m_ptrStream == NULL) return; // Should not happen
  if (channel >= CHANNELUNKNOWN) return;
  sendStartingCode();
  sendDataByte(0x1D);
  sendDataByte(0x01);
  sendDataByte(channel);
  sendCheckSum();
}

/* Get name for current file
 * File name is in 8+3 format in upper case, with spaces
 * without the dot "." between name and extension,
 * e.g. "TEST   WAV" for the file test.wav
 */
bool DFR0534::getFileName(char *name) {
  #define COMMAND 0x1E
  #define RECEIVEBYTETIMEOUTMS 100
  #define RECEIVEGLOBALTIMEOUTMS 500
  #define RECEIVEFAILED false
  #define RECEIVEHEADERLENGTH 2 // startingcode+command

  if (m_ptrStream == NULL) return false; // Should not happen
  if (name == NULL) return false;
  name[0] = '\0';

  sendStartingCode();
  sendDataByte(COMMAND);
  sendDataByte(0x00);
  sendCheckSum();

  // Receive
  int i=0;
  byte data, firstByte = 0, sum, length=0xff;
  unsigned long receiveStartMS = millis();
  do {
    byte dataReady = 0;
    unsigned long lastMS = millis();
    // Wait for response or timeout
    while ((millis()-lastMS < RECEIVEBYTETIMEOUTMS) && (dataReady==0)) dataReady = m_ptrStream->available();

    if (dataReady == 0) return RECEIVEFAILED; // Timeout
    data = m_ptrStream->read();
    if (i==0) { // Begin of transmission
      firstByte=data;
      sum = 0;
    }
    if ((i == 1) && (data != COMMAND)) {
      // Invalid signal => reset receive
      i=0;
      firstByte = 0;
    }
    if (i == RECEIVEHEADERLENGTH) length = data; // Length of receiving string
    if ((i > RECEIVEHEADERLENGTH) && (i-RECEIVEHEADERLENGTH-1<length)) {
      name[i-RECEIVEHEADERLENGTH-1] = data;
      name[i-RECEIVEHEADERLENGTH] = '\0';
    }
    if (firstByte == STARTINGCODE) {
      if (i-RECEIVEHEADERLENGTH<=length) sum+=data; // Update checksum
      i++;
    }
    if (millis()-receiveStartMS > RECEIVEGLOBALTIMEOUTMS) return RECEIVEFAILED; // Timeout
  } while (i<length+RECEIVEHEADERLENGTH+2);
  return (data == sum); // Does checksum matches?
}

// Select file by number but not start playing
void DFR0534::prepareFileByNumber(word track) {
  if (m_ptrStream == NULL) return; // Should not happen
  sendStartingCode();
  sendDataByte(0x1F);
  sendDataByte(0x02);
  sendDataByte((track >> 8) & 0xff);
  sendDataByte(track & 0xff);
  sendCheckSum();
}

// Repeat part of the current file
void DFR0534::repeatPart(byte startMinute, byte startSecond, byte stopMinute, byte stopSecond ) {
  if (m_ptrStream == NULL) return; // Should not happen
  sendStartingCode();
  sendDataByte(0x20);
  sendDataByte(0x04);
  sendDataByte(startMinute);
  sendDataByte(startSecond);
  sendDataByte(stopMinute);
  sendDataByte(stopSecond);
  sendCheckSum();
}

// Stop repeating part of the current file
void DFR0534::stopRepeatPart() {
  if (m_ptrStream == NULL) return; // Should not happen
  sendStartingCode();
  sendDataByte(0x21);
  sendDataByte(0x00);
  sendCheckSum();
}

// Fast backward
void DFR0534::fastBackwardDuration(word seconds) {
  if (m_ptrStream == NULL) return; // Should not happen
  sendStartingCode();
  sendDataByte(0x22);
  sendDataByte(0x02);
  sendDataByte((seconds >> 8) & 0xff);
  sendDataByte(seconds & 0xff);
  sendCheckSum();
}

// Fast forward
void DFR0534::fastForwardDuration(word seconds) {
  if (m_ptrStream == NULL) return; // Should not happen
  sendStartingCode();
  sendDataByte(0x23);
  sendDataByte(0x02);
  sendDataByte((seconds >> 8) & 0xff);
  sendDataByte(seconds & 0xff);
  sendCheckSum();
}

// Get duration/length of current file
// Returns false in case of an error
bool DFR0534::getDuration(byte &hour, byte &minute, byte &second) {
  #define COMMAND 0x24
  #define RECEIVEFAILED false
  #define RECEIVEBYTETIMEOUTMS 100
  #define RECEIVEGLOBALTIMEOUTMS 500
  #define RECEIVEHEADERLENGTH 2 // startingcode+command

  if (m_ptrStream == NULL) return false; // Should not happen
  sendStartingCode();
  sendDataByte(COMMAND);
  sendDataByte(0x00);
  sendCheckSum();

  // Receive
  int i=0;
  byte data, firstByte = 0, sum, length=0xff;
  word result = 0;
  unsigned long receiveStartMS = millis();
  do {
    byte dataReady = 0;
    unsigned long lastMS = millis();
    // Wait for response or timeout
    while ((millis()-lastMS < RECEIVEBYTETIMEOUTMS) && (dataReady==0)) dataReady = m_ptrStream->available();

    if (dataReady == 0) return RECEIVEFAILED; // Timeout
    data = m_ptrStream->read();

    if (i==0) { // Begin of transmission
      firstByte=data;
      sum = 0;
    }
    if ((i == 1) && (data != COMMAND)) {
      // Invalid signal => reset receive
      i=0;
      firstByte = 0;
    }
    if (i == RECEIVEHEADERLENGTH) {
      length = data; // Length of receiving data
      if (length != 3) {
        // Invalid length => reset receive
        i=0;
        firstByte = 0;
      }
    }
    if ((i > RECEIVEHEADERLENGTH) && (i-RECEIVEHEADERLENGTH-1<length)) {
      switch (i-RECEIVEHEADERLENGTH-1) {
        case 0:
          hour=data;
          break;
        case 1:
          minute=data;
          break;
        case 2:
          second=data;
          break;
      }
    }
    if (firstByte == STARTINGCODE) {
      if (i-RECEIVEHEADERLENGTH<=length) sum+=data; // Update checksum
      i++;
    }
    if (millis()-receiveStartMS > RECEIVEGLOBALTIMEOUTMS) return RECEIVEFAILED; // Timeout
  } while (i<length+RECEIVEHEADERLENGTH+2);

  return (data == sum); // Does checksum matches?
}

// Start sending elapsed runtime every 1 second
void DFR0534::startSendingRuntime() {
  if (m_ptrStream == NULL) return; // Should not happen
  sendStartingCode();
  sendDataByte(0x25);
  sendDataByte(0x00);
  sendCheckSum();
}

// Get elapsed runtime/duration of the current file
// Sending runtime must be enabled before with startSendingRuntime()
// Returns false in case of an error
bool DFR0534::getRuntime(byte &hour, byte &minute, byte &second) {
  #define COMMAND 0x25
  #define RECEIVEFAILED false
  #define RECEIVEBYTETIMEOUTMS 100
  #define RECEIVEGLOBALTIMEOUTMS 500
  #define RECEIVEHEADERLENGTH 2 // startingcode+command

  if (m_ptrStream == NULL) return false; // Should not happen

  // Receive
  int i=0;
  byte data, firstByte = 0, sum, length=0xff;
  word result = 0;
  unsigned long receiveStartMS = millis();
  do {
    byte dataReady = 0;
    unsigned long lastMS = millis();
    // Wait for response or timeout
    while ((millis()-lastMS < RECEIVEBYTETIMEOUTMS) && (dataReady==0)) dataReady = m_ptrStream->available();

    if (dataReady == 0) return RECEIVEFAILED; // Timeout
    data = m_ptrStream->read();

    if (i==0) { // Begin of transmission
      firstByte=data;
      sum = 0;
    }
    if ((i == 1) && (data != COMMAND)) {
      // Invalid signal => reset receive
      i=0;
      firstByte = 0;
    }
    if (i == RECEIVEHEADERLENGTH) {
      length = data; // Length of receiving data
      if (length != 3) {
        // Invalid length => reset receive
        i=0;
        firstByte = 0;
      }
    }
    if ((i > RECEIVEHEADERLENGTH) && (i-RECEIVEHEADERLENGTH-1<length)) {
      switch (i-RECEIVEHEADERLENGTH-1) {
        case 0:
          hour=data;
          break;
        case 1:
          minute=data;
          break;
        case 2:
          second=data;
          break;
      }
    }
    if (firstByte == STARTINGCODE) {
      if (i-RECEIVEHEADERLENGTH<=length) sum+=data; // Update checksum
      i++;
    }
    if (millis()-receiveStartMS > RECEIVEGLOBALTIMEOUTMS) return RECEIVEFAILED; // Timeout
  } while (i<length+RECEIVEHEADERLENGTH+2);

  return (data == sum); // Does checksum matches?
}

// Stop sending runtime
void DFR0534::stopSendingRuntime() {
  if (m_ptrStream == NULL) return; // Should not happen
  sendStartingCode();
  sendDataByte(0x26);
  sendDataByte(0x00);
  sendCheckSum();
}
