// Basic full-color PicoDVI test. Provides a 16-bit color video framebuffer to
// which Adafruit_GFX calls can be made. It's based on the EYESPI_Test.ino sketch.
#include <PicoDVI.h>                  // Core display & graphics library
#include <Fonts/FreeSansBold18pt7b.h> // A custom font
#include <Fonts/lcars15pt7b.h>
#include <Fonts/lcars11pt7b.h>
#include <EasyButton.h>
#include <Adafruit_NeoPixel.h>

// Here's how a 320x240 16-bit color framebuffer is declared. Double-buffering
// is not an option in 16-bit color mode, just not enough RAM; all drawing
// operations are shown as they occur. Second argument is a hardware
// configuration -- examples are written for Adafruit Feather RP2040 DVI, but
// that's easily switched out for boards like the Pimoroni Pico DV (use
// 'pimoroni_demo_hdmi_cfg') or Pico DVI Sock ('pico_sock_cfg').
DVIGFX16 tft(DVI_RES_320x240p60, adafruit_feather_dvi_cfg);

//pin 9 is free, as pin_a6 is for vbat and is otherwise known as digital 20
#define VOLT_PIN 					D7		//INPUT_POWER_PIN
#define SOUND_TRIGGER_PIN			D9

//buttons, scroller	- d2 pin supposed to be pin #2
//button on the board is connected to pin 7.  TX is pin 0, RX is pin 1 - these are normally used for serial communication

#define BUTTON_1_PIN				PIN_SERIAL1_RX
#define BUTTON_2_PIN				PIN_SERIAL1_TX
//adafruit defines physically labeled pin D2 as pin 2 in its header file, but it does not respond when set as an input by default.
//you will need to modify system_nrf52840.c file by adding
//#define CONFIG_NFCT_PINS_AS_GPIOS (1)
//YOU WILL SEE THIS CONSTANT REFERENCED IN THAT FILE, with compiler-conditional logic around it to actually free up the NFC pins, but only if that constant exists in that file
#define BUTTON_3_PIN				D12

//pin 7 is the board button
#define BUTTON_BOARD				PIN_BUTTON
//only need 1 pin for potentiometer / scroller input. both poles need to be wired to GND and 3v - order doesn't matter
#define PIN_SCROLL_INPUT			D25

#define SCAN_LED_PIN_1 			A0
#define SCAN_LED_PIN_2 			A1
#define SCAN_LED_PIN_3 			A2
#define SCAN_LED_PIN_4 			A3
#define LED_RED             PIN_LED
#define SCAN_LED_BRIGHTNESS 	(32)

//neopixel power LED. must use an unreserved pin for this.  PWR, ID, EMRG all use this pin
#define NEOPIXEL_CHAIN_DATAPIN		(10)
#define NEOPIXEL_BRIGHTNESS 		(64)
#define NEOPIXEL_LED_COUNT 			(6)
// built-in pins: D4 = blue conn LED, 8 = neopixel on board, D13 = red LED next to micro usb port
// commented out lines are pre-defined by the adafruit board firmware
#define NEOPIXEL_BOARD_LED_PIN		(8)
//system os version #. max 3 digits
#define DEVICE_VERSION			"0.94"
//theme definition: 0 = TNG, 1 = DS9, 2 = VOY
#define UX_THEME	(0)
#define ST77XX_BLACK 0x0000
#define ST77XX_RED 0xF800
#define ST77XX_WHITE 0xFFFF

#if UX_THEME == 0
	// TNG colors here
	#define color_SWOOP				0xF7B3
	#define color_MAINTEXT			0x9E7F
	//#define color_MAINTEXT			0xC69F
	#define color_LABELTEXT			0x841E
	#define color_HEADER			0xFEC8
	#define color_TITLETEXT			0xFEC8
	//196,187,145
	#define color_LABELTEXT2		0xC5D2
	//204,174,220
	#define color_LABELTEXT3		0xCD7B
	#define color_LABELTEXT4		0xEE31
	#define color_LEGEND			0x6A62
	//210,202,157
	#define color_FFT				0xDEB5
#elif UX_THEME == 1
	// ds9
	#define color_SWOOP			0xD4F0
	#define color_MAINTEXT		0xBD19
	#define color_LABELTEXT		0x7A8D
	#define color_HEADER		0xECA7
	#define color_TITLETEXT		0xC3ED
	//to-do: find fitting options for these to mesh with ds9
	#define color_LABELTEXT2	0xC5D2	
	#define color_LABELTEXT3		0xCD7B	
	#define color_LABELTEXT4		0xEE31
	#define color_LEGEND			0x6A62
	#define color_FFT				0xDEB5
#elif UX_THEME == 2
	// voy
	#define color_MAINTEXT		0x9E7F
	#define color_SWOOP			0x7C34
	#define color_LABELTEXT		0x9CDF
	#define color_HEADER		0xCB33
	#define color_TITLETEXT		0xFFAF
	//to-do: find fitting options for these to mesh with voy
	#define color_LABELTEXT2	0xC5D2
	#define color_LABELTEXT3		0xCD7B
	#define color_LABELTEXT4		0xEE31
	#define color_LEGEND			0x6A62
	#define color_FFT				0xDEB5
