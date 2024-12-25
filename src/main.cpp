// Having issues compiling? See wifi_example.h!
#include "wifi.h"

#include "display.h"
#include "paint.h"
#include "pinout.h"
#include <Arduino.h>
#include <WiFi.h>

WiFiServer server(80); // Port 80 is the default for HTTP

void setup() {
  Serial.begin(115200);

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
  if (true) {
    // Write IP address to display.

    // We don't have enough continuous memory to store the whole display buffer,
    // so we only store 4 lines of text. (24 pixels/line)

    // Each pixel takes up 4 bits - so we divide by 2.
    unsigned char *infoTextImage =
        (unsigned char *)malloc(DISPLAY_WIDTH * 4 * 24 / 2);

    // Create a new image with a white background.
    Paint_NewImage(infoTextImage, DISPLAY_WIDTH, 4 * 24, 0, EPD_COLOR_WHITE);
    Paint_Clear(EPD_COLOR_WHITE);

    // Characters are 17 pixels wide, lines are 24 pixels tall.
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

    // Prepare the display for receiving data.
    // These commands are listed as necessary in the datasheet, with no
    // explanation, but they work!
    EPD_SendCommand(0x61);
    EPD_SendData(0x02);
    EPD_SendData(0x58);
    EPD_SendData(0x01);
    EPD_SendData(0xC0);
    EPD_SendCommand(0x10);

    // Pixels are stored in 4-bit format, with the top 4 bits being the left
    // pixel and the bottom 4 bits being the right pixel.
    for (uint32_t i = 0; i < DISPLAY_HEIGHT; i++) {
      for (uint32_t j = 0; j < DISPLAY_WIDTH / 2; j++) {
        if (i < 24 * 4) {
          // If we are within the image buffer, send it's data.
          EPD_SendData(infoTextImage[j + i * DISPLAY_WIDTH / 2]);
        } else {
          // Generate color bars. Each bar is 100 pixels wide (50 bytes).
          // See display.h for color definitions.
          if (j < 50) {
            EPD_SendData(0x00); // Two black pixels. (0000 and 0000)
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
    // Another case of necessary-but-unexplained commands.
    // This takes a *long* time as we wait for the whole display to update here.
    EPD_SendCommand(0x04);
    EPD_WaitUntilBusyHigh();
    EPD_SendCommand(0x12);
    EPD_WaitUntilBusyHigh();
    EPD_SendCommand(0x02);
    EPD_WaitUntilBusyLow();

    // Sleep the display to save power.
    printf("Sleeping\n");
    EPD_Sleep();

    // Free the image buffer.
    free(infoTextImage);
    infoTextImage = NULL;
  }
}

// Run the web server.
void loop() {
  WiFiClient client = server.available();
  if (!client) {
    // No client connected, check again.
    return;
  }
  printf("New client connected!\n");

  printf("Waiting for client request");
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

  printf("\nReading client request: ");

  bool isPost = false;
  bool isGet = false;
  bool headersComplete = false;
  int consecutiveNewlines = 0;
  int i = 0;

  // While there is data available, read it.
  while (client.available()) {
    // Read one byte at a time.
    char c = client.read();

    // If the first character is a P, assume it's a POST request.
    if (i == 0 && c == 'P') {
      printf("POST\n");
      isPost = true;
    }

    // If the first character is a G, assume it's a GET request.
    if (i == 0 && c == 'G') {
      printf("GET\n");
      isGet = true;
    }

    // Headers end with two newlines (\r\n\r\n).
    if (c == '\n' || c == '\r') {
      consecutiveNewlines++;
    } else {
      consecutiveNewlines = 0;
    }

    if (consecutiveNewlines == 4 && !headersComplete) {
      headersComplete = true;
      // POST requests are sending us display bytes, so we need to prepare the
      // display.
      if (isPost) {
        EPD_Init();
        EPD_SendCommand(0x61);
        EPD_SendData(0x02);
        EPD_SendData(0x58);
        EPD_SendData(0x01);
        EPD_SendData(0xC0);
        EPD_SendCommand(0x10);
      }
      // GET requests aren't sending us data after the headers, so we don't need
      // to do anything here.
    }

    if (headersComplete && isPost) {
      // Stream data to the display's framebuffer.
      EPD_SendData(c);
    }

    // Increment the character index.
    i++;
  }

  printf("Sending response.\n");

  if (isPost) {
    // End response with a 200 OK.
    client.print("HTTP/1.1 200 OK\nContent-Type: text/html\n\n");
    client.stop();

    // Display the image in framebuffer.
    // This takes a while, server will be unresponsive.
    EPD_WaitUntilBusyHigh();
    EPD_SendCommand(0x04);
    EPD_WaitUntilBusyHigh();
    EPD_SendCommand(0x12);
    EPD_WaitUntilBusyHigh();
    EPD_SendCommand(0x02);
    EPD_WaitUntilBusyLow();

    // Nothing else to do, so return.
    return;
  }

  // GET request, send the HTML page.

  // Respond with 200 OK and HTML content type.
  client.print("HTTP/1.1 200 OK\nContent-Type: text/html\n\n");

  // Good luck reading this HTML and JS in C++, it's the best I could do!

  // DOCTYPE and head.
  client.print(
      "<!DOCTYPE html><html><head><title>wifi-doorsign</title><meta "
      "name=\"viewport\" content=\"width=device-width, "
      "initial-scale=1.0\">"
      "<link rel=\"preconnect\" href=\"https://fonts.googleapis.com\">"
      "<link rel=\"preconnect\" href=\"https://fonts.gstatic.com\" crossorigin>"
      "<link "
      "href=\"https://fonts.googleapis.com/"
      "css2?family=Inter:ital,opsz,wght@0,14..32,100..900;1,14..32,100..900&"
      "family=Ubuntu+Mono:ital,wght@0,400;0,700;1,400;1,700&display=swap\" "
      "rel=\"stylesheet\">"
      "</head>");
  client.print(
      "<body style=\"display:flex; justify-content:center; font-family:"
      " 'Inter', sans-serif; background-color: Canvas; color: CanvasText; "
      "color-scheme: light dark;\">");
  client.print("<style>");
  // client.print("a { color: #62a0ea; }");
  // client.print("a:hover { color: #99c1f1; }");
  client.print("</style>");
  client.print("<div style=\"max-width:1000px; min-width: 300px;\">");
  client.print("<h1 style=\"text-align:center;\">wifi-doorsign ");
  // Add our IP to the title, to make it very obvious which device we're
  // interacting with.
  client.print(WiFi.localIP());
  client.print("</h1>");
  client.print("<small>Can't find which display you're talking to? <a "
               "id=\"identify_button\" href=\"#\">"
               "Identify.</a></small>");
  client.print("<hr style=\"margin-left:0px;\"/>");
  // --- Text fill color input (number and range)
  client.print("<form style=\"display: flex; flex-direction: "
               "column; gap: 4px;\">");
  client.print("<div style=\"display: flex; flex-direction: "
               "row; gap: 2px;\">");
  client.print("<label for=\"text_fill_number\" style=\"align-content: "
               "center;width: 150px;\">Text fill color: </label>");
  client.print("<input type=\"number\" id=\"text_fill_number\" "
               "name=\"text_fill_number\" "
               "placeholder=\"170\" value=\"170\" min=\"0\" max=\"255\"/>");
  client.print(
      "<input type=\"range\" id=\"text_fill_range\" name=\"text_fill_range\" ");
  client.print("min=\"0\" max=\"255\" step=\"1\" value=\"170\" "
               "style=\"flex-grow:1;\"/>");
  client.print("</div>");
  // --- Text outline color input (number and range)
  client.print("<div style=\"display: flex; flex-direction: "
               "row; gap: 2px;\">");
  client.print("<label for=\"text_outline_number\" style=\"align-content: "
               "center; width: 150px;\">Text outline color: </label>");
  client.print("<input type=\"number\" id=\"text_outline_number\" "
               "name=\"text_outline_number\" "
               "placeholder=\"170\" value=\"0\" min=\"0\" max=\"255\"/>");
  client.print("<input type=\"range\" id=\"text_outline_range\" "
               "name=\"text_outline_range\" ");
  client.print("min=\"0\" max=\"255\" step=\"1\" value=\"0\" "
               "style=\"flex-grow:1;\"/>");
  client.print("</div>");
  client.print("</form>");
  client.print("<hr style=\"margin-left:0px;\"/>");
  // --- Text input fields
  client.print("<form style=\"display: flex; flex-direction: "
               "column; gap: 4px;\">");
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
  client.print("</form>");
  client.print("<hr style=\"margin-left:0px;\"/>");
  // --- Color selection button
  client.print("<form style=\"display: flex; flex-direction: "
               "row; align-items: center;\">");
  client.print("<label for=\"input_color\">Background color: </label>");
  client.print("<input type=\"color\" id=\"input_color\" name=\"input_color\" "
               "value=\"#ffffff\" style=\"margin-left:10px;\" />");
  client.print("</form>");
  client.print("<hr style=\"margin-left:0px;\"/>");
  // --- File selection button
  client.print("<div style=\"display: flex; flex-direction: "
               "row;\">");
  client.print("<form>");
  client.print(
      "<input style=\"margin-bottom: 6px; min-width: 0px;\"type=\"file\" "
      "id=\"input_file\" name=\"input_file\" "
      "accept=\"image/*\" />");
  client.print("</form>");
  // --- Display resolution reminder
  client.print("<div style=\"text-align: right; flex-grow:1; align-content: "
               "end;\">");
  client.print("<small style=\"padding-left:2px;padding-right:2px;"
               "\">600x448px</small>");
  client.print("</div>");
  client.print("</div>");
  // --- The Mighty Canvas, Storer of Image Data.
  client.print("<div>");
  client.print(
      "<canvas id=\"canvas\" width=\"600\" height=\"448\" "
      "style=\"outline: ButtonBorder 1px solid; width:100%;\"></canvas>");
  client.print("</div>");
  // --- Button to send canvas image to display, and progress bar.
  client.print("<div style=\"display: flex; margin-top:6px;\">");
  client.print("<button style=\"margin-right: 7px\" id=\"push_button\">Push to "
               "screen</button>");
  client.print("<progress style=\"flex-grow: 1\" id=\"progress\" value=\"0\" "
               "max=\"100\"></progress>");
  client.print("</div>");
  client.print("<hr style=\"margin-left:0px;\"/>");
  // --- Footer
  client.print(
      "<small>Built by Edward Hesketh, open source at <a "
      "href=\"https://github.com/headblockhead/"
      "wifi-doorsign\">github:headblockhead/wifi-doorsign</a></small>");
  client.print("</div>");
  // This img tag is hidden, and stores the original selected image as a
  // reference.
  client.print("<img width=\"600\" height=\"448\" id=\"img\" src=\"\" "
               "alt=\"Source image\" style=\"display:none\" />");

  // JS time!
  client.print("<script>");

  // Define the RGB values of the colors used in the display.
  client.print("var palette = "
               "[[0,0,0],[255,255,255],[0,255,0],[0,0,255],[255,0,"
               "0],[255,255,"
               "0],[255,128,0]];");

  // Load the font.
  client.print("var Font24 = new FontFace('Ubuntu Mono', "
               "'url(https://fonts.gstatic.com/s/"
               "ubuntumono/v17/KFO-CneDtsqEr0keqCMhbC-BL9H1tY0.woff2)');"
               "Font24.load().then(function(font) {"
               "document.fonts.add(font);"
               "});");

  // When a file is selected,
  client.print("document.getElementById('input_file')."
               "addEventListener('change'"
               ", function(e) {");
  client.print("var file = e.target.files[0];");
  client.print("var reader = new FileReader();");
  client.print("reader.onload = function(e) {");
  // Set the invisible image's source to the selected file.
  client.print("document.getElementById('img').src = e.target.result;");
  client.print("};");
  client.print("reader.readAsDataURL(file);");
  client.print("img.onload = function() {");
  // When the image is loaded, update the canvas.
  client.print("updateImage(img);");
  client.print("};");
  client.print("});");

  // Assign actions to the buttons.

  // This button sends the image to the display.
  client.print("document.getElementById('push_button')."
               "addEventListener('click', "
               "function() {");
  client.print("pushImage(document.getElementById('canvas'));");
  client.print("});");

  // This button draws the IP of the display on the canvas very
  // large, then sends that image to the display.
  client.print("document.getElementById('identify_button')."
               "addEventListener('click', "
               "function() {");
  client.print("event.preventDefault();");
  client.print("var canvas = document.getElementById('canvas');");
  client.print("var ctx = canvas.getContext('2d');");
  client.print("canvas.width = 600;");
  client.print("canvas.height = 448;");
  client.print("ctx.font = '48px Ubuntu Mono';");
  client.print("ctx.fillStyle = '#ffffff';");
  client.print("ctx.strokeStyle = '#000000';");
  client.print("ctx.fillText('");
  client.print(WiFi.localIP());
  client.print("', 300 - ctx.measureText('");
  client.print(WiFi.localIP());
  client.print("').width/2,448-48);");
  client.print("ctx.font = '256px Ubuntu Mono';");
  client.print("ctx.fillText('IDENT', 300 - "
               "ctx.measureText('IDENT').width/2,"
               "448-192);");
  client.print("var new_canvas = seven_color_dither(ctx);");
  client.print("ctx.drawImage(new_canvas, 0, 0);");
  client.print("document.getElementById('push_button').focus({"
               "focusVisible: true});");
  client.print("document.getElementById('push_button').scrollIntoView();");
  client.print("});");

  // Update the image live as changes are made to the various text
  // fields.

  client.print("document.getElementById('input_title')."
               "addEventListener('input', "
               "function() {");
  client.print("updateImage(document.getElementById('img'));");
  client.print("});");
  client.print("document.getElementById('input_text_top')."
               "addEventListener('input', "
               "function() {");
  client.print("updateImage(document.getElementById('img'));");
  client.print("});");
  client.print("document.getElementById('input_text_bottom')."
               "addEventListener('input', "
               "function() {");
  client.print("updateImage(document.getElementById('img'));");
  client.print("});");

  // Update the image when the color input changes.
  client.print("document.getElementById('input_color')."
               "addEventListener('input', "
               "function() {");
  client.print("updateImage(document.getElementById('img'));");
  client.print("});");

  // Match the number input to the range input, and vice versa.
  // Also, update the image when the value changes.

  client.print("document.getElementById('text_fill_number')."
               "addEventListener('input'"
               ", function() {");
  client.print("document.getElementById('text_fill_range').value = "
               "document.getElementById('text_fill_number').value;");
  client.print("updateImage(document.getElementById('img'));");
  client.print("});");

  client.print("document.getElementById('text_fill_range')."
               "addEventListener('input'"
               ", function() {");
  client.print("document.getElementById('text_fill_number').value = "
               "document.getElementById('text_fill_range').value;");
  client.print("updateImage(document.getElementById('img'));");
  client.print("});");

  client.print("document.getElementById('text_outline_number')."
               "addEventListener('input'"
               ", function() {");
  client.print("document.getElementById('text_outline_range').value = "
               "document.getElementById('text_outline_number').value;");
  client.print("updateImage(document.getElementById('img'));");
  client.print("});");

  client.print("document.getElementById('text_outline_range')."
               "addEventListener('input'"
               ", function() {");
  client.print("document.getElementById('text_outline_number').value = "
               "document.getElementById('text_outline_range').value;");
  client.print("updateImage(document.getElementById('img'));");
  client.print("});");

  // updateImage draws the canvas image from the source image,
  // adding text.
  client.print("function updateImage(img) {");
  client.print("var width = 600;");
  client.print("var height = 448;");
  client.print("var text = document.getElementById('input_title').value;");
  client.print("var top_text = "
               "document.getElementById('input_text_top').value;");
  client.print("var bottom_text = "
               "document.getElementById('input_text_bottom').value;");
  client.print("var textBrightness = "
               "document.getElementById('text_fill_number')."
               "valueAsNumber;");
  client.print("var outlineBrightness = "
               "document.getElementById('text_outline_number')."
               "valueAsNumber;");
  client.print("var canvas = document.getElementById('canvas');");
  client.print("var ctx = canvas.getContext('2d');");
  client.print("canvas.width = width;");
  client.print("canvas.height = height;");

  // Fill the canvas with the background color.
  client.print("ctx.fillStyle = "
               "document.getElementById('input_color').value;");
  client.print("ctx.fillRect(0, 0, width, height);");

  // Draw the source image on the canvas, stretching/squeezing to
  // fit if needed.
  client.print("ctx.drawImage(img, 0, 0,width,height);");

  // Set the font to draw the text with.
  client.print("ctx.font = '288px Ubuntu Mono';");

  // Set the fill and stroke colors for the text.
  client.print("ctx.fillStyle = "
               "\"#\" + textBrightness.toString(16).repeat(3) + \"ff\";");
  client.print("ctx.strokeStyle = "
               "\"#\" + outlineBrightness.toString(16).repeat(3) "
               "+ \"ff\";");

  // When drawing the fill, preserve the hue and chroma of the
  // bottom layer, while adopting the luma of the top layer.
  client.print("ctx.globalCompositeOperation = 'luminosity';");

  // Draw the title text's fill.
  client.print("ctx.fillText(text, 300 - "
               "ctx.measureText(text).width/2"
               ",448-144);");

  // Set the font for the top and bottom text.
  client.print("ctx.font = '50px Ubuntu Mono';");

  // Draw the top and bottom texts' fill.
  client.print("ctx.fillText(top_text, 300 - "
               "ctx.measureText(top_text).width/2"
               ",75);");
  client.print("ctx.fillText(bottom_text, 300 - "
               "ctx.measureText(bottom_text).width/2"
               ",448-50);");

  // When drawing the outline, replace pixels beneath.
  client.print("ctx.globalCompositeOperation = 'source-over';");

  // Draw the title text's outline.
  client.print("ctx.font = '288px Ubuntu Mono';");
  client.print("ctx.lineWidth = 8;");
  client.print("ctx.strokeText(text, 300 - "
               "ctx.measureText(text).width/2"
               ",448-144);");

  // Draw the top and bottom texts' outline.
  client.print("ctx.font = '50px Ubuntu Mono';");
  client.print("ctx.lineWidth = 2;");
  client.print("ctx.strokeText(top_text, 300 - "
               "ctx.measureText(top_text).width/2"
               ",75);");
  client.print("ctx.strokeText(bottom_text, 300 - "
               "ctx.measureText(bottom_text).width/2"
               ",448-50);");

  // Dither the canvas image, then replace the current image with
  // the dithered version.
  client.print("var new_canvas = seven_color_dither(ctx);");
  client.print("ctx.drawImage(new_canvas, 0, 0);");
  client.print("}");

  // pushImage converts the canvas contents into bytes, that are
  // then sent to the display.
  client.print("function pushImage(canvas) {");
  client.print("var ctx = canvas.getContext('2d');");
  // Get image data as RGBA bytes.
  client.print("var data = ctx.getImageData(0, 0, 600, 448).data;");
  // Store the image data in a string, with each byte representing
  // two pixels.
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

  // Prepare a POST request to /image.
  client.print("var xhr = new XMLHttpRequest();");
  client.print("xhr.open('POST', '/image', true);");
  client.print("xhr.setRequestHeader('Content-Type', 'application/octet-"
               "stream');");

  // Set the progress bar to 0, and the color to the default.
  client.print("document.getElementById('progress').value = 0;");
  client.print("document.getElementById('progress').style.accentColor = "
               "\"auto\";");

  // When the upload progress changes, update the progress bar.
  client.print("xhr.upload.onprogress = function(e) {");
  client.print("if (e.lengthComputable) {");
  client.print("var percentComplete = (e.loaded / e.total) * 100;");
  client.print("document.getElementById('progress').value = "
               "percentComplete;");
  client.print("}");
  client.print("};");

  // When uploading finishes, set the progress bar's color to
  // green.
  client.print("xhr.onload = function() {");
  client.print("document.getElementById('progress').value = 100;");
  client.print("document.getElementById('progress').style."
               "accentColor = \"green\";");
  client.print("updateImage(document.getElementById('img'));");
  client.print("document.getElementById('push_button').blur();");
  client.print("};");

  // Send the POST request with the image data!
  client.print("xhr.send(textify);");
  client.print("}");

  // A few helper functions provided helpfully by Waveshare.
  client.print("function addVal(c,r,g,b,k){");
  client.print("return[c[0]+(r*k)/32,c[1]+(g*k)/32,c[2]+(b*k)/32];");
  client.print("}");
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

  // seven_color_dither function, provided by Waveshare, and
  // simplified by me.
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

  // Close the HTML tags.
  client.print("</body></html>");

  // It's *finally* over, close the connection.
  client.stop();

  // We're done here.
}
