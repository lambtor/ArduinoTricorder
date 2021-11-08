#include <Adafruit_GFX.h>    // Core graphics library
#include <Adafruit_ST7789.h> // Hardware-specific library for ST7789
#include <SPI.h>
#include <string.h>
#include <SD.h>
#include <Adafruit_NeoPixel.h>
#include <Wire.h>
//this is rgb sensor
#include <Adafruit_TCS34725.h>
#include <EasyButton.h>

// need to remove hyphens from header filenames or exception will get thrown
#include "Fonts/lcars15pt7b.h"
#include "Fonts/lcars11pt7b.h"

//rgb sensor setting - set to false if using a common cathode LED
#define RGB_SENSOR_ACTIVE true

// For the breakout board, you can use any 2 or 3 pins.
// These pins will also work for the 1.8" TFT shield.
//need to use pin 10 for TFT_CS, as pin 9 is analog 7. analog 7 is the only way to get current voltage, which is used for battery %
#define TFT_CS 10
// SD card select pin
//#define SD_CS 4
#define TFT_RST -1
#define TFT_DC 5
#define USE_SD_CARD 1
//pin 9 can pull power level (used for battery %)
#define VOLT_PIN 9
//buttons, scroller
#define BUTTON_1_PIN		19
#define BUTTON_2_PIN		12
#define BUTTON_3_PIN		11
#define SCROLLER_PIN_UP	14
#define SCROLLER_PIN_DOWN	1

// A0 is pin14. can't use that as an output pin?
#define SCAN_LED_PIN_1 15
#define SCAN_LED_PIN_2 16
#define SCAN_LED_PIN_3 17
#define SCAN_LED_PIN_4 18
#define SCAN_LED_BRIGHTNESS 128

// power LED. must use an unreserved pin for this.
// cdn-learn.adafruit.com/assets/assets/000/046/243/large1024/adafruit_products_Feather_M0_Adalogger_v2.2-1.png?1504885273
#define POWER_LED_PIN 6
#define NEOPIXEL_LED_COUNT 1

Adafruit_NeoPixel ledPwrStrip(NEOPIXEL_LED_COUNT, POWER_LED_PIN, NEO_GRB + NEO_KHZ800);
Adafruit_TCS34725 rgbSensor = Adafruit_TCS34725(TCS34725_INTEGRATIONTIME_50MS, TCS34725_GAIN_16X);
// do not fuck with this. 2.0 IS THE BOARD
Adafruit_ST7789 tft = Adafruit_ST7789(TFT_CS, TFT_DC, TFT_RST);

//#define BUFFPIXEL 20
// TNG colors here
#define color_SWOOP		0xF7B3
#define color_MAINTEXT		0xC69F
#define color_LABELTEXT	0x841E
#define color_HEADER		0xFEC8
#define color_TITLETEXT	0xFEC8
// ds9
//#define color_SWOOP       0xD4F0
//#define color_MAINTEXT    0xBD19
//#define color_LABELTEXT   0x7A8D
//#define color_HEADER      0xECA7
//#define color_TITLETEXT	0xC3ED
// voy
//#define color_SWOOP       0x9E7F
//#define color_MAINTEXT    0x7C34
//#define color_LABELTEXT   0x9CDF
//#define color_HEADER      0xCB33
//#define color_TITLETEXT	0xFFAF

