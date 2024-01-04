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
 */

#pragma once

#define DFR0534_VERSION "1.0.0"

#include <arduino.h>
#include <Stream.h>

#define STARTINGCODE 0xAA

class DFR0534 {
  public:
    enum MP3CHANNELS { CHANNELMP3, CHANNELDAC, CHANNELMP3AUX, CHANNELUNKNOWN };
    enum MP3DRIVE { DRIVEUSB, DRIVESD, DRIVEFLASH, DRIVEUNKNOWN };
    enum MP3LOOPMODE { LOOPBACKALL, SINGLEAUDIOLOOP, SINGLEAUDIOSTOP, PLAYRANDOM, DIRECTORYLOOP, RANDOMINDIRECTORY, SEQUENTIALINDIRECTORY, SEQUENTIAL, PLAYMODEUNKNOWN };
    enum MP3EQ { NORMAL, POP, ROCK, JAZZ , CLASSIC, EQUNKNOWN };
    enum MP3STATUS { STOPPED, PLAYING, PAUSED, STATUSUNKNOWN };
    DFR0534(Stream &stream) {
      m_ptrStream = &stream;
    }
    byte getStatus();
    void setEqualizer(byte mode);
    void playFileByNumber(word track);
    void setVolume(byte volume);
    void play();
    void pause();
    void stop();
    void playPrevious();
    void playNext();
    void playFileByName(char *path, byte drive=DRIVEFLASH);
    byte getDrivesStates();
    byte getDrive();
    void setDrive(byte drive);
    word getFileNumber();
    int getTotalFiles();
    void playLastInDirectory();
    void playNextDirectory();
    int getFirstFileNumberInCurrentDirectory();
    int getTotalFilesInCurrentDirectory();
    void increaseVolume();
    void decreaseVolume();
    void insertFileByNumber(word track, byte drive=DRIVEFLASH);
    void stopInsertedFile();
    void setDirectory(char *path, byte drive=DRIVEFLASH); // Seems not to work
    void setLoopMode(byte mode);
    void setRepeatLoops(word loops);
    void playCombined(char* list);
    void stopCombined();
    void setChannel(byte channel);
    bool getFileName(char *name);
    void prepareFileByNumber(word track);
    void repeatPart(byte startMinute, byte startSecond, byte stopMinute, byte stopSecond );
    void stopRepeatPart();
    void fastBackwardDuration(word seconds);
    void fastForwardDuration(word seconds);
    bool getDuration(byte &hour, byte &minute, byte &second);
    void startSendingRuntime();
    bool getRuntime(byte &hour, byte &minute, byte &second);
    void stopSendingRuntime();
  private:
    void sendStartingCode() {
      m_checksum=STARTINGCODE;
      m_ptrStream->write((byte)STARTINGCODE);
    }
    void sendDataByte(byte data) {
      m_checksum +=data;
      m_ptrStream->write((byte)data);
    }
    void sendCheckSum() {
      m_ptrStream->write((byte)m_checksum);
    }
    byte m_checksum;
    Stream *m_ptrStream = NULL;
};
