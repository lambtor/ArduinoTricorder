/**************************************************************************
  This is a library for several Adafruit displays based on ST77* drivers.
  The 2.0" TFT breakout
        ----> https://www.adafruit.com/product/4311
 **************************************************************************/

#include <Adafruit_GFX.h>    // Core graphics library
#include <Adafruit_ST7789.h> // Hardware-specific library for ST7789
#include <SPI.h>
#include <string.h>
#include <SD.h>
//#include <Adafruit_SPIFlash.h>
//#include <Adafruit_ImageReader.h>
#include <Adafruit_NeoPixel.h>
#include <Wire.h>
#include <Adafruit_TCS34725.h>

// need to remove hyphens from header filenames or exception will get thrown
//#include <Fonts/lcars24pt-7b.h>
//#include "Fonts/lcars24pt7b.h"
#include "Fonts/lcars11pt7b.h"

//rgb sensor setting - set to false if using a common cathode LED
#define commonAnode true
#define RGB_SENSOR_ACTIVE false

// For the breakout board, you can use any 2 or 3 pins.
// These pins will also work for the 1.8" TFT shield.
#define TFT_CS 9
#define TFT_RST 6
#define TFT_DC 5
#define USE_SD_CARD 1
//pin 8 can pull power level (used for battery %)

// A0 is pin14. can't use that as an output pin?
#define SCAN_LED_PIN_1 15
#define SCAN_LED_PIN_2 16
#define SCAN_LED_PIN_3 17
#define SCAN_LED_PIN_4 18
#define SCAN_LED_BRIGHTNESS 128

// power LED. must use an unreserved pin for this.
// cdn-learn.adafruit.com/assets/assets/000/046/243/large1024/adafruit_products_Feather_M0_Adalogger_v2.2-1.png?1504885273
#define POWER_LED_PIN 0
#define NEOPIXEL_LED_COUNT 1

Adafruit_NeoPixel ledPwrStrip(NEOPIXEL_LED_COUNT, POWER_LED_PIN, NEO_GRB + NEO_KHZ800);
Adafruit_TCS34725 rgbSensor = Adafruit_TCS34725(TCS34725_INTEGRATIONTIME_700MS, TCS34725_GAIN_1X);

// SD card select pin
#define SD_CS 4

#define BUFFPIXEL 20

// card info works. getting conflicts with multiple attempts to reference shit
// set up variables using the SD utility library functions:
// Sd2Card card;
// SdVolume volume;
// SdFile root;

// TNG colors here
/*
#define color_SWOOP       0xF7B3
#define color_MAINTEXT    0xC69F
#define color_LABELTEXT   0x841E
#define color_HEADER      0xFEC8
*/

// ds9
#define color_SWOOP 0xD4F0
#define color_MAINTEXT 0xBD19
#define color_LABELTEXT 0x7A8D
#define color_HEADER 0xECA7

// voy
/*
#define color_SWOOP       0x9E7F
#define color_MAINTEXT    0x7C34
#define color_LABELTEXT   0x9CDF
#define color_HEADER      0xCB33
*/

// OPTION 1 (recommended) is to use the HARDWARE SPI pins, which are unique
// to each board and not reassignable. For Arduino Uno: MOSI = pin 11 and
// SCLK = pin 13. This is the fastest mode of operation and is required if
// using the breakout board's microSD card.

// do not fuck with this. 2.0 IS THE BOARD
// For 1.14", 1.3", 1.54", and 2.0" TFT with ST7789:
Adafruit_ST7789 tft = Adafruit_ST7789(TFT_CS, TFT_DC, TFT_RST);
//Adafruit_Image oImg;

int32_t nImgWidth = 0, // BMP image dimensions
    nImgHeight = 0;

float p = 3.1415926;
int nFlag = 0;
int nIterator = 1;
int nScanCounter = 0;
int nPwrCounter = 0;
const int chipSelect = 4;

