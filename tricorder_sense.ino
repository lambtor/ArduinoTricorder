#include <Adafruit_NeoPixel.h>
#include <SPI.h>
#include <Adafruit_GFX.h>    	// Core graphics library
#include <Adafruit_ST7789.h> 	// Hardware-specific library for ST7789
#include <string.h>
#include <Adafruit_APDS9960.h>	//RGB, light, proximity, gesture sensor
#include <Wire.h>
#include <EasyButton.h>
#include <Adafruit_BMP280.h>	//altitude, barometer
#include <Adafruit_SHT31.h>		//temp, humidity
#include <Adafruit_Sensor.h>	//wtf is this??
//#include <Adafruit_ZeroPDM.h>
#include <PDM.h>
#include <Adafruit_ZeroFFT.h>
//#include "tricorderSound.C"

//these aren't migrated to nrf52 chip architecture
//#include <arduinoFFT.h>
//#include <ArduinoSound.h>
//#include <AmplitudeAnalyzer.h>

//to-do: have device broadcast for bluetooth connectivity, allow selection of ID led color and preferred "THEME"
//between ds9, voyager, tng

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
#define VOLT_PIN 			PIN_A6
#define SLEEP_PIN			13
#define REED_PIN			12

//buttons, scroller	- d2 pin actually pin #2?
//we can't use pin 13 for buttons, as that is connected directly to a red LED on the board.
//button on the board is connected to pin 7.  TX is pin 0, RX is pin 1 - these are normally used for serial communication
//you can't use 13 as an input, but it can be an output for maybe a single LED?
//#define BUTTON_1_PIN		PIN_A4
//#define BUTTON_2_PIN		PIN_A5
//#define BUTTON_3_PIN		(0)

//#define BUTTON_1_PIN		(0)	//PIN_AREF
#define BUTTON_1_PIN		PIN_AREF
#define BUTTON_2_PIN		PIN_A4
#define BUTTON_3_PIN		PIN_A5
//#define BUTTON_4_PIN		PIN_NFC2
#define BUTTON_SLEEP_PIN	12
#define BUTTON_BOARD		7

// A0 is pin14. can't use that as an output pin?		A0 = 14, A3 = 17
#define SCAN_LED_PIN_1 	PIN_A0	//14
#define SCAN_LED_PIN_2 	PIN_A1	//15
#define SCAN_LED_PIN_3 	PIN_A2	//16
#define SCAN_LED_PIN_4 	PIN_A3	//17

#define SCAN_LED_BRIGHTNESS 	32

// power LED. must use an unreserved pin for this.
// cdn-learn.adafruit.com/assets/assets/000/046/243/large1024/adafruit_products_Feather_M0_Adalogger_v2.2-1.png?1504885273
#define POWER_LED_PIN 			6
#define NEOPIXEL_BRIGHTNESS 	64
#define NEOPIXEL_LED_COUNT 		3
// built-in pins: D4 = blue conn LED, 8 = neopixel on board, D13 = red LED next to micro usb port
#define NEOPIXEL_BOARD_LED_PIN	8
#define BOARD_REDLED_PIN 		13
#define BOARD_BLUELED_PIN 		4
//#define PIN_SERIAL1_RX       (1)
//#define PIN_SERIAL1_TX       (0)

//system os version #. max 3 digits
#define DEVICE_VERSION			"0.84"

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

// ds9
//#define color_SWOOP			0xD4F0
//#define color_MAINTEXT		0xBD19
//#define color_LABELTEXT		0x7A8D
//#define color_HEADER			0xECA7
//#define color_TITLETEXT		0xC3ED
//#define color_LABELTEXT2		0xC5D2
// voy
//#define color_MAINTEXT		0x9E7F
//#define color_SWOOP			0x7C34
//#define color_LABELTEXT		0x9CDF
//#define color_HEADER			0xCB33
//#define color_TITLETEXT		0xFFAF
//#define color_LABELTEXT2		0xC5D2

#define color_REDLABELTEXT		0xE000
#define color_REDDARKLABELTEXT	0x9800
#define color_REDDATATEXT		0xDEFB

#define RGBto565(r,g,b) ((((r) & 0xF8) << 8) | (((g) & 0xFC) << 3) | ((b) >> 3))