#endif

#define color_REDLABELTEXT		0xE000
#define color_REDDARKLABELTEXT	0x9800
#define color_REDDATATEXT		0xDEFB

//use labeltext3 for servo ltpurple
#define color_SERVOPINK			0xFE18
#define color_SERVOPURPLE		0x83B7
#define RGBto565(r,g,b) ((((r) & 0xF8) << 8) | (((g) & 0xFC) << 3) | ((b) >> 3))
Adafruit_NeoPixel ledBoard(1, NEOPIXEL_BOARD_LED_PIN, NEO_GRB + NEO_KHZ800);

//power & board color enumerator: blue = 4, green = 3, yellow = 2, orange = 1, red = 0
int mnPowerColor = 4;
bool mbCyclePowerColor = false;
bool mbCycleBoardColor = false;
int mnBoardColor = 4;
//power interval doesn't need to check more than every 30 seconds
int mnPowerLEDInterval = 30000;
int mnIDLEDInterval = 1000;
unsigned long mnLastUpdateIDLED = 0;
int mnCurrentProfileRed = 0;
int mnCurrentProfileGreen = 0;
int mnCurrentProfileBlue = 0;
String msCurrentProfileName = "";
int mnEMRGLEDInterval = 110;
int mnEMRGMinStrength = 8;
int mnEMRGMaxStrength = 212;
int mnEMRGCurrentStrength = 8;
bool mbLEDIDSet = false;
unsigned long mnLastUpdatePower = 0;
unsigned long mnLastUpdateEMRG = 0;
bool mbEMRGdirection = false;
int mnBoardLEDInterval = 5000;
unsigned long mnLastUpdateBoard = 0;

//left scanner leds
int mnLeftLEDInterval = 200;
int mnLeftLEDCurrent = 0;
unsigned long mnLastUpdateLeftLED = 0;

int mnBoardRedLEDInterval = 350;
int mnBoardBlueLEDInterval = 250;
unsigned long mnLastUpdateBoardRedLED = 0;
unsigned long mnLastUpdateBoardBlueLED = 0;
bool mbBoardRedLED = false;
bool mbBoardBlueLED = false;
//button functionality
bool mbButton1Flag = false;
bool mbButton2Flag = false;
bool mbButton3Flag = false;
bool mbButton7Flag = false;
EasyButton oButton1(BUTTON_1_PIN);
EasyButton oButton2(BUTTON_2_PIN);
EasyButton oButton3(BUTTON_3_PIN);
//board button digital pin 7 - use for thermal camera toggle
EasyButton oButton7(BUTTON_BOARD);

bool mbSleepMode = false;
bool mbTomServoActive = false;
unsigned long mnButton2Press = 0;
unsigned long mnButton3Press = 0;
int mnServoButtonWindow = 1500;
//graphs should take 3 seconds for full draw cycle-> 3000 / 80 lines
uint8_t mnServoDrawInterval = 38;
unsigned long mnServoLastDraw = 0;
uint8_t mnCurrentServoGraphPoint = 0;

bool mbHomeActive = false;
unsigned long mnLastUpdateHome = 0;
int mnHomeUpdateInterval = 1000;
bool mbColorInitialized = false;
bool mbRGBActive = false;
int mnRGBScanInterval = 5000;
int mnRGBCooldown = 0;
unsigned long mnLastRGBScan = 0;
bool mbTempInitialized = false;
bool mbHumidityInitialized = false;
bool mbTempActive = false;
bool mbThermalCameraStarted = false;
bool mbThermalActive = false;
bool mbHumidBarComplete = false;
bool mbTempBarComplete = false;
bool mbBaromBarComplete = false;
bool mbMicrophoneStarted = false;
bool mbMicrophoneActive = false;
bool mbMicrophoneRedraw = false;
int mnClimateCooldown = 0;
int mnTempTargetBar = 0;
int mnTempCurrentBar = 0;
int mnHumidTargetBar = 0;
int mnHumidCurrentBar = 0;
int mnBaromTargetBar = 0;
int mnBaromCurrentBar = 0;

//establish graph values
const uint8_t mnarrServoGraphData[] = {3,7,10,12,13,14,14,14,13,12,10,7,4,4,6,8,11,13,14,15,15,15,15,14,13,
									12,10,8,7,6,4,3,2,2,2,3,4,6,8,9,11,12,12,12,11,10,9,7,5,3,3,3,5,7,10,
									11,11,12,12,12,11,11,10,9,9,9,8,8,8,8,7,6,5,5,5,5,4,3,2,2,2};
									

