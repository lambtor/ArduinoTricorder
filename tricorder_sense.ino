#include <Adafruit_NeoPixel.h>
#include <SPI.h>
#include <Adafruit_GFX.h>    	// Core graphics library
#include <Adafruit_ST7789.h> 	// Hardware-specific library for ST7789
#include <string.h>
#include <Adafruit_APDS9960.h>	//RGB, light, proximity, gesture sensor
#include <Wire.h>
#include <EasyButton.h>
#include <Adafruit_BMP280.h>	//altitude, temp, barometer
#include <Adafruit_SHT31.h>

// need to remove hyphens from header filenames or exception will get thrown
#include "Fonts/lcars15pt7b.h"
#include "Fonts/lcars11pt7b.h"

// For the breakout board, you can use any 2 or 3 pins.
// These pins will also work for the 1.8" TFT shield.
//need to use pin 10 for TFT_CS, as pin 9 is analog 7. analog 7 is the only way to get current voltage, which is used for battery %
#define TFT_CS 10
// SD card select pin
//#define SD_CS 11 - can't use pin 4 as that is for blue connection led
#define TFT_RST -1
#define TFT_DC 5
#define USE_SD_CARD 1

//pin 9 can pull power level (used for battery %)?
//cannot use this pin for anything else if you care about battery %
#define VOLT_PIN PIN_A6

//buttons, scroller	- d2 pin actually pin #2?
#define BUTTON_1_PIN		PIN_A4
#define BUTTON_2_PIN		PIN_A5
#define BUTTON_3_PIN		2

// A0 is pin14. can't use that as an output pin?		A0 = 14, A3 = 17
#define SCAN_LED_PIN_1 PIN_A0	//14
#define SCAN_LED_PIN_2 PIN_A1	//15
#define SCAN_LED_PIN_3 PIN_A2	//16
#define SCAN_LED_PIN_4 PIN_A3	//17

#define SCAN_LED_BRIGHTNESS 32

// power LED. must use an unreserved pin for this.
// cdn-learn.adafruit.com/assets/assets/000/046/243/large1024/adafruit_products_Feather_M0_Adalogger_v2.2-1.png?1504885273
#define POWER_LED_PIN 6
#define NEOPIXEL_BRIGHTNESS 32
#define NEOPIXEL_LED_COUNT 3
// built-in pins: D4 = blue conn LED, 8 = neopixel on board, D13 = red LED next to micro usb port
#define NEOPIXEL_BOARD_LED_PIN	8

// TNG colors here
#define color_SWOOP			0xF7B3
#define color_MAINTEXT			0xC69F
#define color_LABELTEXT		0x841E
#define color_HEADER			0xFEC8
#define color_TITLETEXT		0xFEC8
#define color_LABELTEXT2		0xC5D2
#define color_LABELTEXT3		0xCD7B
#define color_LEGEND			0x6A62

// ds9
//#define color_SWOOP			0xD4F0
//#define color_MAINTEXT		0xBD19
//#define color_LABELTEXT		0x7A8D
//#define color_HEADER			0xECA7
//#define color_TITLETEXT		0xC3ED
//#define color_LABELTEXT2		0xC5D2
// voy
//#define color_SWOOP			0x9E7F
//#define color_MAINTEXT		0x7C34
//#define color_LABELTEXT		0x9CDF
//#define color_HEADER			0xCB33
//#define color_TITLETEXT		0xFFAF
//#define color_LABELTEXT2		0xC5D2

#define color_REDLABELTEXT		0xE000
#define color_REDDARKLABELTEXT	0x9800
#define color_REDDATATEXT		0xDEFB

#define RGBto565(r,g,b) ((((r) & 0xF8) << 8) | (((g) & 0xFC) << 3) | ((b) >> 3))