Adafruit_NeoPixel ledPwrStrip(NEOPIXEL_LED_COUNT, POWER_LED_PIN, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel ledBoard(1, NEOPIXEL_BOARD_LED_PIN, NEO_GRB + NEO_KHZ800);

// do not fuck with this. 2.0 IS THE BOARD - this call uses hardware SPI
Adafruit_ST7789 tft = Adafruit_ST7789(TFT_CS, TFT_DC, TFT_RST);
//create sensor objects
Adafruit_APDS9960 oColorSensor;
Adafruit_BMP280 oTempBarom;
Adafruit_SHT31 oHumid;

//power & board color enumerator: blue = 4, green = 3, yellow = 2, orange = 1, red = 0
int mnPowerColor = 4;
bool mbCyclePowerColor = false;
bool mbCycleBoardColor = false;
int mnBoardColor = 4;
//power interval doesn't need to check more than every 30 seconds
int mnPowerLEDInterval = 30000;
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

bool mbHomeActive = false;
unsigned long mnLastUpdateHome = 0;
int mnHomeUpdateInterval = 1000;
bool mbMicrophoneStarted = false;
bool mbMicrophoneActive = false;
bool mbMicrophoneRedraw = false;

extern PDMClass PDM;

//mic sample rate is hertz, all samples are stored as 16 bit data.
//number of samples must be double the desired resolution, and be a base 2 number
//16k hertz -> 0-8k hertz max audio range captured
#define MIC_SAMPLESIZE 		256
//16k is the ONLY SUPPORTED SAMPLE RATE!
//#define MIC_SAMPLERATE		16000
#define MIC_AMPLITUDE		1000          // Depending on audio source level, you may need to alter this value. Can be used as a 'sensitivity' control.
#define DECIMATION			64
#define FFT_REFERENCELINES	16
//#define FFT_MAX				150
#define FFT_BINCOUNT        16
#define FFT_BARHEIGHTMAX	64

#define GRAPH_OFFSET 10
#define GRAPH_WIDTH (tft.width() - 3)
#define GRAPH_HEIGHT (tft.height() - GRAPH_OFFSET)
#define GRAPH_MIN (tft.height() - 2)
#define GRAPH_MAX (tft.height() - GRAPH_OFFSET)

long MIC_SAMPLERATE	= 16000;
int32_t mnMicVal = 0;
short mnarrSampleData[MIC_SAMPLESIZE];
// number of samples read
volatile int mnSamplesRead = 0;
double mdarrActual[MIC_SAMPLESIZE];
double mdarrImaginary[MIC_SAMPLESIZE];
int mnMaxDBValue = 0;
//unsigned long mnLastDBRead

// a windowed sinc filter for 44 khz, 64 samples
//uint16_t marrSincFilter[DECIMATION] = {0, 2, 9, 21, 39, 63, 94, 132, 179, 236, 302, 379, 467, 565, 674, 792, 920, 1055, 1196, 1341, 1487, 1633, 1776, 1913, 2042, 2159, 2263, 2352, 2422, 2474, 2506, 2516, 2506, 2474, 2422, 2352, 2263, 2159, 2042, 1913, 1776, 1633, 1487, 1341, 1196, 1055, 920, 792, 674, 565, 467, 379, 302, 236, 179, 132, 94, 63, 39, 21, 9, 2, 0, 0};

unsigned long mnLastMicRead = 0;
//decibel estimation every 3 seconds
int mnMicReadInterval = 3000;
short mCurrentMicDisplay[FFT_BINCOUNT] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
short mTargetMicDisplay[FFT_BINCOUNT] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};

//button functionality
bool mbButton1Flag = false;
bool mbButton2Flag = false;
bool mbButton3Flag = false;
bool mbButton5Flag = false;
EasyButton oButton1(BUTTON_1_PIN);
EasyButton oButton2(BUTTON_2_PIN);
EasyButton oButton3(BUTTON_3_PIN);
//reed switch sleep mode - may be better to wire this directly from display backlight pin to ground
EasyButton oButton4(REED_PIN);
bool mbButton4Flag = false;
//board button digital pin 7 - use for "pause scanner"?
//EasyButton oButton5(BUTTON_BOARD);
//bool mbButton5Toggled = false;


