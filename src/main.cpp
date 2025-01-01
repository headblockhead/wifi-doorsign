// Having issues compiling? See wifi_example.h!
#include "secrets.h"

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

  // if true, display IP address on the screen.
  // this should ideally be disabled during development as this blocks the web
  // server.

#if true

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

#endif // endif for display IP block.

  printf("Starting server\n");
  server.begin();
  printf("Server started\n");
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

  bool isAuthed = false;
  bool isPost = false;
  bool isGet = false;
  bool readingData = false;

  // While there is data available, read it.
  String line = "";
  while (client.available()) {
    // Read one byte at a time.
    char c = client.read();
    if (readingData) {
      EPD_SendData(c);
      continue;
    }
    if (c != '\n' && c != '\r') {
      line += c;
    } else if (c == '\n') {
      if (line.length() == 0 && isPost && isAuthed) {
        // End of headers.
        readingData = true;
        EPD_Init();
        EPD_SendCommand(0x61);
        EPD_SendData(0x02);
        EPD_SendData(0x58);
        EPD_SendData(0x01);
        EPD_SendData(0xC0);
        EPD_SendCommand(0x10);
      }
      if (line.startsWith("POST")) {
        printf("POST request\n");
        isPost = true;
      }
      if (line.startsWith("GET")) {
        printf("GET request\n");
        isPost = false;
      }
      if (line.indexOf(auth_key) != -1) {
        printf("Auth key found\n");
        isAuthed = true;
      }
      line = "";
    }
  }

  if (!isAuthed) {
    printf("Not authorized, sending 401\n");
    client.print("HTTP/1.1 401 Unauthorized\nContent-Type: "
                 "text/html\nWWW-Authenticate: Basic\n\n");
    client.print("401 Unauthorized");
    client.stop();
    return;
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

  String content = "";

  // Respond with 200 OK and HTML content type.
  content += "HTTP/1.1 200 OK\nContent-Type: text/html\r\n\r\n";

  // Good luck reading this HTML and JS in C++, it's the best I could do!

  // DOCTYPE and head.
  content +=
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
      "</head>"
      "<body style=\"display:flex; justify-content:center; font-family:"
      " 'Inter', sans-serif; background-color: Canvas; color: CanvasText; "
      "color-scheme: light dark;\">"
      "<style>"
      // "a { color: #62a0ea; }";
      // "a:hover { color: #99c1f1; }";
      "</style>"
      "<div style=\"max-width:1000px; min-width: 300px;\">"
      "<h1 style=\"text-align:center;\">wifi-doorsign ";
  // Add our IP to the title, to make it very obvious which device we're
  // interacting with.
  content += WiFi.localIP().toString();
  content +=
      "</h1>"
      "<small>Can't find which display you're talking to? <a "
      "id=\"identify_button\" href=\"#\">"
      "Identify.</a></small>"
      "<hr style=\"margin-left:0px;\"/>"
      // --- Text fill color input (number and range)
      "<form style=\"display: flex; flex-direction: "
      "column; gap: 4px;\">"
      "<div style=\"display: flex; flex-direction: "
      "row; gap: 2px;\">"
      "<label for=\"text_fill_number\" style=\"align-content: "
      "center;width: 150px;\">Text fill color: </label>"
      "<input type=\"number\" id=\"text_fill_number\" "
      "name=\"text_fill_number\" "
      "placeholder=\"170\" value=\"170\" min=\"0\" max=\"255\"/>"
      "<input type=\"range\" id=\"text_fill_range\" name=\"text_fill_range\" "
      "min=\"0\" max=\"255\" step=\"1\" value=\"170\" "
      "style=\"flex-grow:1;\"/>"
      "</div>"
      // --- Text outline color input (number and range)
      "<div style=\"display: flex; flex-direction: "
      "row; gap: 2px;\">"
      "<label for=\"text_outline_number\" style=\"align-content: "
      "center; width: 150px;\">Text outline color: </label>"
      "<input type=\"number\" id=\"text_outline_number\" "
      "name=\"text_outline_number\" "
      "placeholder=\"170\" value=\"0\" min=\"0\" max=\"255\"/>"
      "<input type=\"range\" id=\"text_outline_range\" "
      "name=\"text_outline_range\" "
      "min=\"0\" max=\"255\" step=\"1\" value=\"0\" "
      "style=\"flex-grow:1;\"/>"
      "</div>"
      "</form>"
      "<hr style=\"margin-left:0px;\"/>"
      // --- Text input fields
      "<form style=\"display: flex; flex-direction: "
      "column; gap: 4px;\">"
      "<label for=\"input_text_top\">Class/Course: </label>"
      "<input type=\"text\" id=\"input_text_top\" name=\"input_text_top\" "
      "placeholder=\"Computer Science\"/>"
      "<label for=\"input_title\">Room: </label>"
      "<input type=\"text\" id=\"input_title\" name=\"input_title\" "
      "placeholder=\"S12\"/>"
      "<label for=\"input_text_bottom\">Extra info: </label>"
      "<input type=\"text\" id=\"input_text_bottom\" "
      "name=\"input_text_bottom\" "
      "placeholder=\"13:45-14:35\" />"
      "</form>"
      "<hr style=\"margin-left:0px;\"/>"
      // --- Color selection button
      "<form style=\"display: flex; flex-direction: "
      "row; align-items: center;\">"
      "<label for=\"input_color\">Background color: </label>"
      "<input type=\"color\" id=\"input_color\" name=\"input_color\" "
      "value=\"#ffffff\" style=\"margin-left:10px;\" />"
      "</form>"
      "<hr style=\"margin-left:0px;\"/>"
      // --- File selection button
      "<div style=\"display: flex; flex-direction: "
      "row;\">"
      "<form>"
      "<input style=\"margin-bottom: 6px; min-width: 0px;\"type=\"file\" "
      "id=\"input_file\" name=\"input_file\" "
      "accept=\"image/*\" />"
      "</form>"
      // --- Display resolution reminder
      "<div style=\"text-align: right; flex-grow:1; align-content: "
      "end;\">"
      "<small style=\"padding-left:2px;padding-right:2px;"
      "\">600x448px</small>"
      "</div>"
      "</div>"
      // --- The Mighty Canvas, Storer of Image Data.
      "<div>"
      "<canvas id=\"canvas\" width=\"600\" height=\"448\" "
      "style=\"outline: ButtonBorder 1px solid; width:100%;\"></canvas>"
      "</div>"
      // --- Button to send canvas image to display, and progress bar.
      "<div style=\"display: flex; margin-top:6px;\">"
      "<button style=\"margin-right: 7px\" id=\"push_button\">Push to "
      "screen</button>"
      "<progress style=\"flex-grow: 1\" id=\"progress\" value=\"0\" "
      "max=\"100\"></progress>"
      "</div>"
      "<hr style=\"margin-left:0px;\"/>"
      // --- Footer
      "<small>Built by Edward Hesketh, open source at <a "
      "href=\"https://github.com/headblockhead/"
      "wifi-doorsign\">github:headblockhead/wifi-doorsign</a></small>"
      "</div>"
      // This img tag is hidden, and stores the original selected image as a
      // reference.
      "<img width=\"600\" height=\"448\" id=\"img\" src=\"\" "
      "alt=\"Source image\" style=\"display:none\" />"

      // JS time!
      "<script>"

      // Define the RGB values of the colors used in the display.
      "var palette = "
      "[[0,0,0],[255,255,255],[0,255,0],[0,0,255],[255,0,"
      "0],[255,255,"
      "0],[255,128,0]];"

      // Load the font.
      "var Font24 = new FontFace('Ubuntu Mono', "
      "'url(https://fonts.gstatic.com/s/"
      "ubuntumono/v17/KFO-CneDtsqEr0keqCMhbC-BL9H1tY0.woff2)');"
      "Font24.load().then(function(font) {"
      "document.fonts.add(font);"
      "});"

      // When a file is selected,
      "document.getElementById('input_file')."
      "addEventListener('change'"
      ", function(e) {"
      "var file = e.target.files[0];"
      "var reader = new FileReader();"
      "reader.onload = function(e) {"
      // Set the invisible image's source to the selected file.
      "document.getElementById('img').src = e.target.result;"
      "};"
      "reader.readAsDataURL(file);"
      "img.onload = function() {"
      // When the image is loaded, update the canvas.
      "updateImage(img);"
      "};"
      "});"

      // Assign actions to the buttons.

      // This button sends the image to the display.
      "document.getElementById('push_button')."
      "addEventListener('click', "
      "function() {"
      "pushImage(document.getElementById('canvas'));"
      "});"

      // This button draws the IP of the display on the canvas very
      // large, then sends that image to the display.
      "document.getElementById('identify_button')."
      "addEventListener('click', "
      "function() {"
      "event.preventDefault();"
      "var canvas = document.getElementById('canvas');"
      "var ctx = canvas.getContext('2d');"
      "canvas.width = 600;"
      "canvas.height = 448;"
      "ctx.font = '48px Ubuntu Mono';"
      "ctx.fillStyle = '#ffffff';"
      "ctx.strokeStyle = '#000000';"
      "ctx.fillText('";
  content += WiFi.localIP().toString();
  content += "', 300 - ctx.measureText('";
  content += WiFi.localIP().toString();
  content += "').width/2,448-48);"
             "ctx.font = '256px Ubuntu Mono';"
             "ctx.fillText('IDENT', 300 - "
             "ctx.measureText('IDENT').width/2,"
             "448-192);"
             "var new_canvas = seven_color_dither(ctx);"
             "ctx.drawImage(new_canvas, 0, 0);"
             "document.getElementById('push_button').focus({"
             "focusVisible: true});"
             "document.getElementById('push_button').scrollIntoView();"
             "});"

             // Update the image live as changes are made to the various text
             // fields.

             "document.getElementById('input_title')."
             "addEventListener('input', "
             "function() {"
             "updateImage(document.getElementById('img'));"
             "});"
             "document.getElementById('input_text_top')."
             "addEventListener('input', "
             "function() {"
             "updateImage(document.getElementById('img'));"
             "});"
             "document.getElementById('input_text_bottom')."
             "addEventListener('input', "
             "function() {"
             "updateImage(document.getElementById('img'));"
             "});"

             // Update the image when the color input changes.
             "document.getElementById('input_color')."
             "addEventListener('input', "
             "function() {"
             "updateImage(document.getElementById('img'));"
             "});"

             // Match the number input to the range input, and vice versa.
             // Also, update the image when the value changes.

             "document.getElementById('text_fill_number')."
             "addEventListener('input'"
             ", function() {"
             "document.getElementById('text_fill_range').value = "
             "document.getElementById('text_fill_number').value;"
             "updateImage(document.getElementById('img'));"
             "});"
             "document.getElementById('text_fill_range')."
             "addEventListener('input'"
             ", function() {"
             "document.getElementById('text_fill_number').value = "
             "document.getElementById('text_fill_range').value;"
             "updateImage(document.getElementById('img'));"
             "});"
             "document.getElementById('text_outline_number')."
             "addEventListener('input'"
             ", function() {"
             "document.getElementById('text_outline_range').value = "
             "document.getElementById('text_outline_number').value;"
             "updateImage(document.getElementById('img'));"
             "});"
             "document.getElementById('text_outline_range')."
             "addEventListener('input'"
             ", function() {"
             "document.getElementById('text_outline_number').value = "
             "document.getElementById('text_outline_range').value;"
             "updateImage(document.getElementById('img'));"
             "});"

             // updateImage draws the canvas image from the source image,
             // adding text.
             "function updateImage(img) {"
             "var width = 600;"
             "var height = 448;"
             "var text = document.getElementById('input_title').value;"
             "var top_text = "
             "document.getElementById('input_text_top').value;"
             "var bottom_text = "
             "document.getElementById('input_text_bottom').value;"
             "var textBrightness = "
             "document.getElementById('text_fill_number')."
             "valueAsNumber;"
             "var outlineBrightness = "
             "document.getElementById('text_outline_number')."
             "valueAsNumber;"
             "var canvas = document.getElementById('canvas');"
             "var ctx = canvas.getContext('2d');"
             "canvas.width = width;"
             "canvas.height = height;"

             // Fill the canvas with the background color.
             "ctx.fillStyle = "
             "document.getElementById('input_color').value;"
             "ctx.fillRect(0, 0, width, height);"

             // Draw the source image on the canvas, stretching/squeezing to
             // fit if needed.
             "ctx.drawImage(img, 0, 0,width,height);"

             // Set the font to draw the text with.
             "ctx.font = '288px Ubuntu Mono';"

             // Set the fill and stroke colors for the text.
             "ctx.fillStyle = "
             "\"#\" + textBrightness.toString(16).repeat(3) + \"ff\";"
             "ctx.strokeStyle = "
             "\"#\" + outlineBrightness.toString(16).repeat(3) "
             "+ \"ff\";"

             // When drawing the fill, preserve the hue and chroma of the
             // bottom layer, while adopting the luma of the top layer.
             "ctx.globalCompositeOperation = 'luminosity';"

             // Draw the title text's fill.
             "ctx.fillText(text, 300 - "
             "ctx.measureText(text).width/2"
             ",448-144);"

             // Set the font for the top and bottom text.
             "ctx.font = '50px Ubuntu Mono';"

             // Draw the top and bottom texts' fill.
             "ctx.fillText(top_text, 300 - "
             "ctx.measureText(top_text).width/2"
             ",75);"
             "ctx.fillText(bottom_text, 300 - "
             "ctx.measureText(bottom_text).width/2"
             ",448-50);"

             // When drawing the outline, replace pixels beneath.
             "ctx.globalCompositeOperation = 'source-over';"

             // Draw the title text's outline.
             "ctx.font = '288px Ubuntu Mono';"
             "ctx.lineWidth = 8;"
             "ctx.strokeText(text, 300 - "
             "ctx.measureText(text).width/2"
             ",448-144);"

             // Draw the top and bottom texts' outline.
             "ctx.font = '50px Ubuntu Mono';"
             "ctx.lineWidth = 2;"
             "ctx.strokeText(top_text, 300 - "
             "ctx.measureText(top_text).width/2"
             ",75);"
             "ctx.strokeText(bottom_text, 300 - "
             "ctx.measureText(bottom_text).width/2"
             ",448-50);"

             // Dither the canvas image, then replace the current image with
             // the dithered version.
             "var new_canvas = seven_color_dither(ctx);"
             "ctx.drawImage(new_canvas, 0, 0);"
             "}"

             // pushImage converts the canvas contents into bytes, that are
             // then sent to the display.
             "function pushImage(canvas) {"
             "var ctx = canvas.getContext('2d');"
             // Get image data as RGBA bytes.
             "var data = ctx.getImageData(0, 0, 600, 448).data;"
             // Store the image data in a string, with each byte representing
             // two pixels.
             "var textify = '';"

             "for (var i = 0; i < data.length; i += 8) {"
             "var cha = 0x00;"

             "var r1 = data[i];"
             "var g1 = data[i + 1];"
             "var b1 = data[i + 2];"
             "var ind1 = getNear(r1,g1,b1);"
             "cha |= ind1 << 4;"

             "var r2 = data[i + 4];"
             "var g2 = data[i + 5];"
             "var b2 = data[i + 6];"
             "var ind2 = getNear(r2,g2,b2);"
             "cha |= ind2;"

             "textify += String.fromCharCode(cha);"
             "}"

             // Prepare a POST request to /image.
             "var xhr = new XMLHttpRequest();"
             "xhr.open('POST', '/image', true);"
             "xhr.setRequestHeader('Content-Type', 'application/octet-"
             "stream');"

             // Set the progress bar to 0, and the color to the default.
             "document.getElementById('progress').value = 0;"
             "document.getElementById('progress').style.accentColor = "
             "\"auto\";"

             // When the upload progress changes, update the progress bar.
             "xhr.upload.onprogress = function(e) {"
             "if (e.lengthComputable) {"
             "var percentComplete = (e.loaded / e.total) * 100;"
             "document.getElementById('progress').value = "
             "percentComplete;"
             "}"
             "};"

             // When uploading finishes, set the progress bar's color to
             // green.
             "xhr.onload = function() {"
             "document.getElementById('progress').value = 100;"
             "document.getElementById('progress').style."
             "accentColor = \"green\";"
             "updateImage(document.getElementById('img'));"
             "document.getElementById('push_button').blur();"
             "};"

             // Send the POST request with the image data!
             "xhr.send(textify);"
             "}"

             // A few helper functions provided helpfully by Waveshare.
             "function addVal(c,r,g,b,k){"
             "return[c[0]+(r*k)/32,c[1]+(g*k)/32,c[2]+(b*k)/32];"
             "}"
             "function getNear(r,g,b) {"
             "var ind= 0;"
             "var err= 1000000;"
             "for (var i = 0; i < 7; i++) {"
             "var curErr = "
             "(r-palette[i][0])*(r-palette[i][0])+(g-palette[i][1])*(g-"
             "palette[i][1])+(b-palette[i][2])*(b-palette[i][2]);"
             "if (curErr < err) {"
             "err = curErr;"
             "ind = i;"
             "}"
             "}"
             "return ind;"
             "}"

             // seven_color_dither function, provided by Waveshare, and
             // simplified by me.
             "function seven_color_dither(ctx) {"
             "var width = 600;"
             "var height = 448;"
             "var index = 0;"
             "var pSrc = ctx.getImageData(0, 0, width, height);"
             "var pDst = ctx.createImageData(width, height);"
             "var aInd = 0;"
             "var bInd = 1;"
             "var errArr = new Array(2);"
             "errArr[0] = new Array(width);"
             "errArr[1] = new Array(width);"
             "for (var i = 0; i < width; i++) {"
             "errArr[bInd][i] = [0,0,0];"
             "}"
             "for (var y = 0; y < height; y++) {"
             "aInd = ((bInd=aInd)+1)&1;"
             "for (var x = 0; x < width; x++) {"
             "errArr[bInd][x] = [0,0,0];"
             "}"
             "for (var x = 0; x < width; x++) {"
             "var pos = (y * width + x) * 4;"
             "var old = errArr[aInd][x];"
             "var r = pSrc.data[pos] + old[0];"
             "var g = pSrc.data[pos + 1] + old[1];"
             "var b = pSrc.data[pos + 2] + old[2];"
             "var colVal = palette[getNear(r,g,b)];"
             "pDst.data[index++] = colVal[0];"
             "pDst.data[index++] = colVal[1];"
             "pDst.data[index++] = colVal[2];"
             "pDst.data[index++] = 255;"
             "r = (r - colVal[0]);"
             "g = (g - colVal[1]);"
             "b = (b - colVal[2]);"
             "if (x ==0) {"
             "errArr[bInd][x  ]=addVal(errArr[bInd][x  ],r,g,b,7.0);"
             "errArr[bInd][x+1]=addVal(errArr[bInd][x+1],r,g,b,2.0);"
             "errArr[aInd][x+1]=addVal(errArr[aInd][x+1],r,g,b,7.0);"
             "} else if (x == width - 1) {"
             "errArr[bInd][x-1]=addVal(errArr[bInd][x-1],r,g,b,7.0);"
             "errArr[bInd][x  ]=addVal(errArr[bInd][x  ],r,g,b,9.0);"
             "} else {"
             "errArr[bInd][x-1]=addVal(errArr[bInd][x-1],r,g,b,3.0);"
             "errArr[bInd][x  ]=addVal(errArr[bInd][x  ],r,g,b,5.0);"
             "errArr[bInd][x+1]=addVal(errArr[bInd][x+1],r,g,b,1.0);"
             "errArr[aInd][x+1]=addVal(errArr[aInd][x+1],r,g,b,7.0);"
             "}"
             "}"
             "}"
             "ctx.putImageData(pDst, 0, 0);"
             "return canvas;"
             "}"
             "</script>"

             // Close the HTML tags.
             "</body></html>";

  client.print(content);
  // It's *finally* over, close the connection.
  client.stop();

  // We're done here.
}