void setup() {
	//NRF_UICR->NFCPINS = 0;
	ledBoard.begin();
	
	//dvi uses begin as initialization
	tft.begin();
	//tft.setRotation(1);
	tft.setFont(&lcars11pt7b);
	tft.setTextSize(1);
	//these goggles, they do nothing!
	tft.setTextWrap(false);
		
	ledBoard.clear();
	ledBoard.setBrightness(NEOPIXEL_BRIGHTNESS);
	
	//set pinmode output for scanner leds
	pinMode(SCAN_LED_PIN_1, OUTPUT);
	pinMode(SCAN_LED_PIN_2, OUTPUT);
	pinMode(SCAN_LED_PIN_3, OUTPUT);
	pinMode(SCAN_LED_PIN_4, OUTPUT);
	
	//set output for board non-neopixel LEDs
	pinMode(LED_RED, OUTPUT);
	
	pinMode(SOUND_TRIGGER_PIN, OUTPUT);
	digitalWrite(SOUND_TRIGGER_PIN, HIGH);
		
	//or it'll be read as low and boot the tricorder into environment section
	pinMode(BUTTON_1_PIN, OUTPUT);
	pinMode(BUTTON_2_PIN, OUTPUT);
	pinMode(BUTTON_3_PIN, OUTPUT);
	pinMode(BUTTON_BOARD, OUTPUT);
	digitalWrite(BUTTON_1_PIN, HIGH);
	digitalWrite(BUTTON_2_PIN, HIGH);
	digitalWrite(BUTTON_3_PIN, HIGH);
	digitalWrite(BUTTON_BOARD, HIGH);	
	delay(10);
		
	//pinMode(PIN_SCROLL_INPUT, INPUT);
	
	pinMode(BUTTON_1_PIN, INPUT);
	pinMode(BUTTON_2_PIN, INPUT);
	pinMode(BUTTON_3_PIN, INPUT);
	
	//pinMode(VOLT_PIN, INPUT);	
	pinMode(BUTTON_BOARD, INPUT);	
	
	/*oButton2.begin();
	oButton2.onPressed(ToggleRGBSensor);	
	
	oButton1.begin();
	oButton1.onPressed(ToggleClimateSensor);
	
	oButton3.begin();
	oButton3.onPressed(ToggleMicrophone);*/
	
	oButton7.begin();
	oButton7.onPressed(ToggleTomServo);
	
	GoHome();
}

void loop() {
	//toggle climate
	oButton1.read();
	//this toggles RGB scanner
	oButton2.read();	
	//toggle microphone
	oButton3.read();
	//toggle thermal camera
	oButton7.read();
			
	RunNeoPixelColor(NEOPIXEL_BOARD_LED_PIN);
	//RunLeftScanner();
	RunHome();
	//blink board LEDs
	RunBoardLEDs();	
	RunTomServo();
}