void setup() {
	ledPwrStrip.begin();
	ledBoard.begin();
	
	// OR use this initializer (uncomment) if using a 2.0" 320x240 TFT. technically this is a rotated 240x320, so declaration is in that order
	tft.init(240, 320, SPI_MODE0); // Init ST7789 320x240
	tft.setRotation(1);
	tft.setFont(&lcars11pt7b);
	//these goggles, they do nothing!
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
	
	//set output for board non-neopixel LEDs
	pinMode(BOARD_REDLED_PIN, OUTPUT);
	pinMode(BOARD_BLUELED_PIN, OUTPUT);
	
	//this will be used to "turn off" the display via reed switch when the door is closed - this is just pulling backlight to ground.
	pinMode(SLEEP_PIN, OUTPUT);
	//this will also cause the red LED on the board to ALWAYS be on, so you know if sleep is activated while door closed.
	//digitalWrite(SLEEP_PIN, HIGH);
	
	//AREF pin is an analog reference and set low by default. need to write high to this to initialize it
	//or it'll be read as low and boot the tricorder into environment section
	pinMode(BUTTON_1_PIN, OUTPUT);
	digitalWrite(BUTTON_1_PIN, HIGH);
	delay(10);
	
	pinMode(BUTTON_1_PIN, INPUT);
	pinMode(BUTTON_2_PIN, INPUT);
	pinMode(BUTTON_3_PIN, INPUT);
	//this is required so the reed switch can be read.  
	//by default, pin 12 is low, and we need this high so the switch can be pulled low when triggered.
	pinMode(REED_PIN, OUTPUT);
	digitalWrite(REED_PIN, HIGH);
	delay(10);
	pinMode(REED_PIN, INPUT);	
	pinMode(VOLT_PIN, INPUT);	
	
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
	
	//microphone
	//PDM.setBufferSize(MIC_SAMPLESIZE * 2);
	PDM.onReceive(PullMicData);
	//PDM.setGain(50);
	mbMicrophoneStarted = PDM.begin(1, MIC_SAMPLERATE);
	PDM.end();
	//PDM.onReceive(GetMicrophoneData);
	
	oButton2.begin();
	oButton2.onPressed(ToggleRGBSensor);	
	
	oButton1.begin();
	oButton1.onPressed(ToggleClimateSensor);
	
	oButton3.begin();
	oButton3.onPressed(ToggleMicrophone);
	
	oButton4.begin();
	oButton4.onPressedFor(500, SleepMode);
	
	//for (int oImag = 0; oImag < MICROPHONE_SAMPLES; oImag++) {
	//	mdarrImaginary[oImag] = 0;
	//}
	
	GoHome();
}

void loop() {
	//this toggles RGB scanner
	oButton2.read();
	oButton1.read();
	oButton3.read();
	oButton4.read();
	
	//there's gotta be a cleaner way to do this?
	if (oButton4.isReleased() && mbButton4Flag) {
		ActiveMode();
	}
	
	RunNeoPixelColor(POWER_LED_PIN);
	RunNeoPixelColor(NEOPIXEL_BOARD_LED_PIN);
	RunLeftScanner();
	RunHome();
	//blink board LEDs
	RunBoardLEDs();
	
	RunRGBSensor();
	RunClimateSensor();
	RunMicrophone();	
}

void SleepMode() {
	mbButton4Flag = true;
	//to-do: turn all lights OFF, turn off screen, reset all "status" variables
	//if (mbButton4Toggled) {
		tft.fillScreen(ST77XX_BLACK);		
		tft.enableSleep(true);
		//need wired pin to set backlight low here
		//void sleep(void) { tft.sendCommand(ST77XX_SLPIN); }
		//void wake(void) { tft.sendCommand(ST77XX_SLPOUT); }
	//} 	
}

void ActiveMode() {	
	mbButton4Flag = false;
	tft.enableSleep(false);
	GoHome();
}