int nIterator = 1;
int nScanCounter = 0;
int nPwrCounter = 0;
bool mbRGBSensorStarted = false;
bool mbRGBScreenActive = false;
int mnLeftLEDInterval = 175;
int mnPowerLEDInterval = 5000;
int mnLeftLEDCurrent = 0;
unsigned long mnLastUpdateLeftLED = 0;
unsigned long mnLastUpdatePower = 0;
unsigned long mnLastRGBScan = 0;
int mnRGBScanInterval = 5000;
int mnRGBCooldown = 0;
bool mbRGBActive = false;
//power color enumerator: blue = 4, green = 3, yellow = 2, orange = 1, red = 0
int mnPowerColor = 4;
//unsigned long mnLastUpdateButton1 = 0;
//unsigned long mnLastUpdateButton2 = 0;
//unsigned long mnLastUpdateButton3 = 0;
int mnButtonMinThreshold = 50;
//int mnLastButton1State = HIGH;
//int mnLastButton2State = HIGH;
//int mnCurrentButton2State = HIGH;
//int mnLastButton3State = HIGH;
boolean mbButton2Flag = false;
EasyButton oButton2(BUTTON_2_PIN);

#define RGBto565(r,g,b) ((((r) & 0xF8) << 8) | (((g) & 0xFC) << 3) | ((b) >> 3))

void setup() {
	ledPwrStrip.begin();

	// OR use this initializer (uncomment) if using a 2.0" 320x240 TFT:
	tft.init(240, 320, SPI_MODE0); // Init ST7789 320x240
	tft.setRotation(1);
	tft.setFont(&lcars11pt7b);
	//these googles, they do nothing!
	tft.setTextWrap(false);

	Serial.begin(9600);
	oButton2.begin();
	oButton2.onPressed(ToggleRGBSensor);
	//setup scroller pins, buttons
	//pinMode(SCROLLER_PIN_UP, INPUT);
	//pinMode(SCROLLER_PIN_DOWN, INPUT);
	
	//manage sensor initialization here, NOT in home screen function
	if (RGB_SENSOR_ACTIVE) {
		mbRGBSensorStarted = rgbSensor.begin();
		if (mbRGBSensorStarted) {
			rgbSensor.setInterrupt(true);
		}
	}
	//if camera active in config, begin camera?  
	//if gps active in config, begin gps
	
	ledPwrStrip.clear();
	// max brightness is 255
	// ledStrip.setBrightness(255);
	ledPwrStrip.setBrightness(32);
	
	GoHome();
	delay(500);
}

//needs rewrite to use milliseconds instead of delay()
//poll battery once per minute for power LED, numeric disp
void loop() {
	//this toggles RGB scanner
	oButton2.read();
		
	//handle left side scanner led 	
	LeftScanner();	
	PowerColor();
	
	RunRGBSensor();
	
	/*
	//prevent screen burn-in, or just make sure display shows that loop is running?
	if (nIterator != 1 && nIterator != 2) {
		tft.invertDisplay(true);
	} else if (nIterator == 1 || nIterator == 2) {
		tft.invertDisplay(false);
	} */

}

void drawParamText(uint8_t nPosX, uint8_t nPosY, char *sText, uint16_t nColor) {
	tft.setCursor(nPosX, nPosY);
	tft.setTextColor(nColor);
	tft.setTextWrap(true);
	tft.print(sText);
}

void drawWalkingText(int nPosX, int nPosY, char *sText, uint16_t nColor) {
	//this needs a refactor eventually, as it will block due to delay()
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
}

void RGBtoHSL(float r, float g, float b, double hsl[]) { 
	//this assumes byte - need to divide by 255 to get byte?
    double rd = (double) r/255;
    double gd = (double) g/255;
    double bd = (double) b/255;
	//double rd = (double)r;
	//double gd = (double)g;
	//double bd = (double)b;
    double max = threeway_max(rd, gd, bd);
    double min = threeway_min(rd, gd, bd);
    double h, s;
	double l = (max + min) / 2;

    if (max == min) {
        h = s = 0; // achromatic
    } else {
        double d = max - min;
        s = l > 0.5 ? d / (2 - max - min) : d / (max + min);
        if (max == rd) {
            h = (gd - bd) / d + (gd < bd ? 6 : 0);
        } else if (max == gd) {
            h = (bd - rd) / d + 2;
        } else if (max == bd) {
            h = (rd - gd) / d + 4;
        }
        h /= 6;
    }
	//these values are factored 0-1. actual ranges are 0-360, 0-100, 0-100
    hsl[0] = h * 360;
    hsl[1] = s * 100;
    hsl[2] = l * 100;
}

