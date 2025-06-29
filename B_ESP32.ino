#include <HardwareSerial.h>
#include <SD.h>
#include <SPI.h>

HardwareSerial SerialTwo(2);  // UART2 (RX=16, TX=17)
const int SD_CS = 5;

File file;
bool receivingFile = false;
String currentFilename = "";

void listFiles() {
  File root = SD.open("/");
  while (true) {
    File entry = root.openNextFile();
    if (!entry) break;
    SerialTwo.println("FILE:" + String(entry.name()));
    entry.close();
  }
  root.close();
}

void sendFileToA(String filename) {
  File readFile = SD.open("/" + filename);
  if (!readFile) {
    Serial.println("Failed to open file");
    return;
  }

  SerialTwo.println("BEGIN:" + filename);
  while (readFile.available()) {
    String line = readFile.readStringUntil('\n');
    SerialTwo.println(line);
  }
  SerialTwo.println("EOF");
  readFile.close();
}

void setup() {
  Serial.begin(115200);
  SerialTwo.begin(9600, SERIAL_8N1, 16, 17);

  if (!SD.begin(SD_CS)) {
    Serial.println("SD Card Mount Failed!");
    return;
  }
  Serial.println("SD Card Ready!");
}

void loop() {
  if (SerialTwo.available()) {
    String input = SerialTwo.readStringUntil('\n');
    input.trim();

    if (input.startsWith("FILENAME:")) {
      currentFilename = input.substring(9);
      if (file) file.close();
      file = SD.open("/" + currentFilename, FILE_WRITE);
      receivingFile = true;

      if (file) {
        Serial.println("Receiving: " + currentFilename);
      } else {
        Serial.println("Failed to open file");
      }

    } else if (input == "EOF") {
      if (file) {
        file.close();
        Serial.println("Saved: " + currentFilename);
        SerialTwo.println("SAVED:" + currentFilename);
      }
      receivingFile = false;

    } else if (input == "LIST") {
      listFiles();

    } else if (input.startsWith("GET:")) {
      String reqFile = input.substring(4);
      sendFileToA(reqFile);

    } else {
      if (receivingFile && file) {
        file.println(input);
      }
    }
  }
}