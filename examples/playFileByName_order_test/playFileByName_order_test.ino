// playFileByName_order_test.ino

// copied these test-files one after each other in this order to the board:
// - `120bpm 16bars 96kbps.mp3` 32sec - 375,6k - as `bell3-mp3`
// - `240bpm 16bars 96kbps.mp3` 16sec -  188,3k - as `bell.mp3`
// - `300bpm 4bars 96kbps.mp3` 3s - 38,3k - as `bell1.mp3`
// then upload this test-sketch and see what the output is.
// that way you get to know in what order the playFileByName function searches.

#include <DFR0534.h>
#include <SoftwareSerial.h>

#define TX_PIN A1
#define RX_PIN A0
SoftwareSerial mp3_serial(RX_PIN, TX_PIN);
DFR0534 mp3_audio(mp3_serial);

unsigned long lastDisplayMS = 0;

void setup() {
    Serial.begin(115200);
    delay(100);

    Serial.println("playFileByName_order_test.ino");

    mp3_serial.begin(9600);
    mp3_audio.setVolume(18);

    printDeviceInfo();

    Serial.println("play /BELL*MP3");
    mp3_audio.playFileByName("/BELL*MP3");
}


void loop() {
    if (millis() - lastDisplayMS > 250) {
        printCurrentStatus();
        lastDisplayMS = millis();
    }
}




void printCurrentStatus() {
    Serial.print("fileNr: ");
    word fileNumber = mp3_audio.getFileNumber();
    if (fileNumber > 0)
        Serial.print(fileNumber);
    else
        Serial.print("--");
    
    char name[12];
    Serial.print(" name: ");
    if (mp3_audio.getFileName(name)) {
        Serial.print(name);
    }

    Serial.print(" status: ");
    switch (mp3_audio.getStatus()) {
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
}


void printDeviceInfo() {
    // Show some device infos
    Serial.print("Ready drives: ");
    byte drive = mp3_audio.getDrivesStates();
    if (((drive >> DFR0534::DRIVEUSB) & 1) == 1)
        Serial.print("USB ");
    if (((drive >> DFR0534::DRIVESD) & 1) == 1)
        Serial.print("SD ");
    if (((drive >> DFR0534::DRIVEFLASH) & 1) == 1)
        Serial.print("FLASH ");
    Serial.println();

    Serial.print("Current playing drive: ");
    switch (mp3_audio.getDrive()) {
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
    Serial.println(mp3_audio.getTotalFiles());
    Serial.print("Total files in directory: ");
    Serial.println(mp3_audio.getTotalFilesInCurrentDirectory());

    Serial.print("First file: ");
    Serial.println(mp3_audio.getFirstFileNumberInCurrentDirectory());
}