void GoHome() {
	//if flag set for thermal debug mode, hijack home screen?	
	//reset any previous sensor statuses
	mnRGBCooldown = 0;
	mnClimateCooldown = 0;
	mbHomeActive = true;
	mbRGBActive = false;
	mbTempActive = false;
	mnCurrentServoGraphPoint = 0;
	mnServoLastDraw = 0;
	//SetActiveNeoPixelButton(0);
	
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
	mbThermalActive = false;
	mbTomServoActive = false;

	//tft.setFont(&lcars11pt7b);
	tft.setFont(&lcars15pt7b);

	// large block of text
	tft.fillScreen(ST77XX_BLACK);
	// home screen header is 2 rounded rectangles, lines to cut them, 1 black rect as backing for header text
	//fillRoundRect(x,y,width,height,cornerRadius, color)
	//top and bottom borders
	tft.fillRoundRect(26, 1, 289, 22, 11, color_SWOOP);
	tft.fillRoundRect(26, 216, 289, 22, 11, color_SWOOP);
	//left side
	tft.fillRoundRect(1, 1, 68, 66, 32, color_SWOOP);
	tft.fillRoundRect(1, 171, 68, 66, 32, color_SWOOP);
	tft.fillRoundRect(59, 23, 24, 193, 11, ST77XX_BLACK);
	//middle section - needs to be split for profile name and color
	//tft.fillRect(1, 39, 58, 164, color_HEADER);
	//dull all LED colors by 50% to lcarsify them
	tft.fillRect(1, 39, 58, 76, RGBto565((int)min(mnCurrentProfileRed*1.75, 255), (int)min(mnCurrentProfileGreen*1.75, 255), (int)min(mnCurrentProfileBlue*1.75, 255)));
	tft.fillRect(1, 147, 58, 55, RGBto565((int)min(mnCurrentProfileRed*1.75, 255), (int)min(mnCurrentProfileGreen*1.75, 255), (int)min(mnCurrentProfileBlue*1.75, 255)));
	//profile label always shows bkg as swoop color 
	tft.fillRect(1, 119, 58, 24, color_SWOOP);
	
	//middle section black lines
	tft.drawFastHLine(1, 35, 58, ST77XX_BLACK);
	tft.drawFastHLine(1, 36, 58, ST77XX_BLACK);
	tft.drawFastHLine(1, 37, 58, ST77XX_BLACK);
	tft.drawFastHLine(1, 38, 58, ST77XX_BLACK);
	tft.drawFastHLine(1, 39, 58, ST77XX_BLACK);	
	tft.drawFastHLine(1, 40, 58, ST77XX_BLACK);	
	tft.drawFastHLine(1, 201, 58, ST77XX_BLACK);
	tft.drawFastHLine(1, 202, 58, ST77XX_BLACK);
	tft.drawFastHLine(1, 203, 58, ST77XX_BLACK);
	tft.drawFastHLine(1, 204, 58, ST77XX_BLACK);
	tft.drawFastHLine(1, 205, 58, ST77XX_BLACK);
	tft.drawFastHLine(1, 206, 58, ST77XX_BLACK);
	//bottom right
	tft.drawFastVLine(296, 216, 22, ST77XX_BLACK);
	tft.drawFastVLine(297, 216, 22, ST77XX_BLACK);	
	tft.drawFastVLine(298, 216, 22, ST77XX_BLACK);
	tft.drawFastVLine(299, 216, 22, ST77XX_BLACK);
	tft.drawFastVLine(300, 216, 22, ST77XX_BLACK);
	tft.drawFastVLine(301, 216, 22, ST77XX_BLACK);	

	//black section for home screen title
	tft.fillRect(232, 1, 69, 22, ST77XX_BLACK);
	drawParamText(229, 21, "  STATUS", color_MAINTEXT);
	//drawParamText(229, 21, "  ABOUT", color_MAINTEXT);
	
	tft.fillRoundRect(76, 39, 14, 11, 5, color_LABELTEXT);
	tft.fillRoundRect(76, 63, 14, 11, 5, color_LABELTEXT);
	tft.fillRoundRect(76, 87, 14, 11, 5, color_LABELTEXT);
	
	tft.fillRoundRect(162, 39, 14, 11, 5, color_MAINTEXT);
	tft.fillRoundRect(162, 63, 14, 11, 5, color_MAINTEXT);
	tft.fillRoundRect(162, 87, 14, 11, 5, color_MAINTEXT);

	tft.fillRoundRect(252, 87, 14, 11, 5, color_MAINTEXT);
	//set font to 11pt since header is done	
	tft.setFont(&lcars11pt7b);
	
	drawParamText(6, 138, msCurrentProfileName, ST77XX_BLACK);
	
	drawParamText(101, 50, "POWER", color_LABELTEXT);
	drawParamText(101, 75, "UPTIME", color_LABELTEXT);
	drawParamText(101, 100, "CPU", color_LABELTEXT);
	
	//number only for battery level
	ShowBatteryLevel(213, 50, color_LABELTEXT, color_MAINTEXT);
	//uptime
	drawParamText(185, 75, "00:00.00", color_MAINTEXT);
	//cpu speed - hz to mhz conversion is to divide by 1 million exactly
	drawParamText(204, 100, String((float) F_CPU / 1000000.0), color_MAINTEXT);
	//device version
	drawParamText(235, 100, "           " + String(DEVICE_VERSION), color_MAINTEXT);
		
	tft.fillRect(70, 119, 81, 34, color_LABELTEXT2);	
  
	//drawParamText(159, 138, "CLIMATE", ST77XX_BLACK);
	drawParamText(78, 142, "CLIMATE", ST77XX_BLACK);
				
	tft.fillRect(70, 168, 82, 34, color_LABELTEXT2);
	drawParamText(78, 191, "CHROMATICS", ST77XX_BLACK);
	//drawParamText(137, 169, "CHROMATICS", ST77XX_BLACK);
	
	tft.fillRect(192, 119, 82, 34, color_LABELTEXT2);
	//drawParamText(169, 200, "SONICS", ST77XX_BLACK);
	drawParamText(200, 142, "SONICS", ST77XX_BLACK);
		
	tft.fillRect(192, 168, 82, 34, color_LABELTEXT2);
	drawParamText(200, 191, "THERMAL", ST77XX_BLACK);
		
	//black lines to separate rectangles from caps
	tft.drawFastVLine(152, 119, 84, ST77XX_BLACK);
	tft.drawFastVLine(153, 119, 84, ST77XX_BLACK);
	tft.drawFastVLine(154, 119, 84, ST77XX_BLACK);
	tft.drawFastVLine(155, 119, 84, ST77XX_BLACK);
	tft.drawFastVLine(156, 119, 84, ST77XX_BLACK);
	tft.drawFastVLine(157, 119, 84, ST77XX_BLACK);
	tft.drawFastVLine(158, 119, 84, ST77XX_BLACK);
	tft.drawFastVLine(275, 119, 84, ST77XX_BLACK);
	tft.drawFastVLine(276, 119, 84, ST77XX_BLACK);
	tft.drawFastVLine(277, 119, 84, ST77XX_BLACK);
	tft.drawFastVLine(278, 119, 84, ST77XX_BLACK);
	tft.drawFastVLine(279, 119, 84, ST77XX_BLACK);
	tft.drawFastVLine(280, 119, 84, ST77XX_BLACK);
	tft.drawFastVLine(281, 119, 84, ST77XX_BLACK);			
}