Adafruit_NeoPixel ledPwrStrip(NEOPIXEL_LED_COUNT, POWER_LED_PIN, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel ledBoard(NEOPIXEL_LED_COUNT, NEOPIXEL_BOARD_LED_PIN, NEO_GRB + NEO_KHZ800);

// do not fuck with this. 2.0 IS THE BOARD
Adafruit_ST7789 tft = Adafruit_ST7789(TFT_CS, TFT_DC, TFT_RST);
//create sensor objects
Adafruit_APDS9960 oColorSensor;
Adafruit_BMP280 oTempBarom;
Adafruit_SHT31 oHumid;

//power & board color enumerator: blue = 4, green = 3, yellow = 2, orange = 1, red = 0
int mnPowerColor = 4;
int mnBoardColor = 4;
int mnPowerLEDInterval = 5000;
int mnEMRGLEDInterval = 100;
int mnEMRGMinStrength = 8;
int mnEMRGMaxStrength = 224;
int mnEMRGCurrentStrength = 8;
bool mbLEDIDSet = false;
unsigned long mnLastUpdatePower = 0;
unsigned long mnLastUpdateEMRG = 0;
bool mbEMRGdirection = false;
int mnBoardLEDInterval = 5000;
unsigned long mnLastUpdateBoard = 0;

//left scanner leds
int mnLeftLEDInterval = 175;
int mnLeftLEDCurrent = 0;
unsigned long mnLastUpdateLeftLED = 0;

//color sensor
bool mbColorInitialized = false;
bool mbRGBActive = false;
int mnRGBScanInterval = 5000;
int mnRGBCooldown = 0;
unsigned long mnLastRGBScan = 0;

//temperature, humidity, pressure
//Average sea-level pressure is 1013.25 mbar
bool mbTempInitialized = false;
bool mbHumidityInitialized = false;
bool mbTempActive = false;
float mfTempC = 0.0;
float mfTempK = 0.0;
float mfHumid = 0.0;
int mnBarom = 0;
int mnClimateScanInterval = 2000;
int mnClimateCooldown = 0;
int mnTempTargetBar = 0;
int mnTempCurrentBar = 0;
int mnHumidTargetBar = 0;
int mnHumidCurrentBar = 0;
int mnBaromTargetBar = 0;
int mnBaromCurrentBar = 0;
int mnClimateBarStart = 62;
unsigned long mnLastTempBar = 0;
unsigned long mnLastBaromBar = 0;
unsigned long mnLastHumidBar = 0;
//17 ms is 60fps. setting this to 120fps, at least for initial crawl
unsigned long mnBarDrawInterval = 9;
bool mbHumidBarComplete = false;
bool mbTempBarComplete = false;
bool mbBaromBarComplete = false;
int marrScaleNotches[] = {123, 185, 247};
unsigned long mnLastClimateScan = 0;

//button functionality
bool mbButton1Flag = false;
bool mbButton2Flag = false;
bool mbButton3Flag = false;
EasyButton oButton1(BUTTON_1_PIN);
EasyButton oButton2(BUTTON_2_PIN);
EasyButton oButton3(BUTTON_3_PIN);

void setup() {
	ledPwrStrip.begin();
	ledBoard.begin();
	
	// OR use this initializer (uncomment) if using a 2.0" 320x240 TFT:
	tft.init(240, 320, SPI_MODE0); // Init ST7789 320x240
	tft.setRotation(1);
	tft.setFont(&lcars11pt7b);
	//these googles, they do nothing!
	tft.setTextWrap(false);
	
	//Serial.begin(9600);
	
	ledPwrStrip.clear();
	ledBoard.clear();
	// max brightness is 255
	// ledStrip.setBrightness(255);
	ledPwrStrip.setBrightness(NEOPIXEL_BRIGHTNESS);
	ledBoard.setBrightness(NEOPIXEL_BRIGHTNESS);
	
	//set pinmode output for scanner leds
	pinMode(SCAN_LED_PIN_1, OUTPUT);
	pinMode(SCAN_LED_PIN_2, OUTPUT);
	pinMode(SCAN_LED_PIN_3, OUTPUT);
	pinMode(SCAN_LED_PIN_4, OUTPUT);
	
	//initialize color sensor, show error if unavailable. sensor hard-coded name is "ADPS"
	//begin will return false if initialize failed. 
	//this shit is super plug & play - library uses i2c address 0x39, same as one used by this board
	//DO NOT call any functions before the begin, or you'll lock up the board
	mbColorInitialized = oColorSensor.begin();	
	
	if (mbColorInitialized) {
		//need rgb to cap at 255 for calculations? this isn't limiting shit.
		oColorSensor.setIntLimits(0, 255);
		oColorSensor.enableColor(true);
		oColorSensor.setADCGain(APDS9960_AGAIN_16X);
		oColorSensor.enableColorInterrupt();
		oColorSensor.setADCIntegrationTime(50);		
	}
	
	//temp sensor
	mbTempInitialized = oTempBarom.begin();
	mbHumidityInitialized = oHumid.begin();	
	
	oButton2.begin();
	oButton2.onPressed(ToggleRGBSensor);	
	
	oButton1.begin();
	oButton1.onPressed(ToggleClimateSensor);
	
	GoHome();
}

void loop() {
	//this toggles RGB scanner
	oButton2.read();
	oButton1.read();
	
	NeoPixelColor(POWER_LED_PIN);
	LeftScanner();
	
	RunRGBSensor();
	RunClimateSensor();
}

void NeoPixelColor(int nPin) {
	unsigned long lTimer = millis();
	if (nPin == POWER_LED_PIN) {
		if (mnLastUpdatePower == 0 || ((lTimer - mnLastUpdatePower) > mnPowerLEDInterval)) {
			//these will need to have their own intervals
			//switch should eventually be changed to poll voltage pin - pin#, r,g,b
			//cycle order = blue, green, yellow, orange, red
			switch (mnPowerColor) {
				case 4: ledPwrStrip.setPixelColor(0, 0, 0, 128); mnPowerColor = 3; break;
				case 3: ledPwrStrip.setPixelColor(0, 0, 128, 0); mnPowerColor = 2; break;
				case 2: ledPwrStrip.setPixelColor(0, 112, 128, 0); mnPowerColor = 1; break;
				case 1: ledPwrStrip.setPixelColor(0, 128, 96, 0); mnPowerColor = 0; break;
				default: ledPwrStrip.setPixelColor(0, 128, 0, 0); mnPowerColor = 4; break;
			}
			if (!mbLEDIDSet) {
				ledPwrStrip.setPixelColor(1, 96, 128, 0);
				mbLEDIDSet = true;
				//ledPwrStrip.setPixelColor(2, 128, 0, 0);
			}
						
			mnLastUpdatePower = lTimer;
			ledPwrStrip.show();
		} 
		if ((lTimer - mnLastUpdateEMRG) > mnEMRGLEDInterval) {
			int nCurrentEMRG = mnEMRGCurrentStrength;
			int nEMRGIncrement = (mnEMRGMaxStrength - mnEMRGMinStrength) / (mnEMRGLEDInterval / 8);
			if (nCurrentEMRG >= mnEMRGMaxStrength || nCurrentEMRG <= mnEMRGMinStrength) {
				mbEMRGdirection = !mbEMRGdirection;
			}
			nCurrentEMRG = nCurrentEMRG + ((mbEMRGdirection == true ? 1 : -1) * nEMRGIncrement);
			
			if (nCurrentEMRG > mnEMRGMaxStrength)
				nCurrentEMRG == mnEMRGMaxStrength;
			if (nCurrentEMRG < mnEMRGMinStrength)
				nCurrentEMRG == mnEMRGMinStrength;
			mnEMRGCurrentStrength = nCurrentEMRG;
			ledPwrStrip.setPixelColor(2, nCurrentEMRG, 0, 0);
			ledPwrStrip.show();
			mnLastUpdateEMRG = lTimer;
		}		
	} else if (nPin == NEOPIXEL_BOARD_LED_PIN) {
		//unsure if want to use, as this needs to be sensor flash.
		if ((lTimer - mnLastUpdateBoard) > mnBoardLEDInterval) {
			//switch should eventually be changed to poll voltage pin
			//cycle order = blue, green, yellow, orange, red
			switch (mnBoardColor) {
				case 4: ledBoard.setPixelColor(0, 0, 0, 128); mnBoardColor = 3; break;
				case 3: ledBoard.setPixelColor(0, 0, 128, 0); mnBoardColor = 2; break;
				case 2: ledBoard.setPixelColor(0, 112, 128, 0); mnBoardColor = 1; break;
				case 1: ledBoard.setPixelColor(0, 128, 96, 0); mnBoardColor = 0; break;
				default: ledBoard.setPixelColor(0, 128, 0, 0); mnBoardColor = 4; break;
			}
			mnLastUpdateBoard = lTimer;
			ledBoard.show();
		}
	}
}

void ActivateFlash() {
	//all neopixel objects are chains, so have to call it by addr
	ledBoard.setPixelColor(0, 255, 255, 255);
	ledBoard.show();
}

//to-do: add parameters to change cycle behavior of LEDs.
//ex: cycled down-> up, stacking, unified blink, KITT ping pong, etc
void LeftScanner() {
	unsigned long lTimer = millis();
	
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

void GoHome() {
	//reset any previous sensor statuses
	mnRGBCooldown = 0;
	mnClimateCooldown = 0;
	
	mbRGBActive = false;
	mbTempActive = false;
	
	mbButton1Flag = false;
	mbButton2Flag = false;
	mbButton3Flag = false;
	
	//reset any bar graph values from climate
	mnTempTargetBar = 0;
	mnTempCurrentBar = 0;
	mnHumidTargetBar = 0;
	mnHumidCurrentBar = 0;
	mnBaromTargetBar = 0;
	mnBaromCurrentBar = 0;
	mbHumidBarComplete = false;
	mbTempBarComplete = false;
	mbBaromBarComplete = false;

	//tft.setFont(&lcars11pt7b);
	tft.setFont(&lcars15pt7b);

	// large block of text
	tft.fillScreen(ST77XX_BLACK);
	// home screen header is 2 rounded rectangles, lines to cut them, 1 black rect as backing for header text
	//fillRoundRect(x,y,width,height,cornerRadius, color)
	tft.fillRoundRect(3, 1, 316, 20, 9, color_SWOOP);
	tft.fillRoundRect(3, 219, 316, 20, 9, color_SWOOP);
	//left side slots
	tft.drawFastVLine(15, 1, 312, ST77XX_BLACK);
	tft.drawFastVLine(16, 1, 312, ST77XX_BLACK);
	tft.drawFastVLine(17, 1, 312, ST77XX_BLACK);
	tft.drawFastVLine(18, 1, 312, ST77XX_BLACK);
	tft.drawFastVLine(19, 1, 312, ST77XX_BLACK);
	tft.drawFastVLine(20, 1, 312, ST77XX_BLACK);
	//bottom right slots
	tft.drawFastVLine(305, 219, 20, ST77XX_BLACK);
	tft.drawFastVLine(304, 219, 20, ST77XX_BLACK);
	tft.drawFastVLine(303, 219, 20, ST77XX_BLACK);
	tft.drawFastVLine(302, 219, 20, ST77XX_BLACK);	
	tft.drawFastVLine(301, 219, 20, ST77XX_BLACK);
	tft.drawFastVLine(300, 219, 20, ST77XX_BLACK);

	//black section for home screen title
	tft.fillRect(237, 1, 68, 20, ST77XX_BLACK);
	drawParamText(234, 20, "  STATUS", color_MAINTEXT);

	//set font to 11pt since header is done
	tft.setFont(&lcars11pt7b);		

	//show sensor statuses	
	if (mbColorInitialized) {
		drawParamText(20, 54, "CHROMATICS", color_LABELTEXT);
		drawParamText(95, 54, "ONLINE", color_MAINTEXT);
	} else {
		//use red alert theme for any errors
		drawParamText(20, 54, "CHROMATICS", color_REDLABELTEXT);
		drawParamText(95, 54, "OFFLINE", color_REDDATATEXT);
	}
	
	//environment = temperature, humidity sensors
	if (mbTempInitialized) {
		drawParamText(20, 84, "CLIMATE", color_LABELTEXT);
		drawParamText(95, 84, mbHumidityInitialized ? "ONLINE" : "PARTIAL", color_MAINTEXT);
	} else if (mbHumidityInitialized) {
		drawParamText(20, 84, "CLIMATE", color_LABELTEXT);
		drawParamText(95, 84, "PARTIAL", color_MAINTEXT);
	} else {
		drawParamText(20, 54, "CLIMATE", color_REDLABELTEXT);
		drawParamText(95, 54, "OFFLINE", color_REDDATATEXT);
	}

	ShowBatteryLevel(222, 54, color_LABELTEXT, color_MAINTEXT);
	
	//bottom crawling text	?
	String sWarningText = "UNITED FEDERATION OF PLANETS";
	//drawWalkingText(75, 200, const_cast<char*>(sWarningText.c_str()), color_MAINTEXT);
	//drawParamText(75, 200, const_cast<char*>(sWarningText.c_str()), color_MAINTEXT);
	//erase crawling text
	//tft.fillRect(75, 180, 180, 25, ST77XX_BLACK);
}

void ToggleRGBSensor() {
	mbButton2Flag = !mbButton2Flag;
	//reset any temperature app values
	mbTempActive = false;
	mnTempTargetBar = 0;
	mnTempCurrentBar = 0;
	mnHumidTargetBar = 0;
	mnHumidCurrentBar = 0;
	mnBaromTargetBar = 0;
	mnBaromCurrentBar = 0;
	mbHumidBarComplete = false;
	mbTempBarComplete = false;
	mbBaromBarComplete = false;
	
	if (mbButton2Flag) {
		//to-do: if sensor disabled or not started, pulse message to display
		if (!mbRGBActive) {
			mbRGBActive = true;
			//load rgb scanner screen - this is done once to improve perf
			tft.fillScreen(ST77XX_BLACK);
			tft.fillRoundRect(0, -25, 100, 140, 25, color_SWOOP);
			tft.fillRoundRect(0, 120, 100, 140, 25, color_SWOOP);
			tft.drawFastHLine(24, 112, 296, color_SWOOP);
			tft.drawFastHLine(24, 113, 296, color_SWOOP);
			tft.drawFastHLine(24, 114, 296, color_SWOOP);
			tft.drawFastHLine(24, 115, 296, color_SWOOP);		
			
			tft.drawFastHLine(24, 120, 296, color_SWOOP);
			tft.drawFastHLine(24, 121, 296, color_SWOOP);
			tft.drawFastHLine(24, 122, 296, color_SWOOP);
			tft.drawFastHLine(24, 123, 296, color_SWOOP);			
						
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
			drawParamText(154, 21, "CHROMATIC ANALYSIS", color_TITLETEXT);
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
		GoHome();		
		//Serial.println("button 2 no flag");
		//tft.fillRoundRect(140, 110, 100, 50, 10, ST77XX_GREEN);
	}
}

void RunRGBSensor() {
	//exit if button config says this shouldn't be running.
	if (!mbRGBActive) return;
	
	//erase values if interval has passed
	if (millis() - mnLastRGBScan > mnRGBScanInterval) {
		tft.fillRect(175, 135, 59, 44, ST77XX_BLACK);
		tft.fillRect(198, 31, 28, 70, ST77XX_BLACK);
		tft.fillRect(74, 31, 30, 70, ST77XX_BLACK);
		tft.fillRect(74, 135, 27, 95, ST77XX_BLACK);
		
		uint16_t nRed, nGreen, nBlue, nClear, nTempMax;
		nTempMax = 0;
		ActivateFlash();
		while(!oColorSensor.colorDataReady() || nTempMax == 0) {
			delay(50);
			nTempMax = 1;
		}		
		oColorSensor.getColorData(&nRed, &nGreen, &nBlue, &nClear);
		
		if (nRed > 255 || nGreen > 255 || nBlue > 255) {
			//convert rgb to 0-255 range based on w/e we have, if its trash?		
			nTempMax = ThreewayIntMax(nRed, nGreen, nBlue);
			nRed = map(nRed, 0, nTempMax, 0, 255);
			nGreen = map(nGreen, 0, nTempMax, 0, 255);
			nBlue = map(nBlue, 0, nTempMax, 0, 255);
		}
								
		///convert rgb to HSL
		double arrdHSL[2];
		double arrdHSV[2];
		double arrdRGB[2];			
		
		//displayRGB
		drawParamText(78 + GetBuffer((double)nRed), 48, String(nRed), color_MAINTEXT);
		drawParamText(78 + GetBuffer((double)nGreen), 74, String(nGreen), color_MAINTEXT);
		drawParamText(78 + GetBuffer((double)nBlue), 99, String(nBlue), color_MAINTEXT);
		
		arrdRGB[0] = nRed;
		arrdRGB[1] = nGreen;
		arrdRGB[2] = nBlue;
		
		//uncomment this when need to display HSL conversion
		RGBtoHSL((double)nRed, (double)nGreen, (double)nBlue, arrdHSL);
		//display HSL values
		drawParamText(203 + GetBuffer(arrdHSL[0]), 48, const_cast<char*>(TruncateDouble(arrdHSL[0]).c_str()), color_MAINTEXT);
		drawParamText(203 + GetBuffer(arrdHSL[1]), 74, const_cast<char*>(TruncateDouble(arrdHSL[1]).c_str()), color_MAINTEXT);
		drawParamText(203 + GetBuffer(arrdHSL[2]), 99, const_cast<char*>(TruncateDouble(arrdHSL[2]).c_str()), color_MAINTEXT);
		
		uint16_t nRGB565 = RGBto565((uint16_t)arrdRGB[0], (uint16_t)arrdRGB[1], (uint16_t)arrdRGB[2]);
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
		strTemp = String(nRGB565, HEX);
		strTemp.toUpperCase();
		strTemp = "0x" + strTemp;
		drawParamText(185, 150, const_cast<char*>(strTemp.c_str()), color_MAINTEXT);
		
		//CMYK conversion formulas assume all RGB are between 0-1.0
		//this should use CMYK function
		//double arrdCMYK[3];
		//RGBtoCMYK(arrdRGB[0], arrdRGB[1], arrdRGB[2], arrdCMYK);
		//drawParamText(78, 150, const_cast<char*>(LeftPadAndTruncate(arrdCMYK[2]).c_str()), color_MAINTEXT);
		//drawParamText(78, 176, const_cast<char*>(LeftPadAndTruncate(arrdCMYK[0]).c_str()), color_MAINTEXT);
		//drawParamText(78, 202, const_cast<char*>(LeftPadAndTruncate(arrdCMYK[1]).c_str()), color_MAINTEXT);	
		//drawParamText(78, 228, const_cast<char*>(LeftPadAndTruncate(arrdCMYK[3]).c_str()), color_MAINTEXT);	
		double fRed, fGreen, fBlue;
		fRed = arrdRGB[0] / 255;
		fGreen = arrdRGB[1] / 255;
		fBlue = arrdRGB[2] / 255;			
		
		double dblack = (1 - ThreewayMax(fRed, fGreen, fBlue));
		
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
		tft.fillRoundRect(186, 189, 113, 42, 21, nRGB565);
		//turn off flash
		ledBoard.clear();
		ledBoard.show();
		
		mnLastRGBScan = millis();
	} else {
		//give timer until next scan?
		//37 58		6 14
		int nCurrentRGBCooldown = (mnRGBScanInterval / 1000) - (int)((millis() - mnLastRGBScan) / 1000);
		if (nCurrentRGBCooldown != mnRGBCooldown && nCurrentRGBCooldown < (mnRGBScanInterval / 1000)) {
			mnRGBCooldown = nCurrentRGBCooldown;
			tft.fillRect(37, 55, 7, 18, color_HEADER);
			if (mnRGBCooldown > 0) {
				drawParamText(37, 71, const_cast<char*>(((String)mnRGBCooldown).c_str()), ST77XX_BLACK);
			}
		}
	}	
}	//end runRGBSensor

void ShowBatteryLevel(int nPosX, int nPosY, uint16_t nLabelColor, uint16_t nValueColor) {
	//to-do: use global setting to enable/disable battery voltage check
	String sBatteryStatus = "";  
	float fBattV = analogRead(VOLT_PIN);
	/*
	//fBattV *= 2;    // we divided by 2 (board has a resistor on this pin), so multiply back,  // Multiply by 3.3V, our reference voltage
	//fBattV *= 3.3; 
	//combine previous actions for brevity- multiply by 2 to adjust for resistor, then 3.3 reference voltage
	fBattV *= 6.6;
	fBattV /= 1024; // convert to voltage
	//3.3 to 4.2 => subtract 3.3 (0) from current voltage. now values should be in range of 0-0.9 : multiply by 1.111, then by 100, convert result to int.
	fBattV -= 3.3;
	fBattV *= 111.11;
	*/
	int nBattPct = map(fBattV, 0, 600, 0, 101);
	//need to use space characters to pad string because it'll wrap back around to left edge when cursor set over 240 (height).
	//this is a glitch of adafruit GFX library - it thinks display is 240x320 despite the rotation in setup() and setting for text wrap
	//sBatteryStatus = "      " + String((int)fBattV);
	sBatteryStatus = "      " + String(nBattPct);
	//carrBatteryStatus = const_cast<char*>(sBatteryStatus.c_str());
	//tft.getTextBounds("POWER", 222, 54, &nTextPosX, &nTextPosY, &nTextWidth, &nTextHeight);
	drawParamText(nPosX + 20, nPosY, const_cast<char*>(sBatteryStatus.c_str()), color_MAINTEXT);
	
	drawParamText(nPosX, nPosY, "POWER", color_LABELTEXT);
}

void ToggleClimateSensor() {
	mbButton1Flag = !mbButton1Flag;
	//reset any rgb sensor values
	mnRGBCooldown = 0;
	mnClimateCooldown = 0;	
	mbRGBActive = false;
	
	if (mbButton1Flag) {
		if (!mbTempActive) {
			mbTempActive = true;
			//load temp scanner screen - this is done once to improve perf
			tft.fillScreen(ST77XX_BLACK);
			//tft.fillScreen(ST77XX_GREEN);
			tft.fillRoundRect(0, -25, 85, 113, 25, color_SWOOP);
			tft.fillRoundRect(0, 93, 85, 172, 25, color_SWOOP);
			tft.drawFastHLine(24, 84, 296, color_SWOOP);
			tft.drawFastHLine(24, 85, 296, color_SWOOP);
			tft.drawFastHLine(24, 86, 296, color_SWOOP);
			tft.drawFastHLine(24, 87, 296, color_SWOOP);		
			
			tft.drawFastHLine(24, 93, 296, color_SWOOP);
			tft.drawFastHLine(24, 94, 296, color_SWOOP);
			tft.drawFastHLine(24, 95, 296, color_SWOOP);
			tft.drawFastHLine(24, 96, 296, color_SWOOP);			
						
			tft.fillRoundRect(50, -3, 40, 87, 5, ST77XX_BLACK);
			tft.fillRoundRect(50, 97, 40, 148, 5, ST77XX_BLACK);
			tft.drawFastVLine(121, 83, 16, ST77XX_BLACK);
			tft.drawFastVLine(122, 83, 16, ST77XX_BLACK);
			tft.drawFastVLine(241, 83, 16, ST77XX_BLACK);
			tft.drawFastVLine(242, 83, 16, ST77XX_BLACK);
			
			//previously was 49,50 for top edge of timer
			tft.drawFastHLine(0, 12, 50, ST77XX_BLACK);
			tft.drawFastHLine(0, 13, 50, ST77XX_BLACK);
			tft.drawFastHLine(0, 14, 50, ST77XX_BLACK);
			tft.fillRect(0, 15, 50, 45, color_HEADER);
			tft.drawFastHLine(0, 57, 50, ST77XX_BLACK);
			tft.drawFastHLine(0, 58, 50, ST77XX_BLACK);
			tft.drawFastHLine(0, 59, 50, ST77XX_BLACK);
			//tft.fillRect(241, 110, 2, 16, ST77XX_BLACK);
			
			tft.fillRect(123, 86, 30, 8, ST77XX_BLACK);
			tft.setFont(&lcars15pt7b);
			drawParamText(174, 20, "CLIMATE ANALYSIS", color_TITLETEXT);			
			
			//data labels
			tft.setFont(&lcars11pt7b);
			tft.fillRoundRect(70, 31, 16, 16, 8, color_LABELTEXT2);			
			tft.fillRoundRect(70, 57, 16, 16, 8, color_LABELTEXT2);
			tft.fillRect(80, 31, 8, 42, ST77XX_BLACK);			
			tft.fillRoundRect(185, 31, 16, 16, 8, color_LABELTEXT);
			tft.fillRoundRect(185, 57, 16, 16, 8, color_LABELTEXT3);
			tft.fillRect(195, 31, 8, 42, ST77XX_BLACK);
			
			//120, 245
			//drawParamText(88, 46, "0.0", color_MAINTEXT);
			drawParamText(120, 46, "CELSIUS", color_LABELTEXT2);
			//drawParamText(88, 72, "000", color_MAINTEXT);
			drawParamText(120, 72, "KELVIN", color_LABELTEXT2);
			
			//drawParamText(209, 46, "000", color_MAINTEXT);
			drawParamText(240, 46, "HUMIDITY", color_LABELTEXT);
			//drawParamText(202, 72, "1000", color_MAINTEXT);
			drawParamText(240, 72, "MILLIBAR", color_LABELTEXT3);
			
			//establish bar graphs - top and bottom of 3 sections
			tft.drawFastHLine(61, 108, 248, color_HEADER);
			tft.drawFastHLine(61, 132, 248, color_HEADER);
			tft.drawFastHLine(61, 154, 248, color_HEADER);
			tft.drawFastHLine(61, 178, 248, color_HEADER);
			tft.drawFastHLine(61, 200, 248, color_HEADER);
			tft.drawFastHLine(61, 224, 248, color_HEADER);
			
			//sides of 3 sections
			tft.drawFastVLine(61, 108, 24, color_HEADER);
			tft.drawFastVLine(61, 154, 24, color_HEADER);
			tft.drawFastVLine(61, 200, 24, color_HEADER);
			tft.drawFastVLine(308, 108, 24, color_HEADER);
			tft.drawFastVLine(308, 154, 24, color_HEADER);
			tft.drawFastVLine(308, 200, 24, color_HEADER);
			
			//quarter markers
			tft.drawFastVLine(123, 108, 24, color_HEADER);
			tft.drawFastVLine(123, 154, 24, color_HEADER);
			tft.drawFastVLine(123, 200, 24, color_HEADER);
			//half
			tft.drawFastVLine(185, 108, 24, color_HEADER);
			tft.drawFastVLine(185, 154, 24, color_HEADER);
			tft.drawFastVLine(185, 200, 24, color_HEADER);
			//3 quarters
			tft.drawFastVLine(247, 108, 24, color_HEADER);
			tft.drawFastVLine(247, 154, 24, color_HEADER);
			tft.drawFastVLine(247, 200, 24, color_HEADER);
			
			//bottom label line of each section
			tft.drawFastHLine(61, 138, 248, color_LEGEND);
			tft.drawFastHLine(61, 184, 248, color_LEGEND);
			tft.drawFastHLine(61, 230, 248, color_LEGEND);
			
			//graph scale labels.  ideally, these would update/shift as readings approach either edge of graph.			
			drawTinyInt(58, 143, -20, ST77XX_WHITE, ST77XX_BLACK);
			drawTinyInt(122, 143, 0, ST77XX_WHITE, ST77XX_BLACK);
			drawTinyInt(183, 143, 20, ST77XX_WHITE, ST77XX_BLACK);
			drawTinyInt(245, 143, 40, ST77XX_WHITE, ST77XX_BLACK);
			drawTinyInt(306, 143, 60, ST77XX_WHITE, ST77XX_BLACK);
			//2nd graph
			drawTinyInt(62, 189, 0, ST77XX_WHITE, ST77XX_BLACK);
			drawTinyInt(120, 189, 25, ST77XX_WHITE, ST77XX_BLACK);
			drawTinyInt(183, 189, 50, ST77XX_WHITE, ST77XX_BLACK);
			drawTinyInt(245, 189, 75, ST77XX_WHITE, ST77XX_BLACK);
			drawTinyInt(304, 189, 100, ST77XX_WHITE, ST77XX_BLACK);
			//3rd  graph
			drawTinyInt(57, 235, 800, ST77XX_WHITE, ST77XX_BLACK);
			drawTinyInt(117, 235, 900, ST77XX_WHITE, ST77XX_BLACK);
			drawTinyInt(179, 235, 1000, ST77XX_WHITE, ST77XX_BLACK);
			drawTinyInt(243, 235, 1100, ST77XX_WHITE, ST77XX_BLACK);
			drawTinyInt(302, 235, 1200, ST77XX_WHITE, ST77XX_BLACK);
		}
	} else {
		//return to main screen
		GoHome();		
	}
	
}	//end toggle climate

void RunClimateSensor() {
	if (!mbTempActive) return;
	
	if (millis() - mnLastClimateScan > mnClimateScanInterval) {
		//pull data - temp is Celsius by default, 
		float fTempC = 0.0;
		int nBarom = 0;
		float fHumid = 0.0;
		
		if (mbTempInitialized) {
			fTempC = oTempBarom.readTemperature();
			//pressure is pascals by default. need to divide by 100 to convert to millibar
			nBarom = (int)(oTempBarom.readPressure() / 100.0);
		}
		//do not read humidity if last value hasn't been displayed on bar graph yet
		if (mbHumidityInitialized) {
			fHumid = oHumid.readHumidity();
		}
		
		//round temp to 1 decimal place for comparisons
		if (fTempC != mfTempC) {
			mfTempC = round(fTempC * 10) / 10.0;
			//blank out area where these numbers live first
			tft.fillRect(80, 31, 33, 42, ST77XX_BLACK);
			String sTempC = (String)mfTempC;
			//slash last character from string
			sTempC.remove(sTempC.length() - 1);
			//celsius
			drawParamText(86, 46, const_cast<char*>(sTempC.c_str()), color_MAINTEXT);
			//kelvin - int, should always be 3 digits - this is a straight conversion
			int nTempK = (int)(fTempC + 273.15);
			drawParamText(90, 72, const_cast<char*>(((String)nTempK).c_str()), color_MAINTEXT);
			
			//map values to bar graph, store plotted value so future plots can shave or append
			//map range is from labels to max width of bar [308-61]
			mnTempTargetBar = map(fTempC, -20, 60, 0, 247);
			//set flag for temp needs update
			mbTempBarComplete = false;
		}
		
		if (fHumid != mfHumid || mnBarom != nBarom) {
			//humidity % to 1 decimal place - barometer convert to int
			mnBarom = nBarom;
			mfHumid = round(fHumid * 10) / 10.0;
			tft.fillRect(195, 31, 42, 42, ST77XX_BLACK);
			String sHumid = (String)mfHumid;
			//slash last character from string
			sHumid.remove(sHumid.length() - 1);
			
			drawParamText(204, 46, const_cast<char*>(sHumid.c_str()), color_MAINTEXT);
			//barometric pressure - maximum human survivable is ~ 70*14.7 psi ~= 70000 millibar (sea diving)
			drawParamText(202 + Get1KBuffer(mnBarom), 72, const_cast<char*>(((String)nBarom).c_str()), color_MAINTEXT);
			
			//set temp, current values for bar graph function
			mnHumidTargetBar = map(mfHumid, 0, 100, 0, 247);
			mnBaromTargetBar = map(mnBarom, 800, 1200, 0, 247);
			mbHumidBarComplete = false;
			mbBaromBarComplete = false;
		}
		
		//humidity
		//tft.fillRect(195, 31, 8, 42, ST77XX_BLACK);
		//drawParamText(209, 46, "00.00", color_MAINTEXT);
		//pressure - 206-219
		//drawParamText(202, 72, "1000", color_MAINTEXT);
		//heat index:
		//H = T + (0.5555 * (e - 10)), where T is the temperature in Celsius and e is the vapor pressure in millibars (mb)
		
		mnLastClimateScan = millis();
	} else {
		//update bar graphs if needed.  match label colors used in stat header
		UpdateClimateBarGraph(0, color_LABELTEXT2);
		UpdateClimateBarGraph(1, color_LABELTEXT);
		UpdateClimateBarGraph(2, color_LABELTEXT3);		
	}	
}

void RGBtoHSL(double r, double g, double b, double hsl[]) { 
	//this assumes byte - need to divide by 255 to get byte?
    double rd = r/255;
    double gd = g/255;
    double bd = b/255;
	//double rd = (double)r;
	//double gd = (double)g;
	//double bd = (double)b;
    double max = ThreewayMax(rd, gd, bd);
    double min = ThreewayMin(rd, gd, bd);
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

int ThreewayIntMax(uint16_t a, uint16_t b, uint16_t c) {
    return max(a, max(b, c));
}

double ThreewayMax(double a, double b, double c) {
    return max(a, max(b, c));
}

double ThreewayMin(double a, double b, double c) {
    return min(a, min(b, c));
}

int GetBuffer(double dInput) {
	if (dInput < 10) {
		return 14;
	} else if (dInput < 100) {
		return 8;
	} 
	return 0;
}

int Get1KBuffer(int nInput) {
	return (nInput < 1000) ? 8 : 0;
}

String TruncateDouble(double dInput) {
	//badlypads to 3 digits. instead have a function to return pad pixel amount, use that in text print call - more precise this way.
	int nInput = (int)dInput;
	String sResult = (String)nInput;
	return sResult;
}

String PrintHex16(uint16_t *data) {
	char tmp[16];
	for (int i = 0; i < 5; i++) { 
		sprintf(tmp, "0x%.4X", data[i]); 
		Serial.print(tmp); 
		Serial.print(" ");
	}
	return (String)tmp;
}

void drawParamText(uint8_t nPosX, uint8_t nPosY, char *sText, uint16_t nColor) {
	tft.setCursor(nPosX, nPosY);
	tft.setTextColor(nColor);
	tft.setTextWrap(true);
	tft.print(sText);
}

void drawParamText(uint8_t nPosX, uint8_t nPosY, String sText, uint16_t nColor) {
	tft.setCursor(nPosX, nPosY);
	tft.setTextColor(nColor);
	tft.setTextWrap(true);
	tft.print(sText);
}

void drawTinyInt(int nPosX, int nPosY, int nValue, uint16_t nFColor, uint16_t nBGColor) {
	switch(nValue) {
		case 0: 
			//0
			tft.fillRect(nPosX, nPosY, 3, 3, nFColor);
			tft.drawPixel(nPosX + 1, nPosY + 1, nBGColor);
			break;
		case 20: 
			//2
			tft.fillRect(nPosX, nPosY, 3, 3, nFColor);
			tft.drawPixel(nPosX, nPosY, nBGColor);
			tft.drawPixel(nPosX + 2, nPosY + 2, nBGColor);
			//0
			tft.fillRect(nPosX + 5, nPosY, 3, 3, nFColor);
			tft.drawPixel(nPosX + 6, nPosY + 1, nBGColor);
			break;
		case 25:	
			//2
			tft.fillRect(nPosX, nPosY, 3, 3, nFColor);
			tft.drawPixel(nPosX, nPosY, nBGColor);
			tft.drawPixel(nPosX + 2, nPosY + 2, nBGColor);			
			//5
			tft.fillRect(nPosX + 4, nPosY, 3, 3, nFColor);
			tft.drawPixel(nPosX + 6, nPosY, nBGColor);
			tft.drawPixel(nPosX + 4, nPosY + 2, nBGColor);
			break;
		case 40:	
			//0
			tft.drawFastVLine(nPosX, nPosY, 2, nFColor);
			tft.drawFastVLine(nPosX + 2, nPosY, 3, nFColor);
			tft.drawPixel(nPosX + 1, nPosY + 1, nFColor);
			//0
			tft.fillRect(nPosX + 5, nPosY, 3, 3, nFColor);
			tft.drawPixel(nPosX + 6, nPosY + 1, nBGColor);
			break;
		case 50:	
			//5
			tft.fillRect(nPosX, nPosY, 3, 3, nFColor);
			tft.drawPixel(nPosX + 2, nPosY, nBGColor);
			tft.drawPixel(nPosX, nPosY + 2, nBGColor);
			//0
			tft.fillRect(nPosX + 5, nPosY, 3, 3, nFColor);
			tft.drawPixel(nPosX + 6, nPosY + 1, nBGColor);
			break;
		case 60:	
			//6
			tft.fillRect(nPosX, nPosY + 1, 3, 2, nFColor);
			tft.drawPixel(nPosX, nPosY, nFColor);
			//0
			tft.fillRect(nPosX + 5, nPosY, 3, 3, nFColor);
			tft.drawPixel(nPosX + 6, nPosY + 1, nBGColor);
			break;
		case 75:	
			//7
			tft.drawFastHLine(nPosX, nPosY, 3, nFColor);
			tft.drawPixel(nPosX + 2, nPosY + 1, nFColor);
			tft.drawPixel(nPosX + 1, nPosY + 2, nFColor);
			//5
			tft.fillRect(nPosX + 4, nPosY, 3, 3, nFColor);
			tft.drawPixel(nPosX + 6, nPosY, nBGColor);
			tft.drawPixel(nPosX + 4, nPosY + 2, nBGColor);
			break;
		case 100:	
			tft.drawFastVLine(nPosX, nPosY, 3, nFColor);
			//0
			tft.fillRect(nPosX + 3, nPosY, 3, 3, nFColor);
			tft.drawPixel(nPosX + 4, nPosY + 1, nBGColor);
			//0
			tft.fillRect(nPosX + 7, nPosY, 3, 3, nFColor);
			tft.drawPixel(nPosX + 8, nPosY + 1, nBGColor);
			break;
		case 800:	
			//8
			tft.fillRect(nPosX, nPosY, 3, 3, nFColor);
			//0
			tft.fillRect(nPosX + 4, nPosY, 3, 3, nFColor);
			tft.drawPixel(nPosX + 1, nPosY + 1, nBGColor);
			//0
			tft.fillRect(nPosX + 8, nPosY, 3, 3, nFColor);
			tft.drawPixel(nPosX + 9, nPosY + 1, nBGColor);
			break;
		case 900:	
			tft.drawFastVLine(nPosX, nPosY, 2, nFColor);
			tft.drawFastVLine(nPosX + 1, nPosY, 2, nFColor);
			tft.drawFastVLine(nPosX + 2, nPosY, 3, nFColor);
			//0
			tft.fillRect(nPosX + 4, nPosY, 3, 3, nFColor);
			tft.drawPixel(nPosX + 5, nPosY + 1, nBGColor);
			//0
			tft.fillRect(nPosX + 8, nPosY, 3, 3, nFColor);
			tft.drawPixel(nPosX + 9, nPosY + 1, nBGColor);
			break;
		case 1000:	
			tft.drawFastVLine(nPosX, nPosY, 3, nFColor);
			
			tft.fillRect(nPosX + 3, nPosY, 3, 3, nFColor);
			tft.drawPixel(nPosX + 4, nPosY + 1, nBGColor);
			//0
			tft.fillRect(nPosX + 7, nPosY, 3, 3, nFColor);
			tft.drawPixel(nPosX + 8, nPosY + 1, nBGColor);
			
			tft.fillRect(nPosX + 11, nPosY, 3, 3, nFColor);
			tft.drawPixel(nPosX + 12, nPosY + 1, nBGColor);
			break;
		case 1100:	
			tft.drawFastVLine(nPosX, nPosY, 3, nFColor);
			tft.drawFastVLine(nPosX + 2, nPosY, 3, nFColor);
			
			tft.fillRect(nPosX + 4, nPosY, 3, 3, nFColor);
			tft.drawPixel(nPosX + 5, nPosY + 1, nBGColor);
			//0
			tft.fillRect(nPosX + 8, nPosY, 3, 3, nFColor);
			tft.drawPixel(nPosX + 9, nPosY + 1, nBGColor);
			break;
		case 1200:
			tft.drawFastVLine(nPosX, nPosY, 3, nFColor);
			//2			
			tft.fillRect(nPosX + 2, nPosY, 3, 3, nFColor);
			tft.drawPixel(nPosX + 2, nPosY, nBGColor);
			tft.drawPixel(nPosX + 4, nPosY + 2, nBGColor);
			//0
			tft.fillRect(nPosX + 6, nPosY, 3, 3, nFColor);
			tft.drawPixel(nPosX + 7, nPosY + 1, nBGColor);
			
			tft.fillRect(nPosX + 10, nPosY, 3, 3, nFColor);
			tft.drawPixel(nPosX + 11, nPosY + 1, nBGColor);
			break;
		case -20: 
			//-
			tft.drawFastHLine(nPosX, nPosY + 1, 2, nFColor);
			//2			
			tft.fillRect(nPosX + 3, nPosY, 3, 3, nFColor);
			tft.drawPixel(nPosX + 3, nPosY, nBGColor);
			tft.drawPixel(nPosX + 5, nPosY + 2, nBGColor);
			//0
			tft.fillRect(nPosX + 7, nPosY, 3, 3, nFColor);
			tft.drawPixel(nPosX + 8, nPosY + 1, nBGColor);
			break;
		default: break;
	}
}

void UpdateClimateBarGraph(int nBarIndex, uint16_t nBarColor) {
	//for increments, need to drawline at desired point and at 1 behind it
	//to ensure it overwrites any previous white bordering
	int nHumidBarX = 0;
	int nTempBarX = 0;
	int nBaromBarX = 0;
	
	switch (nBarIndex) {
		case 0:
			if (mbTempBarComplete || (millis() - mnLastTempBar < mnBarDrawInterval)) break;
			nTempBarX = 61 + mnTempCurrentBar;
			if (mnTempCurrentBar < mnTempTargetBar) {				
				tft.drawFastVLine(nTempBarX, 113, 15, nBarColor);
				tft.drawFastVLine(nTempBarX - 1, 113, 15, nBarColor);
				++mnTempCurrentBar;
			} else if (mnTempCurrentBar > mnTempTargetBar) {
				//if current is one of the notches, use notch color
				tft.drawFastVLine(nTempBarX, 113, 15, (nTempBarX == marrScaleNotches[0] || nTempBarX == marrScaleNotches[1] || nTempBarX == marrScaleNotches[2]) ? color_HEADER : ST77XX_BLACK);
				tft.drawFastVLine(nTempBarX + 1, 113, 15, (nTempBarX + 1 == marrScaleNotches[0] || nTempBarX + 1 == marrScaleNotches[1] || nTempBarX + 1 == marrScaleNotches[2]) ? color_HEADER : ST77XX_BLACK);
				--mnTempCurrentBar;				
			} else if (mnTempTargetBar == mnTempCurrentBar) {
				tft.drawFastVLine(nTempBarX - 1, 113, 15, ST77XX_WHITE);
				tft.drawFastVLine(nTempBarX, 113, 15, ST77XX_WHITE);
				tft.drawFastVLine(nTempBarX + 1, 113, 15, ST77XX_WHITE);
				//flag bar drawing complete
				mbTempBarComplete = true;
			}
			mnLastTempBar = millis();
			break;
		case 1:
			if (mbHumidBarComplete || (millis() - mnLastHumidBar < mnBarDrawInterval)) break;
			nHumidBarX = 61 + mnHumidCurrentBar;
			if (mnHumidCurrentBar < mnHumidTargetBar) {				
				tft.drawFastVLine(nHumidBarX, 159, 15, nBarColor);
				tft.drawFastVLine(nHumidBarX - 1, 159, 15, nBarColor);
				++mnHumidCurrentBar;
			} else if (mnHumidCurrentBar > mnHumidTargetBar) {
				//if current is one of the notches, use notch color				
				tft.drawFastVLine(nHumidBarX, 159, 15, (nHumidBarX == marrScaleNotches[0] || nHumidBarX == marrScaleNotches[1] || nHumidBarX == marrScaleNotches[2]) ? color_HEADER : ST77XX_BLACK);
				tft.drawFastVLine(nHumidBarX + 1, 159, 15, (nHumidBarX + 1 == marrScaleNotches[0] || nHumidBarX + 1 == marrScaleNotches[1] || nHumidBarX + 1 == marrScaleNotches[2]) ? color_HEADER : ST77XX_BLACK);
				--mnHumidCurrentBar;
			} else if (mnHumidTargetBar == mnHumidCurrentBar) {
				tft.drawFastVLine(nHumidBarX - 1, 159, 15, ST77XX_WHITE);
				tft.drawFastVLine(nHumidBarX, 159, 15, ST77XX_WHITE);
				tft.drawFastVLine(nHumidBarX + 1, 159, 15, ST77XX_WHITE);
				//flag bar drawing complete
				mbHumidBarComplete = true;
			}
			mnLastHumidBar = millis();
			break;
		case 2:
			if (mbBaromBarComplete || (millis() - mnLastBaromBar < mnBarDrawInterval)) break;
			nBaromBarX = 61 + mnBaromCurrentBar;
			if (mnBaromCurrentBar < mnBaromTargetBar) {				
				tft.drawFastVLine(nBaromBarX, 205, 15, nBarColor);
				tft.drawFastVLine(nBaromBarX - 1, 205, 15, nBarColor);
				++mnBaromCurrentBar;
			} else if (mnBaromCurrentBar > mnBaromTargetBar) {
				//if current is one of the notches, use notch color
				tft.drawFastVLine(nBaromBarX, 205, 15, (nBaromBarX == marrScaleNotches[0] || nBaromBarX == marrScaleNotches[1] || nBaromBarX == marrScaleNotches[2]) ? color_HEADER : ST77XX_BLACK);
				tft.drawFastVLine(nBaromBarX + 1, 205, 15, (nBaromBarX + 1 == marrScaleNotches[0] || nBaromBarX + 1 == marrScaleNotches[1] || nBaromBarX + 1 == marrScaleNotches[2]) ? color_HEADER : ST77XX_BLACK);
				--mnBaromCurrentBar;
			} else if (mnBaromTargetBar == mnBaromCurrentBar) {
				tft.drawFastVLine(nBaromBarX - 1, 205, 15, ST77XX_WHITE);
				tft.drawFastVLine(nBaromBarX, 205, 15, ST77XX_WHITE);
				tft.drawFastVLine(nBaromBarX + 1, 205, 15, ST77XX_WHITE);
				//flag bar drawing complete
				mbBaromBarComplete = true;
			}
			mnLastBaromBar = millis();
			break;
		default: break;
	}
}