void HSLtoRGB(double h, double s, double l, double rgb[]) {
    double r, g, b;	

    if (s == 0) {
        r = g = b = l; // achromatic
    } else {
        double q = l < 0.5 ? l * (1 + s) : l + s - l * s;
        double p = 2 * l - q;
        r = HUEtoRGB(p, q, h + 1/3.0);
        g = HUEtoRGB(p, q, h);
        b = HUEtoRGB(p, q, h - 1/3.0);
    }

    //rgb[0] = r * 255;
    //rgb[1] = g * 255;
    //rgb[2] = b * 255;
	rgb[0] = r;
	rgb[1] = g;
	rgb[2] = b;
}

void RGBtoCMYK(double r, double g, double b, double cmyk[]) {
	//fRed = arrdRGB[0] / 255;
	//fGreen = arrdRGB[1] / 255;
	//fBlue = arrdRGB[2] / 255;
	cmyk[3] = threeway_min((1-r), (1-g), (1-b));
	double dBlk = (1 - cmyk[3]);
	cmyk[0] = (1 - r - cmyk[3]) / dBlk;
	cmyk[1] = (1 - g - cmyk[3]) / dBlk;
	cmyk[2] = (1 - b - cmyk[3]) / dBlk;
	//Cyan    = (1-fRed-Black)/(1-Black)
	//Magenta = (1-fGreen-Black)/(1-Black)
	//Yellow  = (1-fBlue-Black)/(1-Black) 
}

double threeway_max(double a, double b, double c) {
    return max(a, max(b, c));
}

double threeway_min(double a, double b, double c) {
    return min(a, min(b, c));
}

double HUEtoRGB(double p, double q, double t) {
    if(t < 0) t += 1;
    if(t > 1) t -= 1;
    if(t < 1/6.0) return p + (q - p) * 6 * t;
    if(t < 1/2.0) return q;
    if(t < 2/3.0) return p + (q - p) * (2/3.0 - t) * 6;
    return p;
}

String PrintHex16(uint16_t *data) {
   char tmp[16];
   for (int i=0; i < 5; i++) { 
	 sprintf(tmp, "0x%.4X",data[i]); 
	 Serial.print(tmp); Serial.print(" ");
   }
   return (String)tmp;
}

void RGBtoHSV(float r, float g, float b, double hsv[]) {
	double rd = r;
	double gd = g;
	double bd = b;
    //double rd = (double) r/255;
    //double gd = (double) g/255;
    //double bd = (double) b/255;
    double max = threeway_max(rd, gd, bd), min = threeway_min(rd, gd, bd);
    double h, s, v = max;

    double d = max - min;
    s = max == 0 ? 0 : d / max;

    if (max == min) { 
        h = 0; // achromatic
    } else {
        if (max == rd) {
            h = (gd - bd) / d + (gd < bd ? 6 : 0);
        } else if (max == gd) {
            h = (bd - rd) / d + 2;
        } else if (max == bd) {
            h = (rd - gd) / d + 4;
        }
        h /= 6;
    }

    hsv[0] = h;
    hsv[1] = s;
    hsv[2] = v;
}

void HSVtoRGB(double h, double s, double v, double rgb[]) {
    double r, g, b;

    int i = int(h * 6);
    double f = h * 6 - i;
    double p = v * (1 - s);
    double q = v * (1 - f * s);
    double t = v * (1 - (1 - f) * s);

    switch(i % 6){
        case 0: r = v, g = t, b = p; break;
        case 1: r = q, g = v, b = p; break;
        case 2: r = p, g = v, b = t; break;
        case 3: r = p, g = q, b = v; break;
        case 4: r = t, g = p, b = v; break;
        case 5: r = v, g = p, b = q; break;
    }

    //rgb[0] = r * 255;
    //rgb[1] = g * 255;
    //rgb[2] = b * 255;
	rgb[0] = r;
	rgb[1] = g;
	rgb[2] = b;
}