void setup(void) {
  ledPwrStrip.begin();
  
  Serial.print(F("Hello! ST77xx TFT Test"));

  // OR use this initializer (uncomment) if using a 2.0" 320x240 TFT:
  tft.init(240, 320); // Init ST7789 320x240

  tft.setRotation(1);
  Serial.println(F("Initialized"));

  tft.setFont(&lcars11pt7b);
  // tft.setRotation(1);

  uint16_t time = millis();
  // tft.fillScreen(ST77XX_BLACK);
  time = millis() - time;

  Serial.println(time, DEC);
  delay(200);

  // large block of text
  tft.fillScreen(ST77XX_BLACK);
  // header is 2 circles, then black rectangle to cut them, then header text
  // then solid rect
  tft.fillRoundRect(300, 4, 16, 16, 8, color_HEADER);
  tft.fillRoundRect(3, 4, 16, 16, 8, color_HEADER);
  tft.fillRect(12, 4, 4, 16, color_HEADER);
  tft.fillRect(305, 4, 4, 16, color_HEADER);

  tft.fillRect(15, 1, 290, 20, ST77XX_BLACK);
  tft.fillRect(19, 4, 180, 16, color_HEADER);
  drawParamText(204, 19, "TRICORDER LCARS", color_MAINTEXT);

  char sWarningText[80];
  strcpy(sWarningText, "");
  // strcat(sWarningText, "\n");
  strcat(sWarningText, "UNITED FEDERATION OF PLANETS");
  // strcat(sWarningText, "\n");
  // strcat(sWarningText, "UNAUTHORIZED USE DISCOURAGED");
  // drawParamText(75, 200, sWarningText, color_MAINTEXT);

  tft.fillRoundRect(3, 221, 16, 16, 8, color_HEADER);
  tft.fillRect(12, 221, 4, 16, color_HEADER);
  tft.fillRoundRect(300, 221, 16, 16, 8, color_HEADER);
  tft.fillRect(305, 221, 4, 16, color_HEADER);

  tft.fillRect(15, 221, 290, 20, ST77XX_BLACK);
  // main header rect last
  tft.fillRect(19, 221, 282, 16, color_HEADER);

  drawWalkingText(75, 200, sWarningText, color_MAINTEXT);
  //try bmp draw
  
  // tft.drawCircle(160, 120, 40, color_MAINTEXT);

  // testdrawtext("HELLO PRIORITY ONE\n ARMADA!" , ST77XX_WHITE);
  // testdrawtext("HELLO PRIORITY ONE\n ARMADA!" , 0xFF22);
  // testfillrects(ST77XX_YELLOW, ST77XX_MAGENTA);
  Serial.begin(9600);

  if (RGB_SENSOR_ACTIVE && rgbSensor.begin()) {
    drawParamText(60, 60, "RGB SENSOR CONNECTED", color_MAINTEXT);
  } else if (RGB_SENSOR_ACTIVE) {
    Serial.println("No TCS34725 found ... check your connections");
    drawParamText(60, 60, "NO RGB SENSOR - CHECK CONNECTION", color_MAINTEXT);
    while (1);
  } else if (!RGB_SENSOR_ACTIVE) {
	drawParamText(60, 60, "RGB SENSOR DISABLED", color_MAINTEXT);
  }

  delay(1000);
}