void drawParamCharText(uint8_t nPosX, uint8_t nPosY, char *sText, uint16_t nColor) {
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

int GetBuffer(double dInput) {
	if (dInput < 10) {
		return 14;
	} else if (dInput < 100) {
		return 8;
	} 
	return 0;
}

int GetBuffer(int nInput) {
	if (nInput < 10) {
		return 14;
	} else if (nInput < 100) {
		return 8;
	} else {
		return 0;
	}
}

void RunNeoPixelColor(int nPin) {
	unsigned long lTimer = millis();
	if (nPin == NEOPIXEL_BOARD_LED_PIN) {
		//unsure if want to use, as this needs to be sensor flash.
		//this has a separate update interval from power because we want this to come back faster than 30 seconds after color scanner is used
		if (!mbRGBActive) {
			if ((lTimer - mnLastUpdateBoard) > mnBoardLEDInterval) {
				if (mbCycleBoardColor == false) {
					int nBattMapBoard = GetBatteryTier();
					//cycle order = blue, green, yellow, orange, red
					switch (nBattMapBoard) {
						case 5: ledBoard.setPixelColor(0, 0, 0, 128); break;
						case 4: ledBoard.setPixelColor(0, 0, 0, 128); break;
						case 3: ledBoard.setPixelColor(0, 0, 128, 0); break;
						case 2: ledBoard.setPixelColor(0, 112, 128, 0); break;
						case 1: ledBoard.setPixelColor(0, 128, 96, 0); break;
						default: ledBoard.setPixelColor(0, 128, 0, 0); break;
					}							
				} else {
					switch (mnBoardColor) {
						case 4: ledBoard.setPixelColor(0, 0, 0, 128); mnBoardColor = 3; break;
						case 3: ledBoard.setPixelColor(0, 0, 128, 0); mnBoardColor = 2; break;
						case 2: ledBoard.setPixelColor(0, 112, 128, 0); mnBoardColor = 1; break;
						case 1: ledBoard.setPixelColor(0, 128, 96, 0); mnBoardColor = 0; break;
						default: ledBoard.setPixelColor(0, 128, 0, 0); mnBoardColor = 4; break;
					}
				}
				mnLastUpdateBoard = lTimer;
				ledBoard.show();
			}
		} else {
			//color scanner app running - do nothing
		}
	}
}


void RunHome() {	
	if (!mbHomeActive) return;
	
	unsigned long nNowMillis = millis();
	
	//show system uptime, battery level		
	if (nNowMillis - mnLastUpdateHome > mnHomeUpdateInterval) {
		//blank last uptime display, refresh values
		int nUptimeSeconds = (nNowMillis / 1000);
		int nUptimeMinutes = (nUptimeSeconds / 60);
		int nDisplayMinutes = nUptimeMinutes;
		int nUptimeHours = nUptimeMinutes / 60;
		int nDisplaySeconds = nUptimeSeconds;
		
		//only update battery level every 5 seconds
		if ((nUptimeSeconds % 10) == 0) {
			tft.fillRect(205, 35, 30, 24, ST77XX_BLACK);
			ShowBatteryLevel(213, 50, color_LABELTEXT, color_MAINTEXT);
		}
		
		if (nUptimeHours > 0) {
			nDisplayMinutes = nUptimeMinutes - (nUptimeHours * 60);			
		}
		if (nUptimeMinutes > 0) {
			nDisplaySeconds = nUptimeSeconds - (nUptimeMinutes * 60);
		}
		
		String sUptime = (nUptimeHours > 9 ? String(nUptimeHours) : "0" + String(nUptimeHours) ) + ":" + (nDisplayMinutes > 9 ? String(nDisplayMinutes) : "0" + String(nDisplayMinutes)) + "." + (nDisplaySeconds > 9 ? String(nDisplaySeconds) : "0" + String(nDisplaySeconds));
		tft.fillRect(180, 58, 58, 25, ST77XX_BLACK);
		drawParamText(185, 75, sUptime, color_MAINTEXT);
		
		//show profile color and name in left frame border
		//tft.fillRect(1, 39, 58, 76, RGBto565((int)(mnCurrentProfileRed), (int)(mnCurrentProfileGreen), (int)(mnCurrentProfileBlue)));
		//tft.fillRect(1, 147, 58, 55, RGBto565((int)(mnCurrentProfileRed), (int)(mnCurrentProfileGreen), (int)(mnCurrentProfileBlue)));
		tft.fillRect(1, 39, 58, 76, RGBto565((int)min(mnCurrentProfileRed*1.75, 255), (int)min(mnCurrentProfileGreen*1.75, 255), (int)min(mnCurrentProfileBlue*1.75, 255)));
		tft.fillRect(1, 147, 58, 55, RGBto565((int)min(mnCurrentProfileRed*1.75, 255), (int)min(mnCurrentProfileGreen*1.75, 255), (int)min(mnCurrentProfileBlue*1.75, 255)));
		//profile label always shows bkg as swoop color
		tft.fillRect(1, 119, 58, 24, color_SWOOP);
		drawParamText(6, 138, msCurrentProfileName, ST77XX_BLACK);
				
		mnLastUpdateHome = nNowMillis;
	}
	
}

void ShowBatteryLevel(int nPosX, int nPosY, uint16_t nLabelColor, uint16_t nValueColor) {
	String sBatteryStatus = "";  
	int nBattPct = GetBatteryPercent();
	//need to use space characters to pad string because it'll wrap back around to left edge when cursor starting point set over 240.
	//this is a glitch of adafruit GFX library - it thinks display is 240x320 despite the rotation in setup() and setting for text wrap
	//sBatteryStatus = "      " + String((int)fBattV);
	sBatteryStatus = String(nBattPct);
	drawParamText(nPosX + GetBuffer(nBattPct), nPosY, sBatteryStatus, color_MAINTEXT);	
	//float sRawVolt = (analogRead(VOLT_PIN) * 7.2) / 1024;
	//drawParamText(nPosX + GetBuffer(nBattPct), nPosY, String(sRawVolt), color_MAINTEXT);
}

uint8_t GetBatteryTier() {
	//int nBattPct = map(fBattV, 320, 420, 0, 100);
	int nBattPct = GetBatteryPercent();
	//divide by 20 converts the % to a number 0-4. we have 5 total colors for conveying battery level.
	return (nBattPct / 20);	
}

uint8_t GetBatteryPercent() {
	float fBattV = analogRead(VOLT_PIN);
	//fBattV *= 2;    // we divided by 2 (board has a resistor on this pin), so multiply back,  // Multiply by 3.3V, our reference voltage
	// apparently, previous versions used 3.3 as battery reference, but now this is 3.6
	// for this reason, if you use 3.3 with a newer version of firmware, batt % will show 60 when it should be 100
	//fBattV *= 3.6; 	
	//combine previous actions for brevity- multiply by 2 to adjust for resistor, then 3.3 reference voltage
	fBattV *= 7.2;
	fBattV /= 1024; // convert to voltage
	//3.2 to 4.2 => subtract 3.2 (0) from current voltage. now values should be in range of 0-0.9 : multiply by 1.111, then by 100, convert result to int.
	//fBattV -= 3.2;	
	//convert voltage to a percentage
	fBattV = fBattV * 100;
	fBattV = constrain(fBattV, 320, 420);
	int nBattPct = map(fBattV, 320, 420, 0, 100);
	return nBattPct;
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

void RunBoardLEDs() {
	unsigned long lTimer = millis();
	
	if ((lTimer - mnLastUpdateBoardRedLED) > mnBoardRedLEDInterval) {
		//flip LED state flag, set state based on new flag
		mbBoardRedLED = !mbBoardRedLED;
		if (mbBoardRedLED) {
			digitalWrite(LED_RED, HIGH);
		} else {
			digitalWrite(LED_RED, LOW);
		}
		mnLastUpdateBoardRedLED = lTimer;
	}
}


void ToggleTomServo() {
	mbTomServoActive = false;
	mbButton7Flag = !mbButton7Flag;
	
	if (mbButton7Flag) {
    tft.fillScreen(ST77XX_BLACK);
    mbHomeActive = false;
    mbRGBActive = false;
    mbTomServoActive = true;
    mbMicrophoneActive = false;
    mbTempActive = false;		
    mnCurrentServoGraphPoint = 0;
    mnServoLastDraw = 0;

    mnRGBCooldown = 0;
    mnClimateCooldown = 0;
    mbHomeActive = false;
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
    mbThermalActive = false;

    tft.fillScreen(ST77XX_BLACK);

    //58x237, 21 round
    tft.fillRoundRect(1, 3, 70, 237, 21, color_HEADER);
    tft.fillRoundRect(51, 11, 21, 237, 8, ST77XX_BLACK);
    //58, 4
    tft.fillRect(1, 62, 50, 56, color_MAINTEXT);
    tft.fillRect(1, 58, 51, 4, ST77XX_BLACK);
    tft.fillRect(1, 118, 51, 4, ST77XX_BLACK);
    tft.fillRect(1, 179, 51, 4, ST77XX_BLACK);
    tft.fillRect(26, 3, 55, 8, color_HEADER);
    tft.fillRect(89, 3, 149, 8, color_HEADER);
    tft.fillRect(246, 3, 74, 8, color_HEADER);
    tft.fillRect(1, 200, 25, 40, color_HEADER);

    //32x13 med purple
    tft.fillRect(67, 62, 32, 13, color_SERVOPURPLE);
    tft.fillRect(67, 122, 32, 13, color_SERVOPURPLE);
    tft.fillRect(67, 183, 32, 13, color_SERVOPURPLE);
    tft.fillRect(186, 62, 32, 13, color_SERVOPURPLE);
    tft.fillRect(186, 122, 32, 13, color_SERVOPURPLE);

    //102, 80*13 lt purple left	
    tft.fillRect(102, 62, 80, 13, color_LABELTEXT3);
    tft.fillRect(102, 122, 80, 13, color_LABELTEXT3);
    tft.fillRect(102, 183, 80, 13, color_LABELTEXT3);
    //221, 95*13 lt purple right
    tft.fillRect(221, 62, 95, 13, color_LABELTEXT3);
    tft.fillRect(221, 122, 95, 13, color_LABELTEXT3);

    tft.fillRect(67, 79, 32, 39, color_SERVOPINK);
    tft.fillRect(67, 139, 32, 39, color_SERVOPINK);
    tft.fillRect(67, 200, 32, 40, color_SERVOPINK);
    tft.fillRect(186, 79, 32, 39, color_SERVOPINK);
    tft.fillRect(186, 139, 32, 101, color_SERVOPINK);

    //use big text size for [BIO   SCAN] and SERVO
    tft.setFont(&lcars15pt7b);
    drawParamText(220, 43, "[BIO   SCAN]", ST77XX_WHITE);

    tft.setFont(&lcars11pt7b);
    drawParamText(9, 204, "SERVO", ST77XX_BLACK);

    drawParamText(4, 79, "CAMBOT", ST77XX_BLACK);
    drawParamText(4, 96, "GYPSY", ST77XX_BLACK);
    drawParamText(4, 114, "CROOOW", ST77XX_BLACK);
    //repeat to yourself it's just a show
    drawParamText(70, 35, "REPEAT TO YOURSELF", color_LABELTEXT);
    drawParamText(70, 54, "IT'S JUST A SHOW", color_LABELTEXT);

    drawTinyInt(86, 70, 800, ST77XX_BLACK, color_SERVOPURPLE);
    drawTinyInt(86, 130, 900, ST77XX_BLACK, color_SERVOPURPLE);
    drawTinyInt(83, 190, 1000, ST77XX_BLACK, color_SERVOPURPLE);
    drawTinyInt(205, 70, 1100, ST77XX_BLACK, color_SERVOPURPLE);
    drawTinyInt(203, 130, 1200, ST77XX_BLACK, color_SERVOPURPLE);

    drawTinyInt(127, 6, 1200, ST77XX_BLACK, color_HEADER);
    drawTinyInt(150, 6, 1000, ST77XX_BLACK, color_HEADER);

    //actual robot drawing
    tft.fillCircle(273, 154, 11, ST77XX_WHITE);	
    tft.fillCircle(249, 206, 4, ST77XX_WHITE);	
    tft.fillCircle(298, 206, 4, ST77XX_WHITE);
    tft.fillRect(250, 178, 46, 13, ST77XX_WHITE);
    tft.fillRect(268, 165, 10, 53, ST77XX_RED);
    tft.fillRect(268, 141, 10, 4, ST77XX_RED);
    tft.fillRect(262, 178, 6, 40, ST77XX_RED);
    tft.fillRect(278, 178, 6, 40, ST77XX_RED);
    tft.drawRect(270, 192, 6, 15, ST77XX_WHITE);
    //mouth
    tft.fillRect(271, 169, 4, 4, ST77XX_WHITE);	
    //arms
    tft.drawLine(250, 191, 248, 202, ST77XX_WHITE);
    tft.drawLine(251, 203, 258, 190, ST77XX_WHITE);
    tft.drawLine(286, 190, 296, 203, ST77XX_WHITE);
    tft.drawLine(295, 190, 299, 203, ST77XX_WHITE);
    //base
    tft.fillRect(260, 218, 26, 20, ST77XX_WHITE);
    //triangles require 3 pairs of x,y then color
    tft.fillTriangle(259, 218, 259, 237, 254, 237, ST77XX_WHITE);
    tft.fillTriangle(286, 218, 286, 237, 291, 237, ST77XX_WHITE);

    //baseline for each graph
    tft.drawFastHLine(102, 98, 80, color_LABELTEXT);
    tft.drawFastHLine(102, 158, 80, color_LABELTEXT2);
    tft.drawFastHLine(102, 219, 80, color_LABELTEXT3);
    //right side graph is 94 wide
    tft.drawFastHLine(221, 98, 94, color_LABELTEXT4);

    mnCurrentServoGraphPoint = 0;
    mnServoLastDraw = 0;
	} else {		
		mbTomServoActive = false;
		GoHome();
	}
}

void RunTomServo() {
	if (!mbTomServoActive) return;	
	
	//use main arduino loop for ALL graph drawings, with delay enforcer (3000ms / 80) as throttle to allow button polling
	//3000ms is used because we want these bars to take 3 seconds to cycle through before starting over
	if (millis() - mnServoLastDraw < mnServoDrawInterval) return;
		
	const uint8_t nYBase1 = 98;	
	const uint8_t nYBase2 = 158;
	const uint8_t nYBase3 = 219;
	const uint8_t nYBase4 = 98;
	const uint8_t nXBaseL = 102;
	const uint8_t nXBaseR = 221;
	uint8_t nPreviousServoGraphPoint = 0;
	uint8_t nPreviousServoGraphPoint2 = 0;
	uint8_t nPreviousServoGraphPoint4 = 0;
	uint8_t nPreviousServoGraphPoint8 = 0;
	nPreviousServoGraphPoint = mnCurrentServoGraphPoint == 0 ? 79 : (mnCurrentServoGraphPoint - 1);
	nPreviousServoGraphPoint2 = (mnCurrentServoGraphPoint - 2) < 0 ? 78 : (mnCurrentServoGraphPoint - 2);
	nPreviousServoGraphPoint4 = (mnCurrentServoGraphPoint - 4) < 0 ? 76 : (mnCurrentServoGraphPoint - 4);
	nPreviousServoGraphPoint8 = (mnCurrentServoGraphPoint - 8) < 0 ? 72 : (mnCurrentServoGraphPoint - 8);
	//colors in use: 1 => labeltext, 2 => labeltext2, 3 => labeltext3, 4 => labeltext4
	
	//animate 4 trash graphs	
	//left 80x39, 1 wide > 4 wide > 8 wide
	//right 94x39, bars 2 wide
	//cycle "white" bar from left to right. at last column, start over at 0	
	//each graph update is in its own if-statement, but many of the graph sizes have to handle shared situations
	//every even graph point needs to redraw single-line and 2-line graph, and potentially cause an update for 4-line and 8-line as well
	
	//redraw previous line as graph color if this is not first draw action
	if (mnServoLastDraw > 0) {
		tft.drawFastVLine((nXBaseL + nPreviousServoGraphPoint), (nYBase1 - mnarrServoGraphData[nPreviousServoGraphPoint]), (mnarrServoGraphData[nPreviousServoGraphPoint] * 2), color_LABELTEXT); 
	}
		
	//draw current single line graph data point
	tft.drawFastVLine((nXBaseL + mnCurrentServoGraphPoint), (nYBase1 - mnarrServoGraphData[mnCurrentServoGraphPoint]), (mnarrServoGraphData[mnCurrentServoGraphPoint] * 2), ST77XX_WHITE); 
	
	//graph 2, 3, 4 all use same data at different speeds
	//graph on right side - #4
	if (mnCurrentServoGraphPoint % 2 == 0) {
		if (mnServoLastDraw > 0) {
			tft.fillRect(nXBaseR + nPreviousServoGraphPoint2, (nYBase4 - mnarrServoGraphData[nPreviousServoGraphPoint2]), 2, (mnarrServoGraphData[nPreviousServoGraphPoint2] * 2), color_LABELTEXT4);
		}
		tft.fillRect(nXBaseR + mnCurrentServoGraphPoint, (nYBase4 - mnarrServoGraphData[mnCurrentServoGraphPoint]), 2, (mnarrServoGraphData[mnCurrentServoGraphPoint] * 2), ST77XX_WHITE);
		
		//graph on middle left - #2
		if (mnCurrentServoGraphPoint % 4 == 0) {
			if (mnServoLastDraw > 0) {
				tft.fillRect(nXBaseL + nPreviousServoGraphPoint4, (nYBase2 - mnarrServoGraphData[nPreviousServoGraphPoint4]), 4, (mnarrServoGraphData[nPreviousServoGraphPoint4] * 2), color_LABELTEXT2);
			}
			tft.fillRect(nXBaseL + mnCurrentServoGraphPoint, (nYBase2 - mnarrServoGraphData[mnCurrentServoGraphPoint]), 4, (mnarrServoGraphData[mnCurrentServoGraphPoint] * 2), ST77XX_WHITE);
		
			//graph on bottom left - #3
			if (mnCurrentServoGraphPoint % 8 == 0) {
				if (mnServoLastDraw > 0) {
					tft.fillRect(nXBaseL + nPreviousServoGraphPoint8, (nYBase3 - mnarrServoGraphData[nPreviousServoGraphPoint8]), 8, (mnarrServoGraphData[nPreviousServoGraphPoint8] * 2), color_LABELTEXT3);
				}
				tft.fillRect(nXBaseL + mnCurrentServoGraphPoint, (nYBase3 - mnarrServoGraphData[mnCurrentServoGraphPoint]), 8, (mnarrServoGraphData[mnCurrentServoGraphPoint] * 2), ST77XX_WHITE);		
			}
		}
	}	
	
	++mnCurrentServoGraphPoint;
	if (mnCurrentServoGraphPoint > 79) mnCurrentServoGraphPoint = 0;
	mnServoLastDraw = millis();	
}