String TruncateDouble(double dInput) {
	//badlypads to 3 digits. instead have a function to return pad pixel amount, use that in text print call - more precise this way.
	int nInput = (int)dInput;
	String sResult = (String)nInput;
	return sResult;
} 

int GetBuffer(double dInput) {
	if (dInput < 10) {
		return 14;
	} else if (dInput < 100) {
		return 8;
	} 
	return 0;
}

void GoHome() {
	tft.setFont(&lcars11pt7b);

	// large block of text
	tft.fillScreen(ST77XX_BLACK);
	// home screen header is 2 rounded rectangles, lines to cut them, 1 black rect as backing for header text
	tft.fillRoundRect(3, 4, 316, 16, 8, color_SWOOP);
	tft.fillRoundRect(3, 221, 316, 16, 8, color_SWOOP);
	//left side slots
	tft.drawFastVLine(15, 4, 312, ST77XX_BLACK);
	tft.drawFastVLine(16, 4, 312, ST77XX_BLACK);
	tft.drawFastVLine(17, 4, 312, ST77XX_BLACK);
	tft.drawFastVLine(18, 4, 312, ST77XX_BLACK);
	//bottom right slots
	tft.drawFastVLine(305, 221, 16, ST77XX_BLACK);
	tft.drawFastVLine(304, 221, 16, ST77XX_BLACK);
	tft.drawFastVLine(303, 221, 16, ST77XX_BLACK);
	tft.drawFastVLine(302, 221, 16, ST77XX_BLACK);

	tft.fillRect(255, 4, 50, 16, ST77XX_BLACK);
	drawParamText(252, 19, "  STATUS", color_MAINTEXT);

	String sWarningText = "UNITED FEDERATION OF PLANETS";	

	//show sensor statuses
	if (RGB_SENSOR_ACTIVE && mbRGBSensorStarted) {
		//sWarningText = "CHROMATIC SENSOR";
		//tft.getTextBounds(&sWarningText, 20, 54, &nTextPosX, &nTextPosY, &nTextWidth, &nTextHeight);
		drawParamText(20, 54, "CHROMATICS", color_LABELTEXT);
		//drawWalkingText(95, 54, "ONLINE", color_MAINTEXT);		
		drawParamText(95, 54, "ONLINE", color_MAINTEXT);		
	} else if (RGB_SENSOR_ACTIVE) {
		//Serial.println("No TCS34725 found ... check your connections");
		drawParamText(20, 54, "CHROMATICS", color_LABELTEXT);
		drawParamText(95, 54, "OFFLINE", color_MAINTEXT);
	} else if (!RGB_SENSOR_ACTIVE && mbRGBSensorStarted) {
		drawParamText(20, 54, "CHROMATICS", color_LABELTEXT);
		drawParamText(95, 54, "DISABLED", color_MAINTEXT);
		rgbSensor.setInterrupt(true);
		//disable is possible here, but call will force LED on
	}

	ShowBatteryLevel(222, 54, color_LABELTEXT, color_MAINTEXT);
	
	//bottom crawling text	
	//drawWalkingText(75, 200, const_cast<char*>(sWarningText.c_str()), color_MAINTEXT);
	drawParamText(75, 200, const_cast<char*>(sWarningText.c_str()), color_MAINTEXT);
	//erase crawling text
	//tft.fillRect(75, 180, 180, 25, ST77XX_BLACK);
}

