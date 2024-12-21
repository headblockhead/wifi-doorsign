#include "display.h"
#include "paint.h"
#include "pinout.h"
#include <Arduino.h>
#include <WiFi.h>

const char *ssid = "";
const char *password = "";

IPAddress staticIP(192, 168, 1, 4);
IPAddress gateway(192, 168, 1, 1);
IPAddress subnet(255, 255, 255, 0);
IPAddress dns(192, 168, 1, 1);

WiFiServer server(80); // Port 80 is the default for HTTP
IPAddress myIP;

void setup() {
  Serial.begin(115200);
  printf("Starting...\n");

  printf("Setting up SPI\n");
  pinMode(PIN_SPI_BUSY, INPUT);
  pinMode(PIN_SPI_RST, OUTPUT);
  pinMode(PIN_SPI_DC, OUTPUT);
  pinMode(PIN_SPI_SCK, OUTPUT);
  digitalWrite(PIN_SPI_SCK, 0);
  pinMode(PIN_SPI_DIN, OUTPUT);
  pinMode(PIN_SPI_CS, OUTPUT);
  digitalWrite(PIN_SPI_CS, 1);

  printf("Initializing EPD\n");
  EPD_Init();

  printf("Connecting to %s", ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    printf(".");
  }
  printf("\nConnected to: %s\n", ssid);
  printf("IP address: %s\n", WiFi.localIP().toString().c_str());
  printf("Gateway: %s\n", WiFi.gatewayIP().toString().c_str());
  printf("Subnet: %s\n", WiFi.subnetMask().toString().c_str());

  printf("Starting server\n");
  server.begin();
  printf("Server started\n");

  // Write IP address to display
  if (false) {
    unsigned char *image1 = (unsigned char *)malloc(DISPLAY_WIDTH * 4 * 24 / 2);
    Paint_NewImage(image1, DISPLAY_WIDTH, 4 * 24, 0, EPD_COLOR_WHITE);
    Paint_Clear(EPD_COLOR_WHITE);
    Paint_DrawString(0, 0, "Connected to:", &Font24, EPD_COLOR_WHITE,
                     EPD_COLOR_BLACK);
    Paint_DrawString(14 * 17, 0, ssid, &Font24, EPD_COLOR_WHITE,
                     EPD_COLOR_BLACK);
    Paint_DrawString(0, 24 * 1, "IP address:", &Font24, EPD_COLOR_WHITE,
                     EPD_COLOR_BLACK);
    Paint_DrawString(12 * 17, 24, WiFi.localIP().toString().c_str(), &Font24,
                     EPD_COLOR_WHITE, EPD_COLOR_BLACK);
    Paint_DrawString(0, 24 * 2, "Gateway:", &Font24, EPD_COLOR_WHITE,
                     EPD_COLOR_BLACK);
    Paint_DrawString(9 * 17, 48, WiFi.gatewayIP().toString().c_str(), &Font24,
                     EPD_COLOR_WHITE, EPD_COLOR_BLACK);
    Paint_DrawString(0, 24 * 3, "Subnet:", &Font24, EPD_COLOR_WHITE,
                     EPD_COLOR_BLACK);
    Paint_DrawString(8 * 17, 24 * 3, WiFi.subnetMask().toString().c_str(),
                     &Font24, EPD_COLOR_WHITE, EPD_COLOR_BLACK);

    printf("Sending to EPD\n");
    EPD_SendCommand(0x61);
    EPD_SendData(0x02);
    EPD_SendData(0x58);
    EPD_SendData(0x01);
    EPD_SendData(0xC0);
    EPD_SendCommand(0x10);
    for (uint32_t i = 0; i < DISPLAY_HEIGHT; i++) {
      for (uint32_t j = 0; j < DISPLAY_WIDTH / 2; j++) {
        if (i < 24 * 4) {
          EPD_SendData(image1[j + i * DISPLAY_WIDTH / 2]);
        } else {
          // Generate color bars.
          if (j < 50) {
            EPD_SendData(0x00);
          } else if (j < 100) {
            EPD_SendData(0x22);
          } else if (j < 150) {
            EPD_SendData(0x33);
          } else if (j < 200) {
            EPD_SendData(0x44);
          } else if (j < 250) {
            EPD_SendData(0x55);
          } else if (j < 300) {
            EPD_SendData(0x66);
          }
        }
      }
    }

    printf("Displaying\n");
    EPD_SendCommand(0x04);
    EPD_WaitUntilBusyHigh();
    EPD_SendCommand(0x12);
    EPD_WaitUntilBusyHigh();
    EPD_SendCommand(0x02);
    EPD_WaitUntilBusyLow();

    printf("Sleeping\n");
    EPD_Sleep();
  }
}

void loop() {
  WiFiClient client = server.available();
  if (!client) {
    return;
  }
  printf("New client\n");
  int timeout_max = 1000;
  int timeout = 0;
  while (!client.available()) {
    delay(1);
    printf(".");
    timeout++;
    if (timeout > timeout_max) {
      printf("Timeout\n");
      return;
    }
  }
  while (client.available()) {
    char c = client.read();
    printf("%c", c);
  }
  client.print("HTTP/1.1 200 OK\nContent-Type: text/html\n\n");
  client.print(
      // ---
      "<!DOCTYPE html>"
      "<html>"
      "<head><title>wifi-doorsign</title></head>"
      "<body>"
      "<h1>WiFi Doorsign</h1>"
      "<input type=\"file\" id=\"input_file\" "
      "onchange=\"handleFileSelect(this)\" />"
      "Input:"
      "<img id=\"input_img\" src=\"\" />"
      "Output:"
      "<img width=\"600\" height=\"448\" id=\"output_img\" src=\"\" />"
      "<br/>"
      // ---
      "<script>"
      // ---
      "function handleFileSelect(input) {"
      /**/ "if (input.files && input.files[0]) {"
      /*  */ "var reader = new FileReader();"
      /*  */ "reader.onload = function(e) { "
      /*     */ "document.getElementById('input_img').setAttribute("
      /*    */ "\"src\",e.target.result);"
      /*  */ "};"
      /*  */ "reader.readAsDataURL(input.files[0]);"
      /**/ "}"
      "}"
      // ---
      "</script>"
      // ---
      "</body>"
      "</html>"
      // ---
  );
}