void loop() {

  ledPwrStrip.clear();
  // max brightness is 255
  // ledStrip.setBrightness(255);
  ledPwrStrip.setBrightness(32);
  // to-do: calculate color based on power level.
  // params for this call: led# (assumed you have multiple in a strip, this
  // should always be 0), R G B 0-255. use 0-128 as high brightness will be PITA
  // to look at blue >= 80%, green >= 60%, yellow >= 40%, orange >= 25%, red >=
  // 0% need a way to test the color progression blue ledStrip.setPixelColor(0, 0, 0, 128); green
  double nPwrReduced = nPwrCounter / 4;
  if (nPwrReduced >= 0 && nPwrReduced < 2) {
    ledPwrStrip.setPixelColor(0, 0, 0, 128);
  } else if (nPwrReduced >= 2 && nPwrReduced < 4) {
    ledPwrStrip.setPixelColor(0, 0, 160, 0);
  } else if (nPwrReduced >= 4 && nPwrReduced < 6) {
    // yellow
    ledPwrStrip.setPixelColor(0, 128, 128, 0);
  } else if (nPwrReduced >= 6 && nPwrReduced < 8) {
    // orange
    ledPwrStrip.setPixelColor(0, 160, 64, 0);
  } else if (nPwrReduced >= 8) {
    // red
    ledPwrStrip.setPixelColor(0, 160, 0, 0);
  }  
  
  // yellow-orange
  // ledStrip.setPixelColor(0, 128, 80, 0);
  ledPwrStrip.show();

  // need to coordinate multiple LED and screen updates against single delay
  // call
  if (nScanCounter == 3) {
    analogWrite(SCAN_LED_PIN_1, SCAN_LED_BRIGHTNESS);
  } else {
    analogWrite(SCAN_LED_PIN_1, 0);
  }
  if (nScanCounter == 2) {
    analogWrite(SCAN_LED_PIN_2, SCAN_LED_BRIGHTNESS);
  } else {
    analogWrite(SCAN_LED_PIN_2, 0);
  }
  if (nScanCounter == 1) {
    analogWrite(SCAN_LED_PIN_3, SCAN_LED_BRIGHTNESS);
  } else {
    analogWrite(SCAN_LED_PIN_3, 0);
  }
  if (nScanCounter == 0) {
    analogWrite(SCAN_LED_PIN_4, SCAN_LED_BRIGHTNESS);
  } else {
    analogWrite(SCAN_LED_PIN_4, 0);
  }
  // analogWrite(SCAN_LED_PIN_2, 255);
  // analogWrite(SCAN_LED_PIN_3, 255);
  // analogWrite(SCAN_LED_PIN_4, 255);

  if (nIterator != 1 && nIterator != 2) {
    tft.invertDisplay(true);
  } else if (nIterator == 1 || nIterator == 2) {
    tft.invertDisplay(false);
  }

  // every iteration = 2, invertdisplay(true)
  // every iteration = 1, invertdisplay(false)
  // iteration cycles from 0 to 20

  if (nIterator > 19) {
    nIterator = 0;
  } else {
    nIterator += 1;
  }

  if (nScanCounter > 4) {
    nScanCounter = 0;
  } else {
    nScanCounter += 1;
  }

  if (nPwrCounter > 40) {
    nPwrCounter = 0;
  } else {
    nPwrCounter += 1;
  }


  //delay(125);  
	if (RGB_SENSOR_ACTIVE && nPwrReduced == 1) {
		rgbSensor.setInterrupt(true);
		tft.fillRect(60, 100, 170, 25, ST77XX_BLACK);
		rgbSensor.setInterrupt(false);
		//everything from here down is color sensor
		uint16_t r, g, b, c, colorTemp, lux;
		float fRed, fGreen, fBlue;
		
		rgbSensor.getRGB(&fRed, &fGreen, &fBlue);
		
		//use strings for all basic text, and only when outputting to screen, convert to char*
		String strTemp = "COLOR ";
		String strValues = "";
		strValues = String((int)fRed) + " " + String((int)fGreen) + " " + String((int)fBlue);
	  
	  strTemp += strValues;
	  char *strTempWr;
		strTempWr = const_cast<char*>(strTemp.c_str());

	  drawParamText(60, 120, strTempWr, color_MAINTEXT);
	} else {
		delay(125);
	}

}