void ShowBatteryLevel(int nPosX, int nPosY, uint16_t nLabelColor, uint16_t nValueColor) {
	//to-do: use global setting to enable/disable battery voltage check
	String sBatteryStatus = "";  
	float fBattV = analogRead(VOLT_PIN);
	//fBattV *= 2;    // we divided by 2 (board has a resistor on this pin), so multiply back,  // Multiply by 3.3V, our reference voltage
	//fBattV *= 3.3; 
	//combine previous actions for brevity- multiply by 2 to adjust for resistor, then 3.3 reference voltage
	fBattV *= 6.6;
	fBattV /= 1024; // convert to voltage
	//3.3 to 4.2 => subtract 3.3 (0) from current voltage. now values should be in range of 0-0.9 : multiply by 1.111, then by 100, convert result to int.
	fBattV -= 3.3;
	fBattV *= 111.11;
	//need to use space characters to pad string because it'll wrap back around to left edge when cursor set over 240 (height).
	//this is a glitch of adafruit GFX library - it thinks display is 240x320 despite the rotation in setup() and setting for text wrap
	sBatteryStatus = "      " + String((int)fBattV);
	//carrBatteryStatus = const_cast<char*>(sBatteryStatus.c_str());
	//tft.getTextBounds("POWER", 222, 54, &nTextPosX, &nTextPosY, &nTextWidth, &nTextHeight);
	drawParamText(nPosX + 20, nPosY, const_cast<char*>(sBatteryStatus.c_str()), color_MAINTEXT);
	
	drawParamText(nPosX, nPosY, "POWER", color_LABELTEXT);
}

void LeftScanner() {
	unsigned long lTimer = millis();
	//to-do: add parameters to change cycle behavior of LEDs.
	//ex: cycled down-> up, stacking, unified blink, KITT ping pong, etc
	
	if ((lTimer - mnLastUpdateLeftLED) > mnLeftLEDInterval) {		
		switch (mnLeftLEDCurrent) {
			case 1: analogWrite(SCAN_LED_PIN_1, 0); analogWrite(SCAN_LED_PIN_4, SCAN_LED_BRIGHTNESS); mnLeftLEDCurrent = 4; break;
			case 2: analogWrite(SCAN_LED_PIN_2, 0); analogWrite(SCAN_LED_PIN_1, SCAN_LED_BRIGHTNESS); mnLeftLEDCurrent = 1; break;
			case 3: analogWrite(SCAN_LED_PIN_3, 0); analogWrite(SCAN_LED_PIN_2, SCAN_LED_BRIGHTNESS); mnLeftLEDCurrent = 2; break;
			default: analogWrite(SCAN_LED_PIN_4, 0); analogWrite(SCAN_LED_PIN_3, SCAN_LED_BRIGHTNESS); mnLeftLEDCurrent = 3; break;
		}
		mnLastUpdateLeftLED = lTimer;
	}
}

void PowerColor() {
	unsigned long lTimer = millis();
	
	if ((lTimer - mnLastUpdatePower) > mnPowerLEDInterval) {
		//switch should eventually be changed to poll voltage pin
		//cycle order = blue, green, yellow, orange, red
		switch (mnPowerColor) {
			case 4: ledPwrStrip.setPixelColor(0, 0, 0, 128); mnPowerColor = 3; break;
			case 3: ledPwrStrip.setPixelColor(0, 0, 160, 0); mnPowerColor = 2; break;
			case 2: ledPwrStrip.setPixelColor(0, 128, 128, 0); mnPowerColor = 1; break;
			case 1: ledPwrStrip.setPixelColor(0, 160, 64, 0); mnPowerColor = 0; break;
			default: ledPwrStrip.setPixelColor(0, 160, 0, 0); mnPowerColor = 4; break;
		}
		mnLastUpdatePower = lTimer;
		ledPwrStrip.show();
	}	
}