void RunNeoPixelColor(int nPin) {
	unsigned long lTimer = millis();
	if (nPin == POWER_LED_PIN) {
		if (mnLastUpdatePower == 0 || ((lTimer - mnLastUpdatePower) > mnPowerLEDInterval)) {
			//these will need to have their own intervals
			//switch should eventually be changed to poll voltage pin - pin#, r,g,b
			//cycle order = blue, green, yellow, orange, red
			if (mbCyclePowerColor) {			
				switch (mnPowerColor) {
					case 4: ledPwrStrip.setPixelColor(0, 0, 0, 128); mnPowerColor = 3; break;
					case 3: ledPwrStrip.setPixelColor(0, 0, 128, 0); mnPowerColor = 2; break;
					case 2: ledPwrStrip.setPixelColor(0, 112, 128, 0); mnPowerColor = 1; break;
					case 1: ledPwrStrip.setPixelColor(0, 128, 96, 0); mnPowerColor = 0; break;
					default: ledPwrStrip.setPixelColor(0, 128, 0, 0); mnPowerColor = 4; break;
				}
			} else {
				float fBattV = analogRead(VOLT_PIN);
				int nBattMap = map(fBattV, 0, 600, 0, 5);
				switch (nBattMap) {
					case 4: ledPwrStrip.setPixelColor(0, 0, 0, 128); break;
					case 3: ledPwrStrip.setPixelColor(0, 0, 128, 0); break;
					case 2: ledPwrStrip.setPixelColor(0, 112, 128, 0); break;
					case 1: ledPwrStrip.setPixelColor(0, 128, 96, 0); break;
					default: ledPwrStrip.setPixelColor(0, 128, 0, 0); break;
				}
								
			}
			if (!mbLEDIDSet) {
				ledPwrStrip.setPixelColor(1, 90, 140, 0);
				mbLEDIDSet = true;
				//ledPwrStrip.setPixelColor(2, 128, 0, 0);
			}
						
			mnLastUpdatePower = lTimer;
			//ledPwrStrip.show();
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
		if (!mbRGBActive) {
			if ((lTimer - mnLastUpdateBoard) > mnBoardLEDInterval) {
				if (mbCycleBoardColor == false) {
					float fBattVBoard = analogRead(VOLT_PIN);
					int nBattMapBoard = map(fBattVBoard, 0, 600, 0, 5);
					//cycle order = blue, green, yellow, orange, red
					switch (nBattMapBoard) {
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

void RunBoardLEDs() {
	unsigned long lTimer = millis();
	
	if ((lTimer - mnLastUpdateBoardRedLED) > mnBoardRedLEDInterval) {
		//flip LED state flag, set state based on new flag
		mbBoardRedLED = !mbBoardRedLED;
		if (mbBoardRedLED) {
			digitalWrite(BOARD_REDLED_PIN, HIGH);
		} else {
			digitalWrite(BOARD_REDLED_PIN, LOW);
		}
		mnLastUpdateBoardRedLED = lTimer;
	}
	if ((lTimer - mnLastUpdateBoardBlueLED) > mnBoardBlueLEDInterval) {
		mbBoardBlueLED = !mbBoardBlueLED;
		if (mbBoardRedLED) {
			digitalWrite(BOARD_BLUELED_PIN, HIGH);
		} else {
			digitalWrite(BOARD_BLUELED_PIN, LOW);
		}
		mnLastUpdateBoardBlueLED = lTimer;
	}	
}

void ActivateFlash() {
	//all neopixel objects are chains, so have to call it by addr
	ledBoard.setPixelColor(0, 255, 255, 255);
	ledBoard.show();
}

//to-do: add parameters to change cycle behavior of LEDs.
//ex: cycled down-> up, stacking, unified blink, KITT ping pong, etc
void RunLeftScanner() {
	unsigned long lTimer = millis();
	
	//to-do: when color scanner running, use left side lights to convey scan coming soon:
	
	if ((lTimer - mnLastUpdateLeftLED) > mnLeftLEDInterval) {		
		//add switch or if/else for different scanner modes
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
	mbHomeActive = true;
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
	//top and bottom borders
	tft.fillRoundRect(26, 1, 289, 22, 11, color_SWOOP);
	tft.fillRoundRect(26, 216, 289, 22, 11, color_SWOOP);
	//left side
	tft.fillRoundRect(1, 1, 68, 66, 32, color_SWOOP);
	tft.fillRoundRect(1, 171, 68, 66, 32, color_SWOOP);
	tft.fillRoundRect(59, 23, 24, 193, 11, ST77XX_BLACK);
	//middle section
	tft.fillRect(1, 39, 58, 164, color_HEADER);
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
	
	tft.fillRoundRect(76, 39, 14, 11, 5, color_LABELTEXT);
	tft.fillRoundRect(76, 63, 14, 11, 5, color_LABELTEXT);
	tft.fillRoundRect(76, 87, 14, 11, 5, color_LABELTEXT);
	
	tft.fillRoundRect(162, 39, 14, 11, 5, color_MAINTEXT);
	tft.fillRoundRect(162, 63, 14, 11, 5, color_MAINTEXT);
	tft.fillRoundRect(162, 87, 14, 11, 5, color_MAINTEXT);

	tft.fillRoundRect(252, 87, 14, 11, 5, color_MAINTEXT);
	//set font to 11pt since header is done	
	tft.setFont(&lcars11pt7b);
	
	drawParamText(101, 50, "POWER", color_LABELTEXT);
	drawParamText(101, 75, "UPTIME", color_LABELTEXT);
	drawParamText(101, 100, "CPU", color_LABELTEXT);
	
	//number only for battery level
	ShowBatteryLevel(213, 50, color_LABELTEXT, color_MAINTEXT);
	//uptime
	drawParamText(185, 75, "00:00.00", color_MAINTEXT);
	//cpu speed	
	drawParamText(204, 100, String((float) F_CPU / 1023000.0), color_MAINTEXT);
	//device version
	drawParamText(235, 100, "           " + String(DEVICE_VERSION), color_MAINTEXT);
	
	//color_LABELTEXT2 brown, labeltext3 pinkish, labeltext4 is tan/orange
	//show sensor statuses		
	//environment = temperature, humidity sensors
	if (mbTempInitialized) {
		tft.fillRect(76, 119, 139, 24, color_LABELTEXT4);
		//actual status tab
		tft.fillRoundRect(279, 119, 24, 24, 11, color_LABELTEXT4);
		tft.fillRect(230, 119, 65, 24, color_LABELTEXT4);
		drawParamText(239, 138, mbHumidityInitialized ? "ONLINE" : "PARTIAL", ST77XX_BLACK);
	} else if (mbHumidityInitialized) {
		tft.fillRect(76, 119, 139, 24, color_LABELTEXT3);
		//actual status tab
		tft.fillRoundRect(279, 150, 24, 24, 11, color_LABELTEXT3);
		tft.fillRect(230, 119, 65, 24, color_LABELTEXT3);
		drawParamText(239, 138, "PARTIAL", ST77XX_BLACK);
	} else {
		tft.fillRect(76, 119, 139, 24, color_LABELTEXT2);
		//drawParamText(95, 84, "OFFLINE", color_REDDATATEXT);
	}
	drawParamText(159, 138, "CLIMATE", ST77XX_BLACK);
	
	if (mbColorInitialized) {
		tft.fillRect(76, 150, 139, 24, color_LABELTEXT4);		
		tft.fillRoundRect(279, 150, 24, 24, 11, color_LABELTEXT4);
		tft.fillRect(230, 150, 65, 24, color_LABELTEXT4);
		
		drawParamText(239, 169, "ONLINE", ST77XX_BLACK);
	} else {		
		tft.fillRect(76, 150, 139, 24, color_LABELTEXT2);
	}
	drawParamText(137, 169, "CHROMATICS", ST77XX_BLACK);
	
	if (mbMicrophoneStarted) {
		tft.fillRect(76, 181, 139, 24, color_LABELTEXT4);		
		tft.fillRoundRect(279, 181, 24, 24, 11, color_LABELTEXT4);
		tft.fillRect(230, 181, 65, 24, color_LABELTEXT4);
		drawParamText(239, 200, "ONLINE", ST77XX_BLACK);
	} else {
		tft.fillRect(76, 181, 139, 24, color_LABELTEXT2);
	}
	drawParamText(174, 200, "AUDIO", ST77XX_BLACK);
		
}

void RunHome() {	
	if (!mbHomeActive) return;
	
	//show system uptime, battery level		
	if (millis() - mnLastUpdateHome > mnHomeUpdateInterval) {
		//blank last uptime display, refresh values
		int nUptimeSeconds = (millis() / 1000);
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
		
		String sUptime =  (nUptimeHours > 9 ? String(nUptimeHours) : "0" + String(nUptimeHours) ) + ":" + (nDisplayMinutes > 9 ? String(nDisplayMinutes) : "0" + String(nDisplayMinutes)) + "." + (nDisplaySeconds > 9 ? String(nDisplaySeconds) : "0" + String(nDisplaySeconds));
		tft.fillRect(180, 58, 58, 25, ST77XX_BLACK);
		drawParamText(185, 75, sUptime, color_MAINTEXT);
		mnLastUpdateHome = millis();
	}
}

void ToggleRGBSensor() {
	mbButton2Flag = !mbButton2Flag;
	//reset any temperature app values
	mbTempActive = false;
	mbHomeActive = false;
	mbMicrophoneActive = false;
	mnTempTargetBar = 0;
	mnTempCurrentBar = 0;
	mnHumidTargetBar = 0;
	mnHumidCurrentBar = 0;
	mnBaromTargetBar = 0;
	mnBaromCurrentBar = 0;
	mbHumidBarComplete = false;
	mbTempBarComplete = false;
	mbBaromBarComplete = false;
	//to-do: reset any microphone sensor values
	
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
			drawParamText(151, 21, "CHROMATIC ANALYSIS", color_TITLETEXT);
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
		//if board led lit, turn it off
		if (ledBoard.getPixelColor(0) > 0) {
			ledBoard.clear();
			ledBoard.show();
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
	sBatteryStatus = String(nBattPct);
	drawParamText(nPosX + GetBuffer(nBattPct), nPosY, const_cast<char*>(sBatteryStatus.c_str()), color_MAINTEXT);
	
	//drawParamText(nPosX, nPosY, "POWER", color_LABELTEXT);
}

void ToggleClimateSensor() {
	mbButton1Flag = !mbButton1Flag;
	//reset any rgb sensor values
	mnRGBCooldown = 0;
	mnClimateCooldown = 0;	
	mbRGBActive = false;
	//to-do: reset any microphone sensor values
	mbMicrophoneActive = false;
	mbHomeActive = false;
	
	if (mbButton1Flag) {
		if (!mbTempActive) {
			mbTempActive = true;
			//load temp scanner screen - this is done once to improve perf
			tft.fillScreen(ST77XX_BLACK);
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

void ToggleMicrophone() {
	mbButton3Flag = !mbButton3Flag;
	//reset any rgb sensor values
	mnRGBCooldown = 0;
	mnClimateCooldown = 0;	
	mbMicrophoneActive = false;
	//reset any temperature app values
	mbHomeActive = false;
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
	
	if (mbButton3Flag) {
		//show audio screen
		//tft.fillScreen(0x6B6D);
		tft.fillScreen(ST77XX_BLACK);
		//fillRoundRect(x,y,width,height,cornerRadius, color)
		tft.fillRoundRect(0, -25, 85, 91, 25, color_SWOOP);
		tft.fillRoundRect(0, 71, 85, 194, 25, color_SWOOP);
		tft.drawFastHLine(24, 63, 296, color_SWOOP);
		tft.drawFastHLine(24, 64, 296, color_SWOOP);
		tft.drawFastHLine(24, 65, 296, color_SWOOP);
		tft.drawFastHLine(24, 66, 296, color_SWOOP);		
		
		tft.drawFastHLine(24, 71, 296, color_SWOOP);
		tft.drawFastHLine(24, 72, 296, color_SWOOP);
		tft.drawFastHLine(24, 73, 296, color_SWOOP);
		tft.drawFastHLine(24, 74, 296, color_SWOOP);
		
		tft.fillRoundRect(50, -8, 40, 71, 10, ST77XX_BLACK);
		tft.fillRoundRect(50, 75, 40, 173, 10, ST77XX_BLACK);
		tft.setFont(&lcars15pt7b);
		drawParamText(188, 20, "AUDIO ANALYSIS", color_TITLETEXT);
		tft.setFont(&lcars11pt7b);
		drawParamText(73, 48, "0", color_MAINTEXT);
		drawParamText(107, 48, "DECIBEL", color_LABELTEXT3);
		tft.fillRoundRect(153, 32, 17, 18, 9, color_LABELTEXT4);
		tft.fillRect(153, 32, 6, 17, ST77XX_BLACK);
		drawParamText(194 + GetBuffer(0), 48, "0", color_MAINTEXT);
		drawParamText(227, 48, "MAXIMUM", color_LABELTEXT);
		tft.fillRoundRect(286, 32, 17, 18, 9, color_LABELTEXT4);
		tft.fillRect(286, 32, 6, 17, ST77XX_BLACK);		
		
		//PDM.setBufferSize(2);		
		PDM.setGain(150);
		PDM.onReceive(PullMicData);		
		//first param for begin function is # channels. this is always 1 for mono		
		mbMicrophoneStarted = PDM.begin(1, MIC_SAMPLERATE);
		
		//flip this last
		mbMicrophoneActive = true;
		//tft.drawFastVLine(60, 99, 128, ST77XX_WHITE);
		//tft.drawFastVLine(61, 97, 128, ST77XX_WHITE);
	} else {
		PDM.end();
		//need to zero out both arrays used by draw functions
		
		GoHome();
	}
}

void RunMicrophone() {
	if (!mbMicrophoneActive) return;	
	short nMicReadMax = 0;
	
	//separate if statements for action. 1 for data massage, 1 for display draw actions	
	//realtime polling seems like a lot more resources, but it actually appears to increase stability
	if (mnSamplesRead > 0) {	
		//use pdmwave value for decibel approximation?
		//poll this only once every few seconds - 5?
		if (millis() - mnLastMicRead > mnMicReadInterval) {
			int dbValue = 0;
			//dbValue = 20 * log10(abs(GetPDMWave(4000)) / 8000);
			//mapping can be 20* but 21.8 might be more accurate?
			dbValue = 21.8 * log10(GetPDMWave(4000));
			tft.fillRect(63, 32, 35, 18, ST77XX_BLACK);
			drawParamText(74 + GetBuffer(dbValue), 48, String(dbValue), color_MAINTEXT);
			//log values are in negative 2m or more
			//tft.fillRect(198, 32, 30, 18, ST77XX_BLACK);
			//dbValue = 20 * log10(GetPDMWave(4000) / 1500);
			//drawParamText(198, 48, String(dbValue), color_MAINTEXT);
			
			if (dbValue > mnMaxDBValue) {
				tft.fillRect(194, 32, 20, 18, ST77XX_BLACK);
				drawParamText(194 + GetBuffer(dbValue), 48, String(dbValue), color_MAINTEXT);
				mnMaxDBValue = dbValue;
			}
			mnLastMicRead = millis();
		}
		
		//fuck attempting to run this against a hard-coded filter, just do FFT on raw data?
		int nFFTStatus = ZeroFFT(mnarrSampleData, MIC_SAMPLESIZE);
		//zero the fuckin targets out between each read operation!
				
		//convert FFT data into distinct bin values
        if (nFFTStatus == 0) {
            int nSampleDivider = 8;
            //throw all samples into 
            for (int j = 1; j < (MIC_SAMPLESIZE / 2); j++) {
                //condense 128 samples down to boxes of 16
                int nArrayIndex = j / nSampleDivider;
				if (j % nSampleDivider == 1)
				mTargetMicDisplay[nArrayIndex] = 0;
				
                //mTargetMicDisplay[nArrayIndex] += mnarrSampleData[j];
				mTargetMicDisplay[nArrayIndex] += abs(mnarrSampleData[j]);
            }
			
			/*
			//normalize the incoming data?			
			for (int m = 1; m < FFT_BINCOUNT; m++) {
				nMicReadMax = max(mTargetMicDisplay[m], nMicReadMax);
				//if (m == 0)	nMicReadMax /= 2;
			}*/
			
            //convert combined bin values to usable averages.
            for (int k = 0; k < FFT_BINCOUNT; k++) {
				//int nFactor = map(k, 0, FFT_BINCOUNT - 1, 1, 8);
                //average the bin first?
                //mTargetMicDisplay[k] /= nSampleDivider;
				int nTemp = (k == 0) ? map(mTargetMicDisplay[k], 0, 50, 0, FFT_BARHEIGHTMAX) : map(mTargetMicDisplay[k] * (1 + k/2), 0, 50, 0, FFT_BARHEIGHTMAX);
				//int nTemp = map(mTargetMicDisplay[k] * nFactor, 0, 100, 0, FFT_BARHEIGHTMAX);
                //map bin average value to a usable target band height
                mTargetMicDisplay[k] = constrain(nTemp, 0, FFT_BARHEIGHTMAX);				
            }
        }
		//set boolean flag that display needs refresh
		//mnLastMicRead = millis();
		//this is a trigger to force subsequent loops to not blank graph if no mic data present
		mnSamplesRead = 0;
		//set global redraw flag for visualization
		mbMicrophoneRedraw = true;
	} else {
		if (!mbMicrophoneStarted) {
			//generate message that microphone is unavailable.
			drawParamText(120, 120, "MICROPHONE CONNECTION FAILURE", color_MAINTEXT);
			//mnLastMicRead = millis();
			mbMicrophoneRedraw = false;
			return;
		}
	}
	
	if (mbMicrophoneRedraw == true) {
		UpdateMicrophoneGraph(nMicReadMax, color_FFT);
		mbMicrophoneRedraw = false;
	}		
	
}	//end runmic

void drawReference() {
  //draw some reference lines?
  //163-64 = 99
  tft.drawFastVLine(63, 99, 128, ST77XX_WHITE);
  
  //uint16_t refStep = MIC_SAMPLESIZE / 2 / FFT_REFERENCELINES;
  //tft.setTextSize(2);
  //tft.setTextColor(ST77XX_RED);
  //for(int i=0; i < MIC_SAMPLESIZE / 2 - refStep; i+=refStep){
    //uint16_t fc = FFT_BIN(i, MIC_SAMPLERATE, MIC_SAMPLESIZE);
    //uint16_t xpos = map(i, 0, MIC_SAMPLESIZE / 2, 0, GRAPH_WIDTH);
    //tft.setCursor(xpos, 0);
    //tft.drawFastVLine(xpos + i, GRAPH_MIN, GRAPH_MAX - GRAPH_MIN, ST77XX_WHITE);    
    //tft.print(fc);
	//drawParamText(20, i + 20, String(fc), ST77XX_WHITE);
  //}
}

void PullMicData() {	
	//apparently this causes issues, as this function can't access a global variable?
	//if (mbMicrophoneActive == true) {	
	//no idea how to do this.  read() ONLY WORKS FOR SAMPLE RATE OF 16000!
	int nAvailable = PDM.available();
	//drawParamText(200, 100, String(nAvailable), ST77XX_WHITE);
	//mnSamplesRead = PDM.read(mnarrSampleData, MIC_SAMPLESIZE);
	PDM.read(mnarrSampleData, nAvailable);
	mnSamplesRead = nAvailable / 2;
}

int32_t GetPDMWave(int32_t nSamples) {
	short nMinwave = 30000;
	short nMaxwave = -30000;
	//performance on this function is hot garbage.

	while (nSamples > 0) {
		if (!mnSamplesRead || mnSamplesRead == 0) {
			//return 0;			
			yield();			
			//break;
			continue;
		}
		
		for (int i = 0; i < mnSamplesRead; i++) {
			nMinwave = min(mnarrSampleData[i], nMinwave);
			nMaxwave = max(mnarrSampleData[i], nMaxwave);
			nSamples--;
		}
		// clear the read count
		mnSamplesRead = 0;
	}
	return nMaxwave - nMinwave;
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

int ThreewayIntMax(uint16_t a, uint16_t b, uint16_t c) {	return max(a, max(b, c));	}

double ThreewayMax(double a, double b, double c) {	return max(a, max(b, c));	}

double ThreewayMin(double a, double b, double c) {	return min(a, min(b, c));	}

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

int Get1KBuffer(int nInput) { return (nInput < 1000) ? 8 : 0; }

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

void UpdateMicrophoneGraph(short nMaxDataValue, uint16_t nBarColor) {
    short nGraphMinX = 66;
    short nGraphMinY = 97;
    short nGraphZeroY = 163;
    short nBarWidth = 13;
    short nBarWidthMargined = 15;
    float fBarUpdateStepFactor = 1;
    short nTempYDifference = 0;
	//if nMaxDataValue not specified, values won't be normalized before draw action
    //graph bars should expand up & down from center line @ nGraphZeroY
	//this function's responsibility is to draw each FFT bin bar so the current display values match the target values
	for (int i = 0; i < FFT_BINCOUNT; i++) {
		if (nMaxDataValue > 0) {
			mTargetMicDisplay[i] /= nMaxDataValue;
			//then take this 0-1 value and multiply by graph bar height max to get target height
			mTargetMicDisplay[i] *= FFT_BARHEIGHTMAX;
			mTargetMicDisplay[i] = constrain(mTargetMicDisplay[i], 0, 64);
		}
		
        //do nothing if we're already at the target
        if (mTargetMicDisplay[i] == mCurrentMicDisplay[i]) {
            continue;
        } else if (mTargetMicDisplay[i] > mCurrentMicDisplay[i]) {            
            nTempYDifference = (mTargetMicDisplay[i] - mCurrentMicDisplay[i]);
            //top half & bottom half - instead of drawing twice, draw 1 rectangle that's twice as tall from same starting point
            tft.fillRect(nGraphMinX + (i * nBarWidthMargined), (nGraphZeroY - nTempYDifference), nBarWidth, nTempYDifference * 2, nBarColor);
            //update current bar height
            mCurrentMicDisplay[i] = mTargetMicDisplay[i];
        } else {
            nTempYDifference = (mCurrentMicDisplay[i] - mTargetMicDisplay[i]);
			//this requires 2 calls, as we need to trim from top down and bottom up for both halves of the bar
            //top half
            tft.fillRect(nGraphMinX + (i * nBarWidthMargined), (nGraphZeroY - mCurrentMicDisplay[i]), nBarWidth, nTempYDifference, ST77XX_BLACK);
            //bottom half
            tft.fillRect(nGraphMinX + (i * nBarWidthMargined), (nGraphZeroY + mTargetMicDisplay[i]), nBarWidth, nTempYDifference + 1, ST77XX_BLACK);
            //update current bar height
            mCurrentMicDisplay[i] = mTargetMicDisplay[i];
        }
    }

	//reset refresh flag to false?
	mbMicrophoneRedraw = false;
}