void drawLogo(char *filename, int16_t x, int16_t y) {
	/*
  File     bmpFile;
  int      bmpWidth, bmpHeight;   // W+H in pixels
  uint8_t  bmpDepth;              // Bit depth (currently must be 24)
  uint32_t bmpImageoffset;        // Start of image data in file
  uint32_t rowSize;               // Not always = bmpWidth; may have padding
  uint8_t  sdbuffer[3*BUFFPIXEL]; // pixel buffer (R+G+B per pixel)
  uint8_t  buffidx = sizeof(sdbuffer); // Current position in sdbuffer
  boolean  goodBmp = false;       // Set to true on valid header parse
  boolean  flip    = true;        // BMP is stored bottom-to-top
  int      w, h, row, col, x2, y2, bx1, by1;
  uint8_t  r, g, b;
  uint32_t pos = 0, startTime = millis();

  if((x >= tft.width()) || (y >= tft.height())) return;

  Serial.println();
  Serial.print(F("Loading image '"));
  Serial.print(filename);
  Serial.println('\'');

  // Open requested file on SD card
  if ((bmpFile = SD.open(filename)) == NULL) {
    Serial.print(F("File not found"));
    return;
  }

  // Parse BMP header
  if(read16(bmpFile) == 0x4D42) { // BMP signature
    Serial.print(F("File size: ")); 
	Serial.println(read32(bmpFile));
    (void)read32(bmpFile); // Read & ignore creator bytes
    bmpImageoffset = read32(bmpFile); // Start of image data
    Serial.print(F("Image Offset: ")); 
	Serial.println(bmpImageoffset, DEC);
    // Read DIB header
    Serial.print(F("Header size: ")); 
	Serial.println(read32(bmpFile));
    bmpWidth  = read32(bmpFile);
    bmpHeight = read32(bmpFile);
	
    if(read16(bmpFile) == 1) { // # planes -- must be '1'
      bmpDepth = read16(bmpFile); // bits per pixel
      Serial.print(F("Bit Depth: ")); Serial.println(bmpDepth);
      if((bmpDepth == 24) && (read32(bmpFile) == 0)) { // 0 = uncompressed

        goodBmp = true; // Supported BMP format -- proceed!
        Serial.print(F("Image size: "));
        Serial.print(bmpWidth);
        Serial.print('x');
        Serial.println(bmpHeight);

        // BMP rows are padded (if needed) to 4-byte boundary
        rowSize = (bmpWidth * 3 + 3) & ~3;

        // If bmpHeight is negative, image is in top-down order.
        // This is not canon but has been observed in the wild.
        if(bmpHeight < 0) {
          bmpHeight = -bmpHeight;
          flip      = false;
        }

        // Crop area to be loaded
        x2 = x + bmpWidth  - 1; // Lower-right corner
        y2 = y + bmpHeight - 1;
        if((x2 >= 0) && (y2 >= 0)) { // On screen?
          w = bmpWidth; // Width/height of section to load/display
          h = bmpHeight;
          bx1 = by1 = 0; // UL coordinate in BMP file
          if(x < 0) { // Clip left
            bx1 = -x;
            x   = 0;
            w   = x2 + 1;
          }
          if(y < 0) { // Clip top
            by1 = -y;
            y   = 0;
            h   = y2 + 1;
          }
          if(x2 >= tft.width())  w = tft.width()  - x; // Clip right
          if(y2 >= tft.height()) h = tft.height() - y; // Clip bottom
  
          // Set TFT address window to clipped image bounds
          tft.startWrite(); // Requires start/end transaction now
          tft.setAddrWindow(x, y, w, h);
  
          for (row=0; row<h; row++) { // For each scanline...
  
            // Seek to start of scan line.  It might seem labor-
            // intensive to be doing this on every line, but this
            // method covers a lot of gritty details like cropping
            // and scanline padding.  Also, the seek only takes
            // place if the file position actually needs to change
            // (avoids a lot of cluster math in SD library).
            if(flip) // Bitmap is stored bottom-to-top order (normal BMP)
              pos = bmpImageoffset + (bmpHeight - 1 - (row + by1)) * rowSize;
            else     // Bitmap is stored top-to-bottom
              pos = bmpImageoffset + (row + by1) * rowSize;
            pos += bx1 * 3; // Factor in starting column (bx1)
            if(bmpFile.position() != pos) { // Need seek?
              tft.endWrite(); // End TFT transaction
              bmpFile.seek(pos);
              buffidx = sizeof(sdbuffer); // Force buffer reload
              tft.startWrite(); // Start new TFT transaction
            }
            for (col=0; col<w; col++) { // For each pixel...
              // Time to read more pixel data?
              if (buffidx >= sizeof(sdbuffer)) { // Indeed
                tft.endWrite(); // End TFT transaction
                bmpFile.read(sdbuffer, sizeof(sdbuffer));
                buffidx = 0; // Set index to beginning
                tft.startWrite(); // Start new TFT transaction
              }
              // Convert pixel from BMP to TFT format, push to display
              b = sdbuffer[buffidx++];
              g = sdbuffer[buffidx++];
              r = sdbuffer[buffidx++];
              tft.writePixel(tft.color565(r,g,b));
            } // end pixel
          } // end scanline
          tft.endWrite(); // End last TFT transaction
        } // end onscreen
        Serial.print(F("Loaded in "));
        Serial.print(millis() - startTime);
        Serial.println(" ms");
      } // end goodBmp
    }
  }

  bmpFile.close();
  if(!goodBmp) Serial.println(F("BMP format not recognized."));*/
}

void drawParamText(uint8_t nPosX, uint8_t nPosY, char *sText, uint16_t nColor) {
  tft.setCursor(nPosX, nPosY);
  tft.setTextColor(nColor);
  tft.setTextWrap(true);
  tft.print(sText);
}

void drawWalkingText(int nPosX, int nPosY, char *sText, uint16_t nColor) {
  if (strlen(sText) == 0) {
    return;
  }
  tft.setCursor(nPosX, nPosY);
  tft.setTextColor(nColor);
  tft.setTextWrap(true);
  int i;
  for (i = 0; i < strlen(sText); i++) {
    tft.print(sText[i]);
    delay(50);
  }

  // free(i);
}