void ToggleRGBSensor() {
	mbButton2Flag = !mbButton2Flag;
	
	if (mbButton2Flag) {
		//to-do: if sensor disabled or not started, pulse message to display
		if (!mbRGBActive) {
			mbRGBActive = true;
			//load rgb scanner screen - this is done once to improve perf
			tft.fillScreen(ST77XX_BLACK);
			tft.fillRoundRect(0, -25, 100, 140, 25, color_SWOOP);
			tft.fillRoundRect(0, 120, 100, 140, 25, color_SWOOP);
			tft.drawFastHLine(25, 112, 295, color_SWOOP);
			tft.drawFastHLine(25, 113, 295, color_SWOOP);
			tft.drawFastHLine(25, 114, 295, color_SWOOP);
			tft.drawFastHLine(25, 115, 295, color_SWOOP);		
			
			tft.drawFastHLine(25, 120, 295, color_SWOOP);
			tft.drawFastHLine(25, 121, 295, color_SWOOP);
			tft.drawFastHLine(25, 122, 295, color_SWOOP);
			tft.drawFastHLine(25, 123, 295, color_SWOOP);			
						
			tft.fillRoundRect(50, -3, 60, 115, 5, ST77XX_BLACK);
			tft.fillRoundRect(50, 124, 60, 125, 5, ST77XX_BLACK);
			tft.drawFastVLine(121, 110, 16, ST77XX_BLACK);
			tft.drawFastVLine(122, 110, 16, ST77XX_BLACK);
			tft.drawFastVLine(241, 110, 16, ST77XX_BLACK);
			tft.drawFastVLine(242, 110, 16, ST77XX_BLACK);
			
			//previously was 49,50 for top edge of timer
			tft.drawFastHLine(0, 28, 50, ST77XX_BLACK);
			tft.drawFastHLine(0, 29, 50, ST77XX_BLACK);
			tft.drawFastHLine(0, 30, 50, ST77XX_BLACK);
			tft.fillRect(0, 31, 50, 46, color_HEADER);
			tft.drawFastHLine(0, 77, 50, ST77XX_BLACK);
			tft.drawFastHLine(0, 78, 50, ST77XX_BLACK);
			tft.drawFastHLine(0, 79, 50, ST77XX_BLACK);
			//tft.fillRect(241, 110, 2, 16, ST77XX_BLACK);
			
			tft.fillRect(123, 114, 30, 8, ST77XX_BLACK);
			tft.setFont(&lcars15pt7b);
			drawParamText(184, 21, "CHROMATIC SCAN", color_TITLETEXT);
			//data labels
			tft.setFont(&lcars11pt7b);
			drawParamText(112, 48, "RED", color_LABELTEXT);
			drawParamText(112, 74, "GREEN", color_LABELTEXT);
			drawParamText(112, 99, "BLUE", color_LABELTEXT);
			
			drawParamText(112, 150, "CYAN", color_LABELTEXT);
			drawParamText(112, 176, "MAGENTA", color_LABELTEXT);
			drawParamText(112, 202, "YELLOW", color_LABELTEXT);
			drawParamText(112, 228, "KEY", color_LABELTEXT);
			
			drawParamText(237, 48, "HUE", color_LABELTEXT);
			drawParamText(237, 74, "SATURATION", color_LABELTEXT);
			drawParamText(237, 99, "LUMINOSITY", color_LABELTEXT);
			
			drawParamText(237, 150, "RGB565", color_LABELTEXT);
			drawParamText(237, 176, "HEX", color_LABELTEXT);
		} 
		//Serial.println("button 2 yes flag");
		//tft.fillRoundRect(140, 110, 100, 50, 10, ST77XX_YELLOW);
		//
	} else {
		mbRGBActive = false;
		
		GoHome();
		mnRGBCooldown = 0;
		//Serial.println("button 2 no flag");
		//tft.fillRoundRect(140, 110, 100, 50, 10, ST77XX_GREEN);
	}
}

