// Having issues here? See wifi_example.h!
#include "wifi.h"

#include "display.h"
#include "paint.h"
#include "pinout.h"
#include <Arduino.h>
#include <WiFi.h>

WiFiServer server(80); // Port 80 is the default for HTTP

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
  WiFi.begin(ssid, psk);
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
  unsigned char *image1 = (unsigned char *)malloc(DISPLAY_WIDTH * 4 * 24 / 2);
  Paint_NewImage(image1, DISPLAY_WIDTH, 4 * 24, 0, EPD_COLOR_WHITE);
  Paint_Clear(EPD_COLOR_WHITE);
  Paint_DrawString(0, 0, "Connected to:", &Font24, EPD_COLOR_WHITE,
                   EPD_COLOR_BLACK);
  Paint_DrawString(14 * 17, 0, ssid, &Font24, EPD_COLOR_WHITE, EPD_COLOR_BLACK);
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

void loop() {
  WiFiClient client = server.available();
  if (!client) {
    return;
  }
  printf("New client!\n");
  int timeout_max = 1000;
  int timeout = 0;
  printf("Waiting for client data");
  while (!client.available()) {
    delay(1);
    printf(".");
    timeout++;
    if (timeout > timeout_max) {
      printf("Timeout\n");
      return;
    }
  }
  printf("\nReciving data:\n");
  int i = 0;
  bool post = 0;
  bool get = 0;
  bool dataStart = false;
  int newline_count = 0;
  while (client.available()) {
    char c = client.read();

    if (i == 0 && c == 'P') {
      printf("POST\n");
      post = true;
    }
    if (i == 1 && c == 'U') {
      printf("NOT POST\n");
      post = false;
    }
    if (i == 0 && c == 'G') {
      printf("GET\n");
      get = true;
    }

    if (c == '\n' || c == '\r') {
      newline_count++;
    } else if (newline_count < 4) {
      newline_count = 0;
    }

    if (newline_count == 4 && post && !dataStart) {
      printf("End of headers\n");
      dataStart = true;
      EPD_Init();
      EPD_SendCommand(0x61);
      EPD_SendData(0x02);
      EPD_SendData(0x58);
      EPD_SendData(0x01);
      EPD_SendData(0xC0);
      EPD_SendCommand(0x10);
    }
    if (dataStart) {
      EPD_SendData(c);
    }
    i++;
  }

  printf("\n");
  printf("Sending response\n");

  if (post) {
    client.print("HTTP/1.1 200 OK\nContent-Type: text/html\n\n");
    client.stop();
    EPD_WaitUntilBusyHigh();
    EPD_SendCommand(0x04);
    EPD_WaitUntilBusyHigh();
    EPD_SendCommand(0x12);
    EPD_WaitUntilBusyHigh();
    EPD_SendCommand(0x02);
    EPD_WaitUntilBusyLow();
    printf("Done\n");
    return;
  }

  client.print("HTTP/1.1 200 OK\nContent-Type: text/html\n\n");
  client.print(
      "<!DOCTYPE html><html><head><title>wifi-doorsign</title></head>");
  client.print("<body>");
  client.print("<h1>wifi-doorsign ");
  client.print(WiFi.localIP());
  client.print("</h1>");
  client.print("<hr style=\"width:600px; margin-left:0px;\"/>");
  client.print("<div style=\"width: 600px; display: flex; flex-direction: "
               "row; gap: 2px;\">");
  client.print("<label for=\"text_fill_number\">Text fill color: </label>");
  client.print("<input type=\"number\" id=\"text_fill_number\" "
               "name=\"text_fill_number\" "
               "placeholder=\"170\" value=\"170\" min=\"0\" max=\"255\"/>");
  client.print(
      "<input type=\"range\" id=\"text_fill_range\" name=\"text_fill_range\" ");
  client.print("min=\"0\" max=\"255\" step=\"1\" value=\"170\" "
               "style=\"flex-grow:1;\"/>");
  client.print("</div>");

  client.print("<div style=\"width: 600px; display: flex; flex-direction: "
               "row; gap: 2px;\">");
  client.print(
      "<label for=\"text_outline_number\">Text outline color: </label>");
  client.print("<input type=\"number\" id=\"text_outline_number\" "
               "name=\"text_outline_number\" "
               "placeholder=\"170\" value=\"0\" min=\"0\" max=\"255\"/>");
  client.print("<input type=\"range\" id=\"text_outline_range\" "
               "name=\"text_outline_range\" ");
  client.print("min=\"0\" max=\"255\" step=\"1\" value=\"0\" "
               "style=\"flex-grow:1;\"/>");
  client.print("</div>");

  client.print("<hr style=\"width:600px; margin-left:0px;\"/>");
  client.print("<div style=\"width: 600px; display: flex; flex-direction: "
               "column; gap: 2px;\">");
  client.print("<label for=\"input_text_top\">Class/Course: </label>");
  client.print(
      "<input type=\"text\" id=\"input_text_top\" name=\"input_text_top\" "
      "placeholder=\"Computer Science\"/>");
  client.print("<label for=\"input_title\">Room: </label>");
  client.print("<input type=\"text\" id=\"input_title\" name=\"input_title\" "
               "placeholder=\"S12\"/>");
  client.print("<label for=\"input_text_bottom\">Extra info: </label>");
  client.print("<input type=\"text\" id=\"input_text_bottom\" "
               "name=\"input_text_bottom\" "
               "placeholder=\"13:45-14:35\" />");
  client.print("</div>");
  client.print("<hr style=\"width:600px; margin-left:0px;\"/>");
  client.print("<input style=\"margin-bottom: 4px\"type=\"file\" "
               "id=\"input_file\" name=\"input_file\" "
               "accept=\"image/*\" />");
  client.print("<img width=\"600\" height=\"448\" id=\"img\" src=\"\" "
               "alt=\"Source image\" style=\"display:none\" />");
  client.print("<br/>");
  client.print("<canvas id=\"canvas\" width=\"600\" height=\"448\" "
               "style=\"outline: black 1px solid\"></canvas>");
  client.print("<br/>");
  client.print("<div style=\"width: 600px; display: flex;\">");
  client.print("<button style=\"margin-right: 7px\" id=\"push_button\">Push to "
               "screen</button>");
  client.print("<progress style=\"flex-grow: 1\" id=\"progress\" value=\"0\" "
               "max=\"100\"></progress>");
  client.print("</div>");
  client.print("<script>");

  client.print("var palette = "
               "[[0,0,0],[255,255,255],[0,255,0],[0,0,255],[255,0,0],[255,255,"
               "0],[255,128,0]];");

  // set input listener
  client.print("document.getElementById('input_file').addEventListener('change'"
               ", function(e) {");
  client.print("var file = e.target.files[0];");
  client.print("var reader = new FileReader();");
  client.print("reader.onload = function(e) {");
  client.print("document.getElementById('img').src = e.target.result;");
  client.print("};");
  client.print("reader.readAsDataURL(file);");
  client.print("img.onload = function() {");
  client.print("updateImage(img);");
  client.print("};");
  client.print("});");

  client.print(
      "document.getElementById('push_button').addEventListener('click', "
      "function() {");
  client.print("pushImage(document.getElementById('canvas'));");
  client.print("});");

  client.print(
      "document.getElementById('input_title').addEventListener('input', "
      "function() {");
  client.print("updateImage(document.getElementById('img'));");
  client.print("});");
  client.print(
      "document.getElementById('input_text_top').addEventListener('input', "
      "function() {");
  client.print("updateImage(document.getElementById('img'));");
  client.print("});");
  client.print(
      "document.getElementById('input_text_bottom').addEventListener('input', "
      "function() {");
  client.print("updateImage(document.getElementById('img'));");
  client.print("});");

  client.print(
      "document.getElementById('text_fill_number').addEventListener('input'"
      ", function() {");
  client.print("document.getElementById('text_fill_range').value = "
               "document.getElementById('text_fill_number').value;");
  client.print("updateImage(document.getElementById('img'));");
  client.print("});");

  client.print(
      "document.getElementById('text_fill_range').addEventListener('input'"
      ", function() {");
  client.print("document.getElementById('text_fill_number').value = "
               "document.getElementById('text_fill_range').value;");
  client.print("updateImage(document.getElementById('img'));");
  client.print("});");

  client.print(
      "document.getElementById('text_outline_number').addEventListener('input'"
      ", function() {");
  client.print("document.getElementById('text_outline_range').value = "
               "document.getElementById('text_outline_number').value;");
  client.print("updateImage(document.getElementById('img'));");
  client.print("});");

  client.print(
      "document.getElementById('text_outline_range').addEventListener('input'"
      ", function() {");
  client.print("document.getElementById('text_outline_number').value = "
               "document.getElementById('text_outline_range').value;");
  client.print("updateImage(document.getElementById('img'));");
  client.print("});");

  client.print("function updateImage(img) {");
  client.print("var width = 600;");
  client.print("var height = 448;");
  client.print("var text = document.getElementById('input_title').value;");
  client.print(
      "var top_text = document.getElementById('input_text_top').value;");
  client.print(
      "var bottom_text = document.getElementById('input_text_bottom').value;");
  client.print("var canvas = document.getElementById('canvas');");
  client.print("var ctx = canvas.getContext('2d');");
  client.print("canvas.width = width;");
  client.print("canvas.height = height;");
  client.print("ctx.drawImage(img, 0, 0,width,height);");
  client.print("ctx.font = '288px Ubuntu Mono';");
  client.print("var textBrightness = "
               "document.getElementById('text_fill_number').valueAsNumber;");
  client.print("var outlineBrightness = "
               "document.getElementById('text_outline_number').valueAsNumber;");
  client.print("ctx.fillStyle = "
               "\"#\" + textBrightness.toString(16).repeat(3) + \"ff\";");
  client.print("ctx.strokeStyle = "
               "\"#\" + outlineBrightness.toString(16).repeat(3) + \"ff\";");
  client.print("ctx.lineWidth = 8;");
  client.print("ctx.globalCompositeOperation = 'luminosity';");
  client.print("ctx.fillText(text, 300 - "
               "ctx.measureText(text).width/2"
               ",448-144);");
  client.print("ctx.font = '50px Ubuntu Mono';");
  client.print("ctx.lineWidth = 2;");
  client.print("ctx.fillText(top_text, 300 - "
               "ctx.measureText(top_text).width/2"
               ",75);");
  client.print("ctx.fillText(bottom_text, 300 - "
               "ctx.measureText(bottom_text).width/2"
               ",448-50);");
  client.print("ctx.globalCompositeOperation = 'source-over';");
  client.print("ctx.font = '288px Ubuntu Mono';");
  client.print("ctx.lineWidth = 8;");
  client.print("ctx.strokeText(text, 300 - "
               "ctx.measureText(text).width/2"
               ",448-144);");
  client.print("ctx.font = '50px Ubuntu Mono';");
  client.print("ctx.lineWidth = 2;");
  client.print("ctx.strokeText(top_text, 300 - "
               "ctx.measureText(top_text).width/2"
               ",75);");
  client.print("ctx.strokeText(bottom_text, 300 - "
               "ctx.measureText(bottom_text).width/2"
               ",448-50);");
  client.print("var new_canvas = seven_color_dither(ctx);");
  client.print("ctx.drawImage(new_canvas, 0, 0);");
  client.print("}");

  // push image to device function
  client.print("function pushImage(canvas) {");
  client.print("var ctx = canvas.getContext('2d');");
  client.print("var imgData = ctx.getImageData(0, 0, 600, 448);");
  client.print("var data = imgData.data;");
  client.print("var textify = '';");
  client.print("for (var i = 0; i < data.length; i += 8) {");
  client.print("var cha = 0x00;");
  client.print("var r1 = data[i];");
  client.print("var g1 = data[i + 1];");
  client.print("var b1 = data[i + 2];");
  client.print("var ind1 = getNear(r1,g1,b1);");
  client.print("cha |= ind1 << 4;");
  client.print("var r2 = data[i + 4];");
  client.print("var g2 = data[i + 5];");
  client.print("var b2 = data[i + 6];");
  client.print("var ind2 = getNear(r2,g2,b2);");
  client.print("cha |= ind2;");
  client.print("textify += String.fromCharCode(cha);");
  client.print("}");
  client.print("var xhr = new XMLHttpRequest();");
  client.print("xhr.open('POST', '/image', true);");
  client.print("xhr.setRequestHeader('Content-Type', 'application/octet-"
               "stream');");
  client.print("document.getElementById('progress').value = 0;");
  client.print("document.getElementById('progress').style.accentColor = "
               "\"auto\";");
  client.print("xhr.upload.onprogress = function(e) {");
  client.print("if (e.lengthComputable) {");
  client.print("var percentComplete = (e.loaded / e.total) * 100;");
  client.print("document.getElementById('progress').value = percentComplete;");
  client.print("}");
  client.print("};");
  client.print("xhr.onload = function() {");
  client.print("document.getElementById('progress').value = 100;");
  client.print(
      "document.getElementById('progress').style.accentColor = \"green\";");
  client.print("};");
  client.print("xhr.send(textify);");
  client.print("}");

  // addVal function
  client.print("function addVal(c,r,g,b,k){");
  client.print("return[c[0]+(r*k)/32,c[1]+(g*k)/32,c[2]+(b*k)/32];");
  client.print("}");

  // getNear function
  client.print("function getNear(r,g,b) {");
  client.print("var ind= 0;");
  client.print("var err= 1000000;");
  client.print("for (var i = 0; i < 7; i++) {");
  client.print("var curErr = "
               "(r-palette[i][0])*(r-palette[i][0])+(g-palette[i][1])*(g-"
               "palette[i][1])+(b-palette[i][2])*(b-palette[i][2]);");
  client.print("if (curErr < err) {");
  client.print("err = curErr;");
  client.print("ind = i;");
  client.print("}");
  client.print("}");
  client.print("return ind;");
  client.print("}");

  // seven_color_dither function
  client.print("function seven_color_dither(ctx) {");
  client.print("var width = 600;");
  client.print("var height = 448;");
  client.print("var index = 0;");
  client.print("var pSrc = ctx.getImageData(0, 0, width, height);");
  client.print("var pDst = ctx.createImageData(width, height);");
  client.print("var aInd = 0;");
  client.print("var bInd = 1;");
  client.print("var errArr = new Array(2);");
  client.print("errArr[0] = new Array(width);");
  client.print("errArr[1] = new Array(width);");
  client.print("for (var i = 0; i < width; i++) {");
  client.print("errArr[bInd][i] = [0,0,0];");
  client.print("}");
  client.print("for (var y = 0; y < height; y++) {");
  client.print("aInd = ((bInd=aInd)+1)&1;");
  client.print("for (var x = 0; x < width; x++) {");
  client.print("errArr[bInd][x] = [0,0,0];");
  client.print("}");
  client.print("for (var x = 0; x < width; x++) {");
  client.print("var pos = (y * width + x) * 4;");
  client.print("var old = errArr[aInd][x];");
  client.print("var r = pSrc.data[pos] + old[0];");
  client.print("var g = pSrc.data[pos + 1] + old[1];");
  client.print("var b = pSrc.data[pos + 2] + old[2];");
  client.print("var colVal = palette[getNear(r,g,b)];");
  client.print("pDst.data[index++] = colVal[0];");
  client.print("pDst.data[index++] = colVal[1];");
  client.print("pDst.data[index++] = colVal[2];");
  client.print("pDst.data[index++] = 255;");
  client.print("r = (r - colVal[0]);");
  client.print("g = (g - colVal[1]);");
  client.print("b = (b - colVal[2]);");
  client.print("if (x ==0) {");
  client.print("errArr[bInd][x  ]=addVal(errArr[bInd][x  ],r,g,b,7.0);");
  client.print("errArr[bInd][x+1]=addVal(errArr[bInd][x+1],r,g,b,2.0);");
  client.print("errArr[aInd][x+1]=addVal(errArr[aInd][x+1],r,g,b,7.0);");
  client.print("} else if (x == width - 1) {");
  client.print("errArr[bInd][x-1]=addVal(errArr[bInd][x-1],r,g,b,7.0);");
  client.print("errArr[bInd][x  ]=addVal(errArr[bInd][x  ],r,g,b,9.0);");
  client.print("} else {");
  client.print("errArr[bInd][x-1]=addVal(errArr[bInd][x-1],r,g,b,3.0);");
  client.print("errArr[bInd][x  ]=addVal(errArr[bInd][x  ],r,g,b,5.0);");
  client.print("errArr[bInd][x+1]=addVal(errArr[bInd][x+1],r,g,b,1.0);");
  client.print("errArr[aInd][x+1]=addVal(errArr[aInd][x+1],r,g,b,7.0);");
  client.print("}");
  client.print("}");
  client.print("}");
  client.print("ctx.putImageData(pDst, 0, 0);");
  client.print("return canvas;");
  client.print("}");
  client.print("</script>");
  client.print("</body></html>");
  client.stop();
}