void testlines(uint16_t color) {
  tft.fillScreen(ST77XX_BLACK);
  for (int16_t x = 0; x < tft.width(); x += 6) {
    tft.drawLine(0, 0, x, tft.height() - 1, color);
    delay(0);
  }
  for (int16_t y = 0; y < tft.height(); y += 6) {
    tft.drawLine(0, 0, tft.width() - 1, y, color);
    delay(0);
  }

  tft.fillScreen(ST77XX_BLACK);
  for (int16_t x = 0; x < tft.width(); x += 6) {
    tft.drawLine(tft.width() - 1, 0, x, tft.height() - 1, color);
    delay(0);
  }
  for (int16_t y = 0; y < tft.height(); y += 6) {
    tft.drawLine(tft.width() - 1, 0, 0, y, color);
    delay(0);
  }

  tft.fillScreen(ST77XX_BLACK);
  for (int16_t x = 0; x < tft.width(); x += 6) {
    tft.drawLine(0, tft.height() - 1, x, 0, color);
    delay(0);
  }
  for (int16_t y = 0; y < tft.height(); y += 6) {
    tft.drawLine(0, tft.height() - 1, tft.width() - 1, y, color);
    delay(0);
  }

  tft.fillScreen(ST77XX_BLACK);
  for (int16_t x = 0; x < tft.width(); x += 6) {
    tft.drawLine(tft.width() - 1, tft.height() - 1, x, 0, color);
    delay(0);
  }
  for (int16_t y = 0; y < tft.height(); y += 6) {
    tft.drawLine(tft.width() - 1, tft.height() - 1, 0, y, color);
    delay(0);
  }
}

void testdrawtext(char *text, uint16_t color) {
  tft.setCursor(0, 100);
  tft.setTextColor(color);
  tft.setTextWrap(true);
  tft.print(text);
}

void testfastlines(uint16_t color1, uint16_t color2) {
  tft.fillScreen(ST77XX_BLACK);
  for (int16_t y = 0; y < tft.height(); y += 5) {
    tft.drawFastHLine(0, y, tft.width(), color1);
  }
  for (int16_t x = 0; x < tft.width(); x += 5) {
    tft.drawFastVLine(x, 0, tft.height(), color2);
  }
}

void testdrawrects(uint16_t color) {
  tft.fillScreen(ST77XX_BLACK);
  for (int16_t x = 0; x < tft.width(); x += 6) {
    tft.drawRect(tft.width() / 2 - x / 2, tft.height() / 2 - x / 2, x, x,
                 color);
  }
}

void testfillrects(uint16_t color1, uint16_t color2) {
  tft.fillScreen(ST77XX_BLACK);
  for (int16_t x = tft.width() - 1; x > 6; x -= 6) {
    tft.fillRect(tft.width() / 2 - x / 2, tft.height() / 2 - x / 2, x, x,
                 color1);
    tft.drawRect(tft.width() / 2 - x / 2, tft.height() / 2 - x / 2, x, x,
                 color2);
  }
}

void testfillcircles(uint8_t radius, uint16_t color) {
  for (int16_t x = radius; x < tft.width(); x += radius * 2) {
    for (int16_t y = radius; y < tft.height(); y += radius * 2) {
      tft.fillCircle(x, y, radius, color);
    }
  }
}

void testdrawcircles(uint8_t radius, uint16_t color) {
  for (int16_t x = 0; x < tft.width() + radius; x += radius * 2) {
    for (int16_t y = 0; y < tft.height() + radius; y += radius * 2) {
      tft.drawCircle(x, y, radius, color);
    }
  }
}

void testtriangles() {
  tft.fillScreen(ST77XX_BLACK);
  uint16_t color = 0xF800;
  int t;
  int w = tft.width() / 2;
  int x = tft.height() - 1;
  int y = 0;
  int z = tft.width();
  for (t = 0; t <= 15; t++) {
    tft.drawTriangle(w, y, y, x, z, x, color);
    x -= 4;
    y += 4;
    z -= 4;
    color += 100;
  }
}

void testroundrects() {
  tft.fillScreen(ST77XX_BLACK);
  uint16_t color = 100;
  int i;
  int t;
  for (t = 0; t <= 4; t += 1) {
    int x = 0;
    int y = 0;
    int w = tft.width() - 2;
    int h = tft.height() - 2;
    for (i = 0; i <= 16; i += 1) {
      tft.drawRoundRect(x, y, w, h, 5, color);
      x += 2;
      y += 3;
      w -= 4;
      h -= 6;
      color += 1100;
    }
    color += 100;
  }
}



char* string2char(String command){
    if(command.length()!=0){
        char *p = const_cast<char*>(command.c_str());
        return p;
    }
}


uint16_t read16(File &f) {
  uint16_t result;
  ((uint8_t *)&result)[0] = f.read(); // LSB
  ((uint8_t *)&result)[1] = f.read(); // MSB
  return result;
}

uint32_t read32(File &f) {
  uint32_t result;
  ((uint8_t *)&result)[0] = f.read(); // LSB
  ((uint8_t *)&result)[1] = f.read();
  ((uint8_t *)&result)[2] = f.read();
  ((uint8_t *)&result)[3] = f.read(); // MSB
  return result;
}