void RunRGBSensor() {
	if (mbRGBActive) {		
		//erase values if interval has passed
		if (millis() - mnLastRGBScan > mnRGBScanInterval) {
			tft.fillRect(175, 135, 59, 44, ST77XX_BLACK);
			tft.fillRect(198, 31, 28, 70, ST77XX_BLACK);
			tft.fillRect(74, 31, 30, 70, ST77XX_BLACK);
			tft.fillRect(74, 135, 27, 95, ST77XX_BLACK);
			//get values from sensor
			rgbSensor.setInterrupt(false);					
			
			//delay here is set to 75 because sensor is 50ms. blocking is unavoidable here
			delay(50);
			
			float fRed, fGreen, fBlue;
			//need way to calculate HSL from HSL values, then set LUM to 75% and calculate RGB for THAT. 
			//current readings are just a bit off.
			rgbSensor.getRGB(&fRed, &fGreen, &fBlue);
			
			//need a sanity test to prevent weird readings from destroying display			
			if (fRed < 0 || fRed > 255 || fGreen < 0 || fGreen > 255 || fBlue < 0 || fBlue > 255) {
				//to-do: create full error screen in redalert color scheme
				tft.fillRoundRect(186, 189, 113, 42, 10, ST77XX_WHITE);
				drawParamText(223, 228, "SCAN ERROR", ST77XX_BLACK);
				rgbSensor.setInterrupt(true);
				//write zeroes to all values?
				mnLastRGBScan = millis();
				return;
			}
			
			///convert rgb to HSL
			double arrdHSL[2];
			double arrdHSV[2];
			double arrdRGB[2];
			//RGBtoHSL((double)fRed, (double)fGreen, (double)fBlue, arrdHSL);
			RGBtoHSV((double)fRed, (double)fGreen, (double)fBlue, arrdHSV);
			//multiply sat by 1.2, convert it back to RGB for a more accurate sensor reading? colors are 20% dulled
			arrdHSV[1] *= 1.3;
			HSVtoRGB(arrdHSV[0], arrdHSV[1], arrdHSV[2], arrdRGB);
			//displayRGB
			drawParamText(78 + GetBuffer(arrdRGB[0]), 48, const_cast<char*>(TruncateDouble(arrdRGB[0]).c_str()), color_MAINTEXT);
			drawParamText(78 + GetBuffer(arrdRGB[1]), 74, const_cast<char*>(TruncateDouble(arrdRGB[1]).c_str()), color_MAINTEXT);
			drawParamText(78 + GetBuffer(arrdRGB[2]), 99, const_cast<char*>(TruncateDouble(arrdRGB[2]).c_str()), color_MAINTEXT);
			
			//uncomment this when need to display HSL conversion
			RGBtoHSL(arrdRGB[0], arrdRGB[1], arrdRGB[2], arrdHSL);
			//display HSL values
			drawParamText(203 + GetBuffer(arrdHSL[0]), 48, const_cast<char*>(TruncateDouble(arrdHSL[0]).c_str()), color_MAINTEXT);
			drawParamText(203 + GetBuffer(arrdHSL[1]), 74, const_cast<char*>(TruncateDouble(arrdHSL[1]).c_str()), color_MAINTEXT);
			drawParamText(203 + GetBuffer(arrdHSL[2]), 99, const_cast<char*>(TruncateDouble(arrdHSL[2]).c_str()), color_MAINTEXT);
			
			//free(arrdHSL);
			//free(arrdHSV);
						
			//output HSL, RGB to display
			uint16_t nRGB565 = RGBto565((int)arrdRGB[0], (int)arrdRGB[1], (int)arrdRGB[2]);
			//use strings for all basic text, and only when outputting to screen, convert to char*
			String strTemp = "";			
			if ((String((int)arrdRGB[0], HEX)).length() == 1) {
				strTemp += "0" + String((int)arrdRGB[0], HEX);
			} else {
				strTemp += String((int)arrdRGB[0], HEX);
			}
			if ((String((int)arrdRGB[1], HEX)).length() == 1) {
				strTemp += "0" + String((int)arrdRGB[1], HEX);
			} else {
				strTemp += String((int)arrdRGB[1], HEX);
			}
			if ((String((int)arrdRGB[2], HEX)).length() == 1) {
				strTemp += "0" + String((int)arrdRGB[2], HEX);
			} else {
				strTemp += String((int)arrdRGB[2], HEX);
			}
			//print HTML, RGB565			
			strTemp.toUpperCase();
			drawParamText(185, 176, const_cast<char*>(strTemp.c_str()), color_MAINTEXT);
			strTemp = PrintHex16(&nRGB565);
			drawParamText(185, 150, const_cast<char*>(strTemp.c_str()), color_MAINTEXT);
			
			//CMYK conversion formulas assume all RGB are between 0-1.0
			//this should use CMYK function
			//double arrdCMYK[3];
			//RGBtoCMYK(arrdRGB[0], arrdRGB[1], arrdRGB[2], arrdCMYK);
			//drawParamText(78, 150, const_cast<char*>(LeftPadAndTruncate(arrdCMYK[2]).c_str()), color_MAINTEXT);
			//drawParamText(78, 176, const_cast<char*>(LeftPadAndTruncate(arrdCMYK[0]).c_str()), color_MAINTEXT);
			//drawParamText(78, 202, const_cast<char*>(LeftPadAndTruncate(arrdCMYK[1]).c_str()), color_MAINTEXT);	
			//drawParamText(78, 228, const_cast<char*>(LeftPadAndTruncate(arrdCMYK[3]).c_str()), color_MAINTEXT);	
			
			fRed = arrdRGB[0] / 255;
			fGreen = arrdRGB[1] / 255;
			fBlue = arrdRGB[2] / 255;			
			
			double dblack = (1 - threeway_max((double)fRed, (double)fGreen, (double)fBlue));
			//Serial.println(String(dblack));
			//strTemp = String((int)(dblack * 100));
			drawParamText(78 + GetBuffer(dblack *100), 228, const_cast<char*>(TruncateDouble(dblack * 100).c_str()), color_MAINTEXT);
			//use rgb array for cmy to minimize memory use
			// red -> magenta, green -> yellow, blue -> cyan
			arrdRGB[0] = ((1 - fGreen - dblack) / (1 - dblack)) * 100;
			arrdRGB[2] = ((1 - fRed - dblack) / (1 - dblack)) * 100;
			arrdRGB[1] = ((1 - fBlue - dblack) / (1 - dblack)) * 100; 
			
			drawParamText(78 + GetBuffer(arrdRGB[2]), 150, const_cast<char*>(TruncateDouble(arrdRGB[2]).c_str()), color_MAINTEXT);
			drawParamText(78 + GetBuffer(arrdRGB[0]), 176, const_cast<char*>(TruncateDouble(arrdRGB[0]).c_str()), color_MAINTEXT);
			drawParamText(78 + GetBuffer(arrdRGB[1]), 202, const_cast<char*>(TruncateDouble(arrdRGB[1]).c_str()), color_MAINTEXT);	
			
			
			//draw swatch, turn off scanner LED
			tft.fillRoundRect(186, 189, 113, 42, 10, nRGB565);
			rgbSensor.setInterrupt(true);
			
			//free(arrdRGB);
			
			mnLastRGBScan = millis();
		} else {
			//give timer until next scan?
			//37 58		6 14
			int nCurrentRGBCooldown = (mnRGBScanInterval / 1000) - (int)((millis() - mnLastRGBScan) / 1000);
			if (nCurrentRGBCooldown != mnRGBCooldown && nCurrentRGBCooldown < 5) {
				mnRGBCooldown = nCurrentRGBCooldown;
				tft.fillRect(37, 55, 7, 18, color_HEADER);
				if (mnRGBCooldown > 0) {
					drawParamText(37, 71, const_cast<char*>(((String)mnRGBCooldown).c_str()), ST77XX_BLACK);
				}
			}
		}
	
	}
}
