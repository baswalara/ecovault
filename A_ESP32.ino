#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <HardwareSerial.h>

HardwareSerial SerialTwo(2);  // UART2 (RX=16, TX=17)
AsyncWebServer server(80);

const char* ssid = "Oneplus";
const char* password = "12345677";

String fileListHTML = "";
String expectedFileName = "";
String fileDownloadBuffer = "";
bool receivingFile = false;

const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
  <body>
    <h2>Upload TXT file</h2>
    <form method="POST" action="/upload" enctype="multipart/form-data">
      <input type="file" name="upload"><br><br>
      <input type="submit" value="Upload">
    </form>
    <hr>
    <h2>Files on SD Card</h2>
    %FILE_LIST%
  </body>
</html>
)rawliteral";

void setup() {
  Serial.begin(115200);
  SerialTwo.begin(9600, SERIAL_8N1, 16, 17);

  WiFi.begin(ssid, password);
  Serial.print("Connecting to Wi-Fi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();
  Serial.println("Connected! IP address: " + WiFi.localIP().toString());

  // Serve upload page
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    SerialTwo.println("LIST");
    delay(300);  // wait for list to fill fileListHTML
    String page = index_html;
    page.replace("%FILE_LIST%", fileListHTML);
    request->send(200, "text/html", page);
  });

  // Upload handler
  server.on(
    "/upload", HTTP_POST,
    [](AsyncWebServerRequest *request) {
      request->send(200, "text/plain", "File uploaded and sent to ESP32-B.");
    },
    [](AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final) {
      if (index == 0) {
        Serial.println("Uploading file: " + filename);
        SerialTwo.println("FILENAME:" + filename);
        delay(50);
      }
      SerialTwo.write(data, len);
      if (final) {
        SerialTwo.println("EOF");
        Serial.println("File sent to ESP32-B.");
      }
    }
  );

  // Download handler
  server.on("/download", HTTP_GET, [](AsyncWebServerRequest *request){
    if (!request->hasParam("file")) {
      request->send(400, "text/plain", "Missing file name");
      return;
    }
    expectedFileName = request->getParam("file")->value();
    fileDownloadBuffer = "";
    receivingFile = true;
    SerialTwo.println("GET:" + expectedFileName);

    // Delay response for 800ms to allow UART receive to complete
    delay(800);

    if (fileDownloadBuffer.length() > 0) {
      request->send(200, "text/plain", fileDownloadBuffer);
    } else {
      request->send(500, "text/plain", "Failed to download file");
    }
  });

  server.begin();
}

void loop() {
  while (SerialTwo.available()) {
    String line = SerialTwo.readStringUntil('\n');
    line.trim();

    if (line.startsWith("FILE:")) {
      String fname = line.substring(5);
      fileListHTML += "<p><a href='/download?file=" + fname + "'>" + fname + "</a></p>";
    } else if (line.startsWith("BEGIN:")) {
      String fname = line.substring(6);
      if (receivingFile && fname == expectedFileName) {
        fileDownloadBuffer = "";
      }
    } else if (line == "EOF") {
      receivingFile = false;
    } else if (receivingFile) {
      fileDownloadBuffer += line + "\n";
    }
  }
}