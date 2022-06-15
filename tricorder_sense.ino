#include <Adafruit_NeoPixel.h>
#include <SPI.h>
#include <Adafruit_GFX.h>    	// Core graphics library
#include <Adafruit_ST7789.h> 	// Hardware-specific library for ST7789
//#include <string.h>
#include <ArduinoJson.h>
#include <Adafruit_APDS9960.h>	//RGB, light, proximity, gesture sensor
#include <Wire.h>
#include <EasyButton.h>
#include <Adafruit_BMP280.h>	//altitude, barometer
#include <Adafruit_SHT31.h>		//temp, humidity
#include <Adafruit_Sensor.h>	//wtf is this??
#include <PDM.h>
#include <Adafruit_ZeroFFT.h>
#include <MLX90640_API.h>
#include <MLX90640_I2C_Driver.h>
#include <Adafruit_LIS3MDL.h>	//magnetometer, for door close detection
#include <Adafruit_LSM6DS33.h>


//full arduino pinout for this board is here:
/*https://github.com/adafruit/Adafruit_nRF52_Arduino/blob/master/variants/feather_nrf52840_sense/variant.h*/

// need to remove hyphens from header filenames or exception will get thrown
#include "Fonts/lcars15pt7b.h"
#include "Fonts/lcars11pt7b.h"

// For the breakout board, you can use any 2 or 3 pins.
// These pins will also work for the 1.8" TFT shield.
//need to use pin 6 for TFT_CS, as pin 20 is analog 7. analog 7 is the only way to get current voltage, which is used for battery %

//MISO is not required for screen to work - this is used by mem card only?
#define TFT_CS 						(6)
// SD card select pin
//#define SD_CS 			11 	//- can't use pin 4 as that is for blue connection led
#define TFT_RST 					-1
#define TFT_DC 						(5)
#define USE_SD_CARD 				(0)

//pin 9 is free, as pin_a6 is for vbat and is otherwise known as digital 20
#define VOLT_PIN 					PIN_A6		//INPUT_POWER_PIN
#define SOUND_TRIGGER_PIN			(9)

//buttons, scroller	- d2 pin supposed to be pin #2
//button on the board is connected to pin 7.  TX is pin 0, RX is pin 1 - these are normally used for serial communication

#define BUTTON_1_PIN				PIN_SERIAL1_RX
#define BUTTON_2_PIN				PIN_SERIAL1_TX
//adafruit defines physically labeled pin D2 as pin 2 in its header file, but it does not respond when set as an input by default.
//you will need to modify system_nrf52840.c file by adding
//#define CONFIG_NFCT_PINS_AS_GPIOS (1)
//YOU WILL SEE THIS CONSTANT REFERENCED IN THAT FILE, with compiler-conditional logic around it to actually free up the NFC pins, but only if that constant exists in that file
#define BUTTON_3_PIN				(2)

//pin 7 is the board button
#define BUTTON_BOARD				PIN_BUTTON1
//only need 1 pin for potentiometer / scroller input. both poles need to be wired to GND and 3v - order doesn't matter
#define PIN_SCROLL_INPUT			PIN_A0

#define SCAN_LED_PIN_1 			PIN_A2
#define SCAN_LED_PIN_2 			PIN_A3
#define SCAN_LED_PIN_3 			PIN_A4
#define SCAN_LED_PIN_4 			PIN_A5

#define SCAN_LED_BRIGHTNESS 	(32)

//neopixel power LED. must use an unreserved pin for this.  PWR, ID, EMRG all use this pin
#define NEOPIXEL_CHAIN_DATAPIN		(10)
#define NEOPIXEL_BRIGHTNESS 		(64)
#define NEOPIXEL_LED_COUNT 			(6)
// built-in pins: D4 = blue conn LED, 8 = neopixel on board, D13 = red LED next to micro usb port
// commented out lines are pre-defined by the adafruit board firmware
#define NEOPIXEL_BOARD_LED_PIN		(8)
//#define PIN_NEOPIXEL 				8
//#define BOARD_REDLED_PIN 			13
//#define BOARD_BLUELED_PIN 		4
//#define LED_RED 					13
//#define LED_BLUE					4

//system os version #. max 3 digits
#define DEVICE_VERSION			"0.94"
//theme definition: 0 = TNG, 1 = DS9, 2 = VOY
#define UX_THEME	(0)
#define THERMAL_CAMERA_PORTRAIT		(1)

//uncomment this line to have raw magnetometer z value displayed on home screen
//#define MAGNET_DEBUG	(0)
//the -700 threshold was based on resolution of 10, or 1024 max
//-20000 is based on the analog resolution required by battery pin (-32768 to 32768 range)
//this is intended for the z index reading of magnetometer (negative means field is below the board)
int mnMagnetSleepThreshold = -28000;

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

Adafruit_NeoPixel ledPwrStrip(NEOPIXEL_LED_COUNT, NEOPIXEL_CHAIN_DATAPIN, NEO_GRB + NEO_KHZ800);
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
//colors range is purple > blue > green > yellow > orange > red > pink > white
const uint32_t mnIDLEDColorscape[] = {0x8010,0x0010,0x0400,0x7C20,0x8300,0x8000,0x8208,0x7C30};
const String marrProfiles[] = {"ALPHA","BETA","GAMMA","DELTA","EPSILON","ZETA","ETA","THETA"};
//reverse order for these makes the scroller show alpha when 
//const String marrProfiles[] = {"THETA","ETA","ZETA","EPSILON","DELTA","GAMMA","BETA","ALPHA"};

const uint16_t mnThermalCameraLabels[] = {0xD6BA,0xC0A3,0xD541,0xD660,0x9E02,0x0458,0x89F1};

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
#define MIC_SAMPLESIZE 			256
//16k is the ONLY SUPPORTED SAMPLE RATE!
//#define MIC_SAMPLERATE		16000
#define MIC_AMPLITUDE			1000          // Depending on audio source level, you may need to alter this value. Can be used as a 'sensitivity' control.
#define DECIMATION				64
#define FFT_REFERENCELINES		16
//#define FFT_MAX					150
#define FFT_BINCOUNT        	16
#define FFT_BARHEIGHTMAX		64

#define GRAPH_OFFSET 			10
#define GRAPH_WIDTH (tft.width() - 3)
#define GRAPH_HEIGHT (tft.height() - GRAPH_OFFSET)
#define GRAPH_MIN (tft.height() - 2)
#define GRAPH_MAX (tft.height() - GRAPH_OFFSET)

long MIC_SAMPLERATE	= 16000;
int32_t mnMicVal = 0;
bool mbMicMaxRefresh = false;
short mnarrSampleData[MIC_SAMPLESIZE];
// number of samples read
volatile int mnSamplesRead = 0;
double mdarrActual[MIC_SAMPLESIZE];
double mdarrImaginary[MIC_SAMPLESIZE];
int mnMaxDBValue = 0;

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
bool mbButton7Flag = false;
EasyButton oButton1(BUTTON_1_PIN);
EasyButton oButton2(BUTTON_2_PIN);
EasyButton oButton3(BUTTON_3_PIN);
//board button digital pin 7 - use for thermal camera toggle
EasyButton oButton7(BUTTON_BOARD);

bool mbSleepMode = false;

Adafruit_LIS3MDL oMagneto;
bool mbMagnetometer = false;
float mfMagnetX, mfMagnetY, mfMagnetz;
int mnLastMagnetCheck = 0;
int mnMagnetInterval = 2000;
//if this cutoff is not working, either change where your speaker sits in the shell or add another magnet to your door to force the z-drop
int mnLastMagnetValue = 0;

//Adafruit_MLX90640 oThermalCamera;
const uint8_t mbCameraAddress = 0x33;
paramsMLX90640 moCameraParams;
#define TA_SHIFT 8
//temperature cutoffs are given in celsius. -12C => -10F
int MIN_CAMERA_TEMP = 20;
int MAX_CAMERA_TEMP = 35;
 
//need array length of 768 as this is 32*24 resolution
float mfarrTempFrame[768];
float mfTR = 0.0;
float mfTA = 0.0;
float mfCameraEmissivity = 0.95;
bool mbThermalCameraStarted = false;
bool mbThermalActive = false;
//this interval caps draw and thermal data frame rates. camera data will run slower than 30fps, but this throttle is needed to allow button press event polling
//50ms -> 20fps cap
uint8_t mnThermalCameraInterval = 50;
unsigned long mnLastCameraFrame = 0;
//this must be less than 10 for all data to be displayed on 320x240. 
//this scales display window to 256 x 192, a border of 24px all around
// if thermal camera is rotated 90 degrees, can change thermal to square viewport
// portrait hardware could be overall benefit, with viewport 216x216 (24*9)
uint16_t mnThermalPixelWidth = (THERMAL_CAMERA_PORTRAIT == 1) ? 9 : 8;
uint16_t mnThermalPixelHeight = (THERMAL_CAMERA_PORTRAIT == 1) ? 9 : 8;
uint8_t mnCameraDisplayStartX = (THERMAL_CAMERA_PORTRAIT == 1) ? 52 : 32;
uint8_t mnCameraDisplayStartY = (THERMAL_CAMERA_PORTRAIT == 1) ? 12 : 24;
//uint8_t mnCameraDisplayStartY = 0;

//16hz ~= 16fps
//while this is editable, it's probably not worth the time to decode and re-encode just to convert it to the desired LCARS color scheme
//lowest value is super dark purple: RGB 78,0,128
//highest value (0xF800) was 255,0,0
//2021.07.26	expanding color map to go from red to white for top-most end of temp range supported
const uint16_t mnarrThermalDisplayColors[] = {0x480F, 0x400F,0x400F,0x400F,0x4010,0x3810,0x3810,0x3810,0x3810,0x3010,0x3010,
0x3010,0x2810,0x2810,0x2810,0x2810,0x2010,0x2010,0x2010,0x1810,0x1810,0x1811,0x1811,0x1011,0x1011,0x1011,0x0811,0x0811,0x0811,0x0011,0x0011,
0x0011,0x0011,0x0011,0x0031,0x0031,0x0051,0x0072,0x0072,0x0092,0x00B2,0x00B2,0x00D2,0x00F2,0x00F2,0x0112,0x0132,0x0152,0x0152,0x0172,0x0192,
0x0192,0x01B2,0x01D2,0x01F3,0x01F3,0x0213,0x0233,0x0253,0x0253,0x0273,0x0293,0x02B3,0x02D3,0x02D3,0x02F3,0x0313,0x0333,0x0333,0x0353,0x0373,
0x0394,0x03B4,0x03D4,0x03D4,0x03F4,0x0414,0x0434,0x0454,0x0474,0x0474,0x0494,0x04B4,0x04D4,0x04F4,0x0514,0x0534,0x0534,0x0554,0x0554,0x0574,
0x0574,0x0573,0x0573,0x0573,0x0572,0x0572,0x0572,0x0571,0x0591,0x0591,0x0590,0x0590,0x058F,0x058F,0x058F,0x058E,0x05AE,0x05AE,0x05AD,0x05AD,
0x05AD,0x05AC,0x05AC,0x05AB,0x05CB,0x05CB,0x05CA,0x05CA,0x05CA,0x05C9,0x05C9,0x05C8,0x05E8,0x05E8,0x05E7,0x05E7,0x05E6,0x05E6,0x05E6,0x05E5,
0x05E5,0x0604,0x0604,0x0604,0x0603,0x0603,0x0602,0x0602,0x0601,0x0621,0x0621,0x0620,0x0620,0x0620,0x0620,0x0E20,0x0E20,0x0E40,0x1640,0x1640,
0x1E40,0x1E40,0x2640,0x2640,0x2E40,0x2E60,0x3660,0x3660,0x3E60,0x3E60,0x3E60,0x4660,0x4660,0x4E60,0x4E80,0x5680,0x5680,0x5E80,0x5E80,0x6680,
0x6680,0x6E80,0x6EA0,0x76A0,0x76A0,0x7EA0,0x7EA0,0x86A0,0x86A0,0x8EA0,0x8EC0,0x96C0,0x96C0,0x9EC0,0x9EC0,0xA6C0,0xAEC0,0xAEC0,0xB6E0,0xB6E0,
0xBEE0,0xBEE0,0xC6E0,0xC6E0,0xCEE0,0xCEE0,0xD6E0,0xD700,0xDF00,0xDEE0,0xDEC0,0xDEA0,0xDE80,0xDE80,0xE660,0xE640,0xE620,0xE600,0xE5E0,0xE5C0,
0xE5A0,0xE580,0xE560,0xE540,0xE520,0xE500,0xE4E0,0xE4C0,0xE4A0,0xE480,0xE460,0xEC40,0xEC20,0xEC00,0xEBE0,0xEBC0,0xEBA0,0xEB80,0xEB60,0xEB40,
0xEB20,0xEB00,0xEAE0,0xEAC0,0xEAA0,0xEA80,0xEA60,0xEA40,0xF220,0xF200,0xF1E0,0xF1C0,0xF1A0,0xF180,0xF160,0xF140,0xF100,0xF0E0,0xF0C0,0xF0A0,
0xF080,0xF060,0xF040,0xF020,0xF800
//}; 
,0xF841,0xF8A2,0xF8E3,0xF945,0xF986,0xF9E7,0xFA28,0xFA8A,0xFACB,0xFB2C,0xFB6D,0xFBCF,0xFC10,0xFC71,0xFCB2,0xFD14,0xFD55,0xFDB6,0xFDF7,0xFD59
,0xFE9A,0xFEFB,0xFF3C,0xFF9E};

bool mbTomServoActive = false;
unsigned long mnButton2Press = 0;
unsigned long mnButton3Press = 0;
int mnServoButtonWindow = 1500;
//graphs should take 3 seconds for full draw cycle-> 3000 / 80 lines
uint8_t mnServoDrawInterval = 38;
unsigned long mnServoLastDraw = 0;
uint8_t mnCurrentServoGraphPoint = 0;

//establish graph values
const uint8_t mnarrServoGraphData[] = {3,7,10,12,13,14,14,14,13,12,10,7,4,4,6,8,11,13,14,15,15,15,15,14,13,
									12,10,8,7,6,4,3,2,2,2,3,4,6,8,9,11,12,12,12,11,10,9,7,5,3,3,3,5,7,10,
									11,11,12,12,12,11,11,10,9,9,9,8,8,8,8,7,6,5,5,5,5,4,3,2,2,2};

void setup() {
	//NRF_UICR->NFCPINS = 0;
	ledPwrStrip.begin();
	ledBoard.begin();
	
	// use this initializer for a 2.0" 320x240 TFT. technically this is a rotated 240x320, so declaration is in that order
	tft.init(240, 320, SPI_MODE0); // Init ST7789 320x240
	tft.setRotation(1);
	tft.setFont(&lcars11pt7b);
	//these goggles, they do nothing!
	tft.setTextWrap(false);
		
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
	pinMode(LED_RED, OUTPUT);
	pinMode(LED_BLUE, OUTPUT);
	
	pinMode(SOUND_TRIGGER_PIN, OUTPUT);
	digitalWrite(SOUND_TRIGGER_PIN, HIGH);
		
	//or it'll be read as low and boot the tricorder into environment section
	pinMode(BUTTON_1_PIN, OUTPUT);
	pinMode(BUTTON_2_PIN, OUTPUT);
	pinMode(BUTTON_3_PIN, OUTPUT);
	digitalWrite(BUTTON_1_PIN, HIGH);
	digitalWrite(BUTTON_2_PIN, HIGH);
	digitalWrite(BUTTON_3_PIN, HIGH);
		
	delay(10);
		
	pinMode(PIN_SCROLL_INPUT, INPUT);
	
	pinMode(BUTTON_1_PIN, INPUT);
	pinMode(BUTTON_2_PIN, INPUT);
	pinMode(BUTTON_3_PIN, INPUT);
	
	pinMode(VOLT_PIN, INPUT);	
	pinMode(BUTTON_BOARD, INPUT);
	
	//initialize color sensor, show error if unavailable. sensor hard-coded name is "ADPS"
	//begin will return false if initialize failed. 
	//this shit is super plug & play - library uses i2c address 0x39, same as one used by this board
	//DO NOT call any functions before the begin, or you'll lock up the board
	mbColorInitialized = oColorSensor.begin();	
	
	if (mbColorInitialized) {
		//need rgb to cap at 255 for calculations? this isn't limiting shit.
		//proximity sensor has a range of 4-8 inches, or 10-20cm
		oColorSensor.setIntLimits(0, 255);
		oColorSensor.enableColor(true);
		oColorSensor.enableProximity(true);
		oColorSensor.setADCGain(APDS9960_AGAIN_16X);
		oColorSensor.enableColorInterrupt();
		oColorSensor.setADCIntegrationTime(50);		
	}
	
	//temp sensor
	mbTempInitialized = oTempBarom.begin();
	mbHumidityInitialized = oHumid.begin();	
	
	//microphone
	PDM.onReceive(PullMicData);
	//PDM.setGain(50);
	mbMicrophoneStarted = PDM.begin(1, MIC_SAMPLERATE);
	PDM.end();
	
	mbMagnetometer = oMagneto.begin_I2C();
	
	//all magnet settings - data rate of 1.25Hz a bit faster than 1 per second
	oMagneto.setPerformanceMode(LIS3MDL_MEDIUMMODE);
	oMagneto.setOperationMode(LIS3MDL_CONTINUOUSMODE);
	//oMagneto.setIntThreshold(500);
	//can be 4, 8, 12, 16 - don't need high range here, as magnet in door will be pretty strong and very close
	oMagneto.setRange(LIS3MDL_RANGE_8_GAUSS);
	/*oMagneto.configInterrupt(false, false, true, true, false, true);
	*/
	
	SetThermalClock();
	uint16_t oCameraParams[834];
	int nStatus = -1;
	
	nStatus = MLX90640_DumpEE(mbCameraAddress, oCameraParams);
	
	if (nStatus == 0) {
		mbThermalCameraStarted = true;
		nStatus = MLX90640_ExtractParameters(oCameraParams, &moCameraParams);
		//start at 1hz
		MLX90640_SetRefreshRate(mbCameraAddress, 0x01);
		//oThermalCamera.setMode(MLX90640_CHESS);
		//oThermalCamera.setResolution(MLX90640_ADC_16BIT);
		//minimize refresh rate since we can't turn off the camera? 
		//refresh rate will be double viable display frame rate, as need 2 data pulls per frame
		//1Hz refresh rate works when clock is at 100kHz
		//oThermalCamera.setRefreshRate(MLX90640_1_HZ);
		//4Hz refresh rate works when clock is at 400kHz - 2fps
		//oThermalCamera.setRefreshRate(MLX90640_4_HZ);		
		//Wire.setClock(1000000); // max 1 MHz gets translated to 400kHz with adafruit "driver"
		//TWIM_FREQUENCY_FREQUENCY_K100 is default for board?
		//TWIM_FREQUENCY_FREQUENCY_K250
		//TWIM_FREQUENCY_FREQUENCY_K400		
		//8fps for screen is viable if clock speed is 800kHz & camera refresh rate is 16hz
		//need to modify wire_nrf52.cpp to support 
		//TWIM_FREQUENCY_FREQUENCY_K1000
		////Wire.setClock(TWIM_FREQUENCY_FREQUENCY_K400);
		//Wire.endTransmission(MLX90640_I2CADDR_DEFAULT);
	}
	ResetWireClock();
	
	oButton2.begin();
	oButton2.onPressed(ToggleRGBSensor);	
	
	oButton1.begin();
	oButton1.onPressed(ToggleClimateSensor);
	
	oButton3.begin();
	oButton3.onPressed(ToggleMicrophone);
	//oButton3.onPressed(ActivateTomServo);
	
	oButton7.begin();
	oButton7.onPressed(ToggleThermal);
	//10k potentiometer / scroller should be limited to a readable range of 10-890
	//analog write values can go from 0 to 255. analogRead can go from 0 to 2^(analogReadResolution)
	//changing this WILL screw with other stuff, like the magnetometer & battery
	//analogReadResolution(10);
	GoHome();
}

void loop() {
	//if magnet read in Z direction is over a threshold, trigger sleep.
	//check this first in the loop, as everything else depends on it
	//magnet from speaker throws z = ~ +14000, no magnets has z idle at ~ -500
	//use z > 5000 ?
	//all analogRead actions depend on resolution setting - changing this will screw with battery % readings
	
	//need tests with speaker above and door magnet below - may require testing within assembled shell
	#if !defined(MAGNET_DEBUG)
		if (mbMagnetometer && (millis() - mnLastMagnetCheck) > mnMagnetInterval) {
			oMagneto.read();
			//magnet function modification needs to use a massive drop as the sleep trigger.
			int nCurrentMagnetZ = oMagneto.y;
						
			if (!mbSleepMode && (nCurrentMagnetZ < mnMagnetSleepThreshold)) {
				SleepMode();
			} else if (mbSleepMode && (nCurrentMagnetZ > mnMagnetSleepThreshold)) {
				ActiveMode();
			}			
			mnLastMagnetCheck = millis();
		}
	#endif
	
	if (mbSleepMode) {
		if (ledPwrStrip.getPixelColor(1) != ST77XX_BLACK) {
			ledPwrStrip.clear();
			ledPwrStrip.show();
		}
		return;
	}
	
	//toggle climate
	oButton1.read();
	//this toggles RGB scanner
	oButton2.read();	
	//toggle microphone
	oButton3.read();
	//toggle thermal camera
	oButton7.read();
			
	RunNeoPixelColor(NEOPIXEL_CHAIN_DATAPIN);
	RunNeoPixelColor(NEOPIXEL_BOARD_LED_PIN);
	RunLeftScanner();
	RunHome();
	//blink board LEDs
	RunBoardLEDs();
	
	RunRGBSensor();
	RunClimateSensor();
	RunMicrophone();	
	RunThermal();
	RunTomServo();
}

void SleepMode() {
	mbSleepMode = true;
	
	//turn off screen 
	tft.fillScreen(ST77XX_BLACK);		
	//need wired pin to set backlight low here
	//digitalWrite(DISPLAY_BACKLIGHT_PIN, LOW);
	tft.enableSleep(true);
	
	//set sound trigger pin HIGH, as low causes playback
	digitalWrite(SOUND_TRIGGER_PIN, HIGH);
	
	//board edge LEDs off
	digitalWrite(LED_RED, LOW);
	digitalWrite(LED_BLUE, LOW);
	//board "power" / "camera flash" LED off
	ledBoard.setPixelColor(0, 0, 0, 0);
	ledBoard.show();
	//left scanner LEDs off
	analogWrite(SCAN_LED_PIN_1, 0);
	analogWrite(SCAN_LED_PIN_2, 0);
	analogWrite(SCAN_LED_PIN_3, 0);
	analogWrite(SCAN_LED_PIN_4, 0);
	//set chained neopixels off - PWR, EMRG, ID	
	ledPwrStrip.clear();
	//ledPwrStrip.show();
	mbLEDIDSet = false;
	
	//reset all "status" variables
	mnRGBCooldown = 0;
	mnClimateCooldown = 0;
	mbHomeActive = true;
	mbRGBActive = false;
	mbTempActive = false;	
	mbThermalActive = false;
	mbTomServoActive = false;
	mbMicrophoneActive = false;
	mnCurrentServoGraphPoint = 0;
	mnServoLastDraw = 0;
	SetActiveNeoPixelButton(0);
	
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
}

void ActiveMode() {	
	mbSleepMode = false;	
	tft.enableSleep(false);
	//tft.enableDisplay(true);
	//force immediate refresh of neopixel LEDs
	mnLastUpdatePower = 0;
	mnLastUpdateIDLED = 0;
	mnLastUpdateEMRG = 0;
	GoHome();
}

void ActivateSound() {
	digitalWrite(SOUND_TRIGGER_PIN, LOW);
}

void DisableSound() {
	digitalWrite(SOUND_TRIGGER_PIN, HIGH);
}

void RunNeoPixelColor(int nPin) {
	unsigned long lTimer = millis();
	if (nPin == NEOPIXEL_CHAIN_DATAPIN) {
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
				int nBattMap = GetBatteryTier();
				switch (nBattMap) {
					case 5: ledPwrStrip.setPixelColor(0, 0, 0, 128); break;
					case 4: ledPwrStrip.setPixelColor(0, 0, 0, 128); break;
					case 3: ledPwrStrip.setPixelColor(0, 0, 128, 0); break;
					case 2: ledPwrStrip.setPixelColor(0, 112, 128, 0); break;
					case 1: ledPwrStrip.setPixelColor(0, 128, 96, 0); break;
					default: ledPwrStrip.setPixelColor(0, 128, 0, 0); break;
				}								
			}	//end if powercolorCycle
						
			mnLastUpdatePower = lTimer;
			ledPwrStrip.show();
		}
		
		//need to push ID LED separate from power, as PWR only updates every 30 seconds
		if (mnLastUpdateIDLED == 0 || ((lTimer - mnLastUpdateIDLED) > mnIDLEDInterval)) {
			//set ID LED color based on value pulled from A0, middle prong of scroll potentiometer
			//colors range is purple > blue > green > yellow > orange > red > pink > white
			uint16_t nScrollerValue = analogRead(PIN_SCROLL_INPUT);
			uint16_t nTempColor = nScrollerValue;
			if (nScrollerValue < 110) {
				nTempColor = 0;
			} else if (nScrollerValue < 220) {
				nTempColor = 1;
			} else if (nScrollerValue < 330) {
				nTempColor = 2;
			} else if (nScrollerValue < 440) {
				nTempColor = 3;
			} else if (nScrollerValue < 550) {
				nTempColor = 4;
			} else if (nScrollerValue < 660) {
				nTempColor = 5;
			} else if (nScrollerValue < 770) {
				nTempColor = 6;
			} else {
				nTempColor = 7;
			}
			msCurrentProfileName = marrProfiles[nTempColor];
			
			//calling uint16 parameter function for set color was not working. converting uint16 to rgb
			mnCurrentProfileRed = ((((mnIDLEDColorscape[nTempColor] >> 11) & 0x1F) * 527) + 23) >> 6;
			mnCurrentProfileGreen = ((((mnIDLEDColorscape[nTempColor] >> 5) & 0x3F) * 259) + 33) >> 6;
			mnCurrentProfileBlue = (((mnIDLEDColorscape[nTempColor] & 0x1F) * 527) + 23) >> 6;
			//NEOPIXEL_LED_COUNT
			//ledPwrStrip.setPixelColor(1, mnCurrentProfileRed, mnCurrentProfileGreen, mnCurrentProfileBlue);
			ledPwrStrip.setPixelColor(NEOPIXEL_LED_COUNT-2, mnCurrentProfileRed, mnCurrentProfileGreen, mnCurrentProfileBlue);
			
			ledPwrStrip.show();
			mnLastUpdateIDLED = lTimer;
		}
		
		//throb EMRG LED
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
			//ledPwrStrip.setPixelColor(2, nCurrentEMRG, 0, 0);
			ledPwrStrip.setPixelColor(NEOPIXEL_LED_COUNT-1, nCurrentEMRG, 0, 0);
			ledPwrStrip.show();
			mnLastUpdateEMRG = lTimer;
		}		
		
	} else if (nPin == NEOPIXEL_BOARD_LED_PIN) {
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

void SetActiveNeoPixelButton(int nButtonID) {
	if (NEOPIXEL_LED_COUNT != 6) return;
	//if bottom right of screen gets a tiny led board, extend same logic to those?
	//NEOPIXEL_LED_COUNT would be 9 in that case
	switch (nButtonID) {
		//home screen, NO APPS ACTIVE
		case 0: ledPwrStrip.setPixelColor(NEOPIXEL_LED_COUNT-5, 0, 128, 0);
				ledPwrStrip.setPixelColor(NEOPIXEL_LED_COUNT-4, 0, 128, 0);
				ledPwrStrip.setPixelColor(NEOPIXEL_LED_COUNT-3, 0, 128, 0);
				break;
		//GEO
		case 1: ledPwrStrip.setPixelColor(NEOPIXEL_LED_COUNT-5, 128, 96, 0);
				ledPwrStrip.setPixelColor(NEOPIXEL_LED_COUNT-4, 0, 128, 0);
				ledPwrStrip.setPixelColor(NEOPIXEL_LED_COUNT-3, 0, 128, 0);
				break;
		//MET
		case 2: ledPwrStrip.setPixelColor(NEOPIXEL_LED_COUNT-5, 0, 128, 0);
				ledPwrStrip.setPixelColor(NEOPIXEL_LED_COUNT-4, 128, 96, 0);
				ledPwrStrip.setPixelColor(NEOPIXEL_LED_COUNT-3, 0, 128, 0);
				break;
		//BIO
		case 3: ledPwrStrip.setPixelColor(NEOPIXEL_LED_COUNT-5, 0, 128, 0);
				ledPwrStrip.setPixelColor(NEOPIXEL_LED_COUNT-4, 0, 128, 0);
				ledPwrStrip.setPixelColor(NEOPIXEL_LED_COUNT-3, 128, 96, 0);
				break;
		//CAMERA - all RED
		case 4: ledPwrStrip.setPixelColor(NEOPIXEL_LED_COUNT-5, 128, 0, 0);
				ledPwrStrip.setPixelColor(NEOPIXEL_LED_COUNT-4, 128, 0, 0);
				ledPwrStrip.setPixelColor(NEOPIXEL_LED_COUNT-3, 128, 0, 0);
				break;
		//TOM SERVO - all BLUE? PURPLE?
		case 5: ledPwrStrip.setPixelColor(NEOPIXEL_LED_COUNT-5, 0, 0, 128);
				ledPwrStrip.setPixelColor(NEOPIXEL_LED_COUNT-4, 0, 0, 128);
				ledPwrStrip.setPixelColor(NEOPIXEL_LED_COUNT-3, 0, 0, 128);
				break;
	}
	ledPwrStrip.show();
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
	if ((lTimer - mnLastUpdateBoardBlueLED) > mnBoardBlueLEDInterval) {
		mbBoardBlueLED = !mbBoardBlueLED;
		if (mbBoardRedLED) {
			digitalWrite(LED_BLUE, HIGH);
		} else {
			digitalWrite(LED_BLUE, LOW);
		}
		mnLastUpdateBoardBlueLED = lTimer;
	}	
}

void ActivateFlash() {
	//all neopixel objects are chains, so have to call it by addr
	ledBoard.setPixelColor(0, 255, 255, 255);
	ledBoard.show();
}

void DisableFlash() {
	ledBoard.setPixelColor(0, 0, 0, 0);
	ledBoard.show();
}

//to-do: add parameters to change cycle behavior of LEDs.
//ex: cycled down-> up, stacking, unified blink, KITT ping pong, etc
void RunLeftScanner() {
	unsigned long lTimer = millis();
		
	//when color scanner running, use left side lights to convey scan coming soon:
	if (!mbRGBActive) {
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
	} else {
		//currently stacks "downward" - all on to only alpha on - which is more like a lifting drape
		switch (mnRGBCooldown) {
			case 1: 
				analogWrite(SCAN_LED_PIN_1, SCAN_LED_BRIGHTNESS); 
				analogWrite(SCAN_LED_PIN_2, 0); 
				analogWrite(SCAN_LED_PIN_3, 0); 
				analogWrite(SCAN_LED_PIN_4, 0); 
				break;
			case 2: 
				analogWrite(SCAN_LED_PIN_1, SCAN_LED_BRIGHTNESS); 
				analogWrite(SCAN_LED_PIN_2, SCAN_LED_BRIGHTNESS); 
				analogWrite(SCAN_LED_PIN_3, 0); 
				analogWrite(SCAN_LED_PIN_4, 0); 
				break;
			case 3: 
				analogWrite(SCAN_LED_PIN_1, SCAN_LED_BRIGHTNESS); 
				analogWrite(SCAN_LED_PIN_2, SCAN_LED_BRIGHTNESS); 
				analogWrite(SCAN_LED_PIN_3, SCAN_LED_BRIGHTNESS); 
				analogWrite(SCAN_LED_PIN_4, 0); 
				break;
			case 4: 
				analogWrite(SCAN_LED_PIN_1, SCAN_LED_BRIGHTNESS); 
				analogWrite(SCAN_LED_PIN_2, SCAN_LED_BRIGHTNESS); 
				analogWrite(SCAN_LED_PIN_3, SCAN_LED_BRIGHTNESS); 
				analogWrite(SCAN_LED_PIN_4, SCAN_LED_BRIGHTNESS); 
				break;
			default: 
				analogWrite(SCAN_LED_PIN_1, 0); 
				analogWrite(SCAN_LED_PIN_2, 0); 
				analogWrite(SCAN_LED_PIN_3, 0); 
				analogWrite(SCAN_LED_PIN_4, 0); 
				break;
		}
	}
}

//to-do: call scanner clear on any mode change?
void ClearLeftScanner() {
	analogWrite(SCAN_LED_PIN_1, 0); 
	analogWrite(SCAN_LED_PIN_2, 0); 
	analogWrite(SCAN_LED_PIN_3, 0); 
	analogWrite(SCAN_LED_PIN_4, 0); 
	mnLeftLEDCurrent = 1;
}

void GoHome() {
	ResetWireClock();
	DisableSound();
	//if flag set for thermal debug mode, hijack home screen?	
	//reset any previous sensor statuses
	mnRGBCooldown = 0;
	mnClimateCooldown = 0;
	mbHomeActive = true;
	mbRGBActive = false;
	mbTempActive = false;
	mnCurrentServoGraphPoint = 0;
	mnServoLastDraw = 0;
	SetActiveNeoPixelButton(0);
	
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
		
	//color_LABELTEXT2 brown, labeltext3 pinkish, labeltext4 is tan/orange
	//show sensor statuses		
	//environment = temperature, humidity sensors
	//119x34, 7 pixels between rectangle and cap
	//82x34 if no cap
	if (mbTempInitialized) {
		//tft.fillRect(76, 119, 139, 24, color_LABELTEXT4);
		////actual status tab
		//tft.fillRoundRect(279, 119, 24, 24, 11, color_LABELTEXT4);
		//tft.fillRect(230, 119, 65, 24, color_LABELTEXT4);
		//drawParamText(239, 138, mbHumidityInitialized ? "ONLINE" : "PARTIAL", ST77XX_BLACK);
		tft.fillRoundRect(70, 119, 113, 34, 17, (mbHumidityInitialized ? color_LABELTEXT4 : color_LABELTEXT3));
		tft.fillRect(70, 119, 18, 34, (mbHumidityInitialized ? color_LABELTEXT4 : color_LABELTEXT3));		
	} else if (mbHumidityInitialized) {
		//tft.fillRect(76, 119, 139, 24, color_LABELTEXT3);
		////actual status tab
		//tft.fillRoundRect(279, 150, 24, 24, 11, color_LABELTEXT3);
		//tft.fillRect(230, 119, 65, 24, color_LABELTEXT3);
		//drawParamText(239, 138, "PARTIAL", ST77XX_BLACK);
		tft.fillRoundRect(70, 119, 113, 34, 17, color_LABELTEXT3);
		tft.fillRect(70, 119, 18, 34, color_LABELTEXT3);
	} else {
		//offline, show no round cap
		//tft.fillRect(76, 119, 139, 24, color_LABELTEXT2);
		tft.fillRect(70, 119, 81, 34, color_LABELTEXT2);		
	}
	//drawParamText(159, 138, "CLIMATE", ST77XX_BLACK);
	drawParamText(78, 142, "CLIMATE", ST77XX_BLACK);
	
	if (mbColorInitialized) {
		//tft.fillRect(76, 150, 139, 24, color_LABELTEXT4);		
		//tft.fillRoundRect(279, 150, 24, 24, 11, color_LABELTEXT4);
		//tft.fillRect(230, 150, 65, 24, color_LABELTEXT4);		
		//drawParamText(239, 169, "ONLINE", ST77XX_BLACK);
		tft.fillRoundRect(70, 168, 113, 34, 17, color_LABELTEXT4);
		tft.fillRect(70, 168, 18, 34, color_LABELTEXT4);
	} else {		
		tft.fillRect(70, 168, 82, 34, color_LABELTEXT2);
	}
	drawParamText(78, 191, "CHROMATICS", ST77XX_BLACK);
	//drawParamText(137, 169, "CHROMATICS", ST77XX_BLACK);
	
	if (mbMicrophoneStarted) {
		//tft.fillRect(76, 181, 139, 24, color_LABELTEXT4);		
		//tft.fillRoundRect(279, 181, 24, 24, 11, color_LABELTEXT4);
		//tft.fillRect(230, 181, 65, 24, color_LABELTEXT4);
		//drawParamText(239, 200, "ONLINE", ST77XX_BLACK);
		tft.fillRoundRect(192, 119, 113, 34, 17, color_LABELTEXT4);
		tft.fillRect(192, 119, 18, 34, color_LABELTEXT4);
	} else {
		//tft.fillRect(76, 181, 139, 24, color_LABELTEXT2);
		tft.fillRect(192, 119, 82, 34, color_LABELTEXT2);
	}	
	//drawParamText(169, 200, "SONICS", ST77XX_BLACK);
	drawParamText(200, 142, "SONICS", ST77XX_BLACK);
	
	if (mbThermalCameraStarted) {
		tft.fillRoundRect(192, 168, 113, 34, 17, color_LABELTEXT4);
		tft.fillRect(192, 168, 18, 34, color_LABELTEXT4);
	} else {
		tft.fillRect(192, 168, 82, 34, color_LABELTEXT2);
	}
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
		
		//tft.fillRect(250, 60, 60, 20, ST77XX_BLACK);
		//drawParamText(250, 75, (String)analogRead(PIN_SCROLL_INPUT), color_MAINTEXT); 
		
		mnLastUpdateHome = nNowMillis;
	}
	//raw magnet z index data output to home screen for debug
	//#if defined(MAGNET_DEBUG)
	if (mbMagnetometer && ((nNowMillis - mnLastMagnetCheck) > mnMagnetInterval)) {
		oMagneto.read();
		//output raw data to screen - test this with magnet behind 4mm of PLA ~
		//drawParamText(250, 25, (String)oMagneto.x, color_MAINTEXT);
		//drawParamText(250, 50, (String)oMagneto.y, color_MAINTEXT);
		tft.fillRect(250, 45, 40, 35, ST77XX_BLACK);
		//drawParamText(250, 55, (String)oMagneto.x, color_MAINTEXT);
		drawParamText(250, 75, (String)oMagneto.y, color_MAINTEXT);
		mnLastMagnetCheck = nNowMillis;
	}
	//#endif
}

void ResetWireClock() {
	//Wire.endTransmission(MLX90640_I2CADDR_DEFAULT);
	//all sensors on i2c use 100,000 rate. 
	//adafruit board definitions force any call to this under 100k up to 100k.
	Wire.flush();
	Wire.setClock(100000);
}

void SetThermalClock() {
	//max viable speed on this board. used only for thermal camera. 
	//adafruit board definitions force any call to this with over 400k down to 400k.
	Wire.flush();	
	Wire.setClock(400000);
}

void ToggleRGBSensor() {
	ResetWireClock();
	
	//set timestamp of button press and check if this activates tomservo
	mnButton2Press = millis();
	int nTomServoButtonDifference = mnButton2Press - mnButton3Press;
	if (abs(nTomServoButtonDifference) < mnServoButtonWindow) {
		mbTomServoActive = true;
		DisableSound();
		ActivateTomServo();
		return;
	}
	mnCurrentServoGraphPoint = 0;
	mnServoLastDraw = 0;
	
	mbButton2Flag = !mbButton2Flag;
	//reset any temperature app values
	mbTempActive = false;
	mbHomeActive = false;
	mbMicrophoneActive = false;
	mbThermalActive = false;
	mnTempTargetBar = 0;
	mnTempCurrentBar = 0;
	mnHumidTargetBar = 0;
	mnHumidCurrentBar = 0;
	mnBaromTargetBar = 0;
	mnBaromCurrentBar = 0;
	mbHumidBarComplete = false;
	mbTempBarComplete = false;
	mbBaromBarComplete = false;
	mbTomServoActive = false;
	
	if (mbButton2Flag) {
		
		if (!mbColorInitialized) {
			if (millis() - mnLastRGBScan > mnRGBScanInterval) {
				tft.fillScreen(ST77XX_BLACK);
				drawParamText(211, 21, "CHROMATICS", color_TITLETEXT);
				drawParamText(110, 169, "SENSOR OFFLINE", color_MAINTEXT);
				mnLastRGBScan = millis();		
			}		
			return;
		}
		
		if (!mbRGBActive) {
			SetActiveNeoPixelButton(2);
			ActivateSound();
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
			//drawParamText(151, 21, "CHROMATIC ANALYSIS", color_TITLETEXT);
			drawParamText(211, 21, "CHROMATICS", color_TITLETEXT);
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
		drawParamCharText(203 + GetBuffer(arrdHSL[0]), 48, const_cast<char*>(TruncateDouble(arrdHSL[0]).c_str()), color_MAINTEXT);
		drawParamCharText(203 + GetBuffer(arrdHSL[1]), 74, const_cast<char*>(TruncateDouble(arrdHSL[1]).c_str()), color_MAINTEXT);
		drawParamCharText(203 + GetBuffer(arrdHSL[2]), 99, const_cast<char*>(TruncateDouble(arrdHSL[2]).c_str()), color_MAINTEXT);
		
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
		drawParamCharText(185, 176, const_cast<char*>(strTemp.c_str()), color_MAINTEXT);
		strTemp = String(nRGB565, HEX);
		strTemp.toUpperCase();
		strTemp = "0x" + strTemp;
		drawParamCharText(185, 150, const_cast<char*>(strTemp.c_str()), color_MAINTEXT);
		
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
		
		drawParamCharText(78 + GetBuffer(dblack *100), 228, const_cast<char*>(TruncateDouble(dblack * 100).c_str()), color_MAINTEXT);
		//use rgb array for cmy to minimize memory use
		// red -> magenta, green -> yellow, blue -> cyan
		arrdRGB[0] = ((1 - fGreen - dblack) / (1 - dblack)) * 100;
		arrdRGB[2] = ((1 - fRed - dblack) / (1 - dblack)) * 100;
		arrdRGB[1] = ((1 - fBlue - dblack) / (1 - dblack)) * 100; 
		
		drawParamCharText(78 + GetBuffer(arrdRGB[2]), 150, const_cast<char*>(TruncateDouble(arrdRGB[2]).c_str()), color_MAINTEXT);
		drawParamCharText(78 + GetBuffer(arrdRGB[0]), 176, const_cast<char*>(TruncateDouble(arrdRGB[0]).c_str()), color_MAINTEXT);
		drawParamCharText(78 + GetBuffer(arrdRGB[1]), 202, const_cast<char*>(TruncateDouble(arrdRGB[1]).c_str()), color_MAINTEXT);	
					
		//draw swatch, turn off scanner LED
		tft.fillRoundRect(186, 189, 113, 42, 21, nRGB565);
		uint8_t nProximity = 0;
		nProximity = oColorSensor.readProximity();
		//put proximity value inside color swatch rect - assume distance in mm?
		if (nProximity > 0 && nProximity < 256) {
			drawParamText(240 + GetBuffer(nProximity), 225, String(nProximity) + "MM", ST77XX_BLACK);
		}
		
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
				drawParamCharText(37, 71, const_cast<char*>(((String)mnRGBCooldown).c_str()), ST77XX_BLACK);
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

void ToggleClimateSensor() {
	ResetWireClock();		
	mbButton1Flag = !mbButton1Flag;
	//reset any rgb sensor values
	mnRGBCooldown = 0;
	mnClimateCooldown = 0;	
	mbRGBActive = false;
	mbMicrophoneActive = false;
	mbHomeActive = false;
	mbThermalActive = false;
	mbTomServoActive = false;
	mnCurrentServoGraphPoint = 0;
	mnServoLastDraw = 0;
	
	if (mbButton1Flag) {		
		if (!mbTempActive) {
			SetActiveNeoPixelButton(1);
			ActivateSound();
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
			
			//small black box for "scroller" 
			tft.fillRect(123, 86, 30, 8, ST77XX_BLACK);
			tft.setFont(&lcars15pt7b);
			//drawParamText(174, 20, "CLIMATE ANALYSIS", color_TITLETEXT);			
			drawParamText(244, 20, "CLIMATE", color_TITLETEXT);
			
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
			drawParamCharText(86, 46, const_cast<char*>(sTempC.c_str()), color_MAINTEXT);
			//kelvin - int, should always be 3 digits - this is a straight conversion
			int nTempK = (int)(fTempC + 273.15);
			drawParamCharText(90, 72, const_cast<char*>(((String)nTempK).c_str()), color_MAINTEXT);
			
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
			
			drawParamCharText(204, 46, const_cast<char*>(sHumid.c_str()), color_MAINTEXT);
			//barometric pressure - maximum human survivable is ~ 70*14.7 psi ~= 70000 millibar (sea diving)
			drawParamCharText(202 + Get1KBuffer(mnBarom), 72, const_cast<char*>(((String)nBarom).c_str()), color_MAINTEXT);
			
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
	ResetWireClock();
	
	//set timestamp of button press and check if this activates tomservo
	mnButton3Press = millis();
	//if the 2nd and 3rd buttons are pressed within 1 second of each other, go tom servo
	int nTomServoButtonDifference = mnButton2Press - mnButton3Press;
	if (abs(nTomServoButtonDifference) < mnServoButtonWindow) {		
		mbTomServoActive = true;
		DisableSound();
		ActivateTomServo();
		return;
	}
	mnCurrentServoGraphPoint = 0;
	mnServoLastDraw = 0;
	
	mbButton3Flag = !mbButton3Flag;
	//reset any rgb sensor values
	mnRGBCooldown = 0;
	mnClimateCooldown = 0;	
	mbMicrophoneActive = false;
	mbThermalActive = false;
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
	mbTomServoActive = false;
	mnCurrentServoGraphPoint = 0;
	mnServoLastDraw = 0;
	mbRGBActive = false;
	
	if (mbButton3Flag) {
		DisableSound();
		SetActiveNeoPixelButton(3);
		//show audio screen
		//tft.fillScreen(0x6B6D);
		tft.fillScreen(ST77XX_BLACK);
		//fillRoundRect(x,y,width,height,cornerRadius, color)
		tft.fillRoundRect(0, -25, 85, 91, 25, color_SWOOP);
		tft.fillRoundRect(0, 71, 85, 194, 25, color_SWOOP);
		
		//tft.drawFastHLine(123, 61, 30, color_SWOOP);
		//break these into 3 each to avoid a redundant black vline call
		//tft.drawFastHLine(24, 63, 296, color_SWOOP);
		//97,99,98
		tft.drawFastHLine(24, 63, 96, color_SWOOP);
		tft.drawFastHLine(123, 63, 97, color_SWOOP);
		tft.drawFastHLine(223, 63, 97, color_SWOOP);
		
		//tft.drawFastHLine(24, 64, 296, color_SWOOP);
		tft.drawFastHLine(24, 64, 96, color_SWOOP);
		tft.drawFastHLine(123, 64, 97, color_SWOOP);
		tft.drawFastHLine(223, 64, 97, color_SWOOP);
		
		//tft.drawFastHLine(24, 65, 296, color_SWOOP);
		tft.drawFastHLine(24, 65, 96, color_SWOOP);
		tft.drawFastHLine(123, 65, 97, color_SWOOP);
		tft.drawFastHLine(223, 65, 97, color_SWOOP);
		
		//break this into 2 lines to avoid having to draw over it with a black fillRect call
		//tft.drawFastHLine(24, 66, 296, color_SWOOP);
		tft.drawFastHLine(24, 66, 96, color_SWOOP);
		tft.drawFastHLine(153, 66, 67, color_SWOOP);
		tft.drawFastHLine(223, 66, 97, color_SWOOP);
		
		//break this into 2 lines to avoid having to draw over it with a black fillRect call
		//tft.drawFastHLine(24, 71, 296, color_SWOOP);
		//tft.drawFastHLine(24, 71, 99, color_SWOOP);
		//tft.drawFastHLine(153, 71, 167, color_SWOOP);
		tft.drawFastHLine(24, 71, 96, color_SWOOP);
		tft.drawFastHLine(153, 71, 67, color_SWOOP);
		tft.drawFastHLine(223, 71, 97, color_SWOOP);
		
		//break these into 3 each to avoid a redundant black vline call
		//tft.drawFastHLine(24, 72, 296, color_SWOOP);
		tft.drawFastHLine(24, 72, 96, color_SWOOP);
		tft.drawFastHLine(123, 72, 97, color_SWOOP);
		tft.drawFastHLine(223, 72, 97, color_SWOOP);
		
		//tft.drawFastHLine(24, 73, 296, color_SWOOP);
		tft.drawFastHLine(24, 73, 96, color_SWOOP);
		tft.drawFastHLine(123, 73, 97, color_SWOOP);
		tft.drawFastHLine(223, 73, 97, color_SWOOP);
		
		//tft.drawFastHLine(24, 74, 296, color_SWOOP);
		tft.drawFastHLine(24, 74, 96, color_SWOOP);
		tft.drawFastHLine(123, 74, 97, color_SWOOP);
		tft.drawFastHLine(223, 74, 97, color_SWOOP);
		
		tft.fillRoundRect(50, -8, 40, 71, 10, ST77XX_BLACK);
		tft.fillRoundRect(50, 75, 40, 173, 10, ST77XX_BLACK);
		
		//final black lines to section off graph area
		tft.drawFastHLine(0, 141, 50, ST77XX_BLACK);
		tft.drawFastHLine(0, 142, 50, ST77XX_BLACK);
		tft.drawFastHLine(0, 143, 50, ST77XX_BLACK);
		//tft.drawFastHLine(0, 163, 50, ST77XX_BLACK);
		//tft.drawFastHLine(0, 164, 50, ST77XX_BLACK);		
		tft.drawFastHLine(0, 176, 50, ST77XX_BLACK);
		tft.drawFastHLine(0, 177, 50, ST77XX_BLACK);
		tft.drawFastHLine(0, 178, 50, ST77XX_BLACK);
		
		//color in block section of swoop left of FFT
		tft.fillRect(0, 144, 50, 32, color_HEADER);
		
		tft.setFont(&lcars15pt7b);
		//drawParamText(190, 20, "AUDIO ANALYSIS", color_TITLETEXT);
		drawParamText(185, 20, "SONIC SPECTRUM", color_TITLETEXT);
		
		tft.setFont(&lcars11pt7b);
		drawParamText(86 + GetBuffer(0), 48, "0", color_MAINTEXT);
		drawParamText(116, 48, "DECIBEL", color_LABELTEXT);
		//tft.fillRoundRect(153, 32, 17, 18, 9, color_SWOOP);
		//tft.fillRect(153, 32, 6, 17, ST77XX_BLACK);
		drawParamText(201 + GetBuffer(0), 48, "0", color_MAINTEXT);
		drawParamText(230, 48, "MAXIMUM", color_LABELTEXT3);
		//tft.fillRoundRect(286, 32, 17, 18, 9, color_SWOOP);
		//tft.fillRect(286, 32, 6, 17, ST77XX_BLACK);		
		drawParamText(38, 167, "0", ST77XX_BLACK);
		
		tft.fillRoundRect(73, 33, 16, 16, 8, color_LABELTEXT);
		tft.fillRect(83, 33, 8, 16, ST77XX_BLACK);
		tft.fillRoundRect(185, 33, 16, 16, 8, color_LABELTEXT3);
		tft.fillRect(195, 33, 8, 16, ST77XX_BLACK);
		
		//PDM.setBufferSize(2);		
		PDM.setGain(150);
		PDM.onReceive(PullMicData);		
		//first param for begin function is # channels. this is always 1 for mono		
		mbMicrophoneStarted = PDM.begin(1, MIC_SAMPLERATE);
		
		//flip this last
		mbMicrophoneActive = true;
		//tft.drawFastVLine(60, 99, 128, ST77XX_WHITE);
		//tft.drawFastVLine(61, 97, 128, ST77XX_WHITE);
		//some token graph borders??
		tft.drawFastVLine(60, 97, 128, color_LEGEND);
		tft.drawFastVLine(61, 97, 128, color_LEGEND);
		tft.drawFastHLine(60, 96, 4, color_LEGEND);
		tft.drawFastHLine(60, 225, 4, color_LEGEND);
		tft.drawFastHLine(60, 161, 4, color_LEGEND);
		tft.drawFastVLine(311, 97, 128, color_LEGEND);
		tft.drawFastVLine(312, 97, 128, color_LEGEND);
		tft.drawFastHLine(308, 96, 4, color_LEGEND);
		tft.drawFastHLine(308, 225, 4, color_LEGEND);
		
	} else {
		PDM.end();
		//need to zero out both arrays used by draw functions
		mbMicMaxRefresh = true;
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
			tft.fillRect(86, 32, 22, 18, ST77XX_BLACK);
			drawParamText(86 + GetBuffer(dbValue), 48, String(dbValue), color_MAINTEXT);
			//log values are in negative 2m or more
			//tft.fillRect(198, 32, 30, 18, ST77XX_BLACK);
			//dbValue = 20 * log10(GetPDMWave(4000) / 1500);
			//drawParamText(198, 48, String(dbValue), color_MAINTEXT);
			
			if ((dbValue > mnMaxDBValue) || mbMicMaxRefresh) {
				tft.fillRect(201, 32, 22, 18, ST77XX_BLACK);
				
				if (mnLastMicRead > 0) {
					if (!mbMicMaxRefresh) {
						mnMaxDBValue = dbValue;
						//mnMaxDBValue = 120;
					}
					drawParamText(201 + GetBuffer(mnMaxDBValue), 48, String(mnMaxDBValue), color_MAINTEXT);
				
					mbMicMaxRefresh = false;
				}
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

void ToggleThermal() {
	mbTomServoActive = false;
	
	mbButton7Flag = !mbButton7Flag;
	
	if (!mbThermalCameraStarted) {
		tft.fillRect(150, 80, 70, 40, ST77XX_BLACK);
		drawParamText(110, 169, "THERMALOPTICS OFFLINE", color_HEADER);
		return;
	}
		
	if (mbButton7Flag) {
		SetActiveNeoPixelButton(4);
		//Wire.beginTransmission(MLX90640_I2CADDR_DEFAULT);
		tft.fillScreen(ST77XX_BLACK);
		mbHomeActive = false;
		mbRGBActive = false;
		mbThermalActive = true;
		mbMicrophoneActive = false;
		mbTempActive = false;		
		mnCurrentServoGraphPoint = 0;
		mnServoLastDraw = 0;
		
		//camera data refresh rate needs to be twice as high as expected display frame rate
		//oThermalCamera.setRefreshRate(MLX90640_4_HZ);
		SetThermalClock();
		MLX90640_SetRefreshRate(mbCameraAddress, 0x04);
		DisableSound();
		
		//draw border for thermal camera visualization
		//main swoop left and right 22x30, 6x33
		
		tft.fillRoundRect(6, 6, 22, 30, 11, color_SWOOP);
		tft.fillRoundRect(6, 204, 22, 30, 11, color_SWOOP);
				
		tft.fillRect(6, 204, 22, 22, color_SWOOP);
		tft.fillRect(6, 14, 12, 22, color_SWOOP);
		
		tft.fillRoundRect(292, 6, 22, 30, 11, color_SWOOP);
		tft.fillRoundRect(292, 204, 22, 30, 11, color_SWOOP);
		
		tft.fillRect(302, 14, 12, 22, color_SWOOP);
		tft.fillRect(302, 204, 12, 22, color_SWOOP);
		
		tft.fillRoundRect(18, 9, 12, 32, 6, ST77XX_BLACK);
		tft.fillRoundRect(18, 199, 12, 32, 6, ST77XX_BLACK);
		
		tft.fillRect(16, 6, 14, 3, color_SWOOP);
		tft.fillRect(290, 6, 16, 3, color_SWOOP);
		
		tft.fillRect(16, 231, 14, 3, color_SWOOP);
		tft.fillRect(290, 231, 16, 3, color_SWOOP);
		
		tft.fillRect(12, 36, 6, 33, color_SWOOP);
		tft.fillRect(12, 171, 6, 33, color_SWOOP);
		tft.fillRect(302, 36, 6, 33, color_SWOOP);
		tft.fillRect(302, 171, 6, 33, color_SWOOP);
		
		tft.fillRoundRect(282, 9, 20, 32, 6, ST77XX_BLACK);
		tft.fillRoundRect(282, 199, 20, 32, 6, ST77XX_BLACK);
		
		//inner border middle
		tft.fillRect(12, 71, 6, 48, color_LABELTEXT2);
		tft.fillRect(12, 121, 6, 48, color_LABELTEXT2);
		tft.fillRect(302, 71, 6, 48, color_LABELTEXT2);
		tft.fillRect(302, 121, 6, 48, color_LABELTEXT2);
		
		//use loop for all spectrum color edges
		for (uint8_t j = 0; j < 7; j++) {
			int nTempY = 38 + (j * 23);
			//38, 61, 84, 107, 135, 158, 181
			if (j > 3) {
				nTempY = nTempY + 5;
			}
			//draw both rectangles of same color, 1 for each side, since Y coord already calculated
			tft.fillRect(6, nTempY, 4, (j == 3 ? 26 : 21), mnThermalCameraLabels[j]);
			tft.fillRect(310, nTempY, 4, (j == 3 ? 26 : 21), mnThermalCameraLabels[j]);
		}		
		
	} else {		
		mbThermalActive = false;
		//oThermalCamera.setRefreshRate(MLX90640_0_5_HZ);
		MLX90640_SetRefreshRate(mbCameraAddress, 0x01);
		ResetWireClock();
		GoHome();
	}
}

void RunThermal() {
	if (!mbThermalActive) return;	
	
	if (!mbThermalCameraStarted) {
		tft.fillRect(150, 80, 70, 40, ST77XX_BLACK);
		drawParamText(110, 169, "THERMALOPTICS OFFLINE", color_HEADER);
		//set value for screen blanked so this doesn't constantly get redrawn
		return;
	}
		
	//pimoroni camera "bottom" of view is section with pin holes (small metal tab on lens barrel points down)
	//use 10fps cap to restrict how often it tries to pull camera data, or this will block button press polling
	//need 2 data frames for each displayed frame, as it only pulls half the range at a time.
	if ((millis() - mnLastCameraFrame) > mnThermalCameraInterval) {		
		//iterate through full frame capture, then do 1 draw with all values
		for (byte i = 0; i < 2; i++) {
			uint16_t arrTempFrameRaw[834];
			int nStatus = MLX90640_GetFrameData(mbCameraAddress, arrTempFrameRaw);
			
			if (mfTR == 0.0) {
				//mfVdd = MLX90640_GetVdd
				mfTA = MLX90640_GetTa(arrTempFrameRaw, &moCameraParams);
				mfTR = mfTA - TA_SHIFT;		
			}
			MLX90640_CalculateTo(arrTempFrameRaw, &moCameraParams, mfCameraEmissivity, mfTR, mfarrTempFrame);		
		}
		
		
		//it's more code, but higher performance to check toggle once per frame than twice per thermal "pixel"
		if (THERMAL_CAMERA_PORTRAIT == 1) {
			//rotate this image counter clockwise 90 deg to compensate for different hardware orientation in shell
			//when camera rotated to portrait, clip viewport to square. 
			for (uint8_t nRow = 0; nRow < 24; nRow++) {	
				//need option to flip left-right of thermal camera display. cut column edges down by 4 to square off viewport
				for (uint8_t nCol = 4; nCol < 28; nCol++) {				
					float fTemp = mfarrTempFrame[nRow*32 + nCol];
					
					//clip temperature readings to defined range for color mapping
					//may want to increase color fidelity to accomodate larger range?
					fTemp = min(fTemp, MAX_CAMERA_TEMP);
					fTemp = max(fTemp, MIN_CAMERA_TEMP); 
					uint8_t nColorIndex = map(fTemp, MIN_CAMERA_TEMP, MAX_CAMERA_TEMP, 0, 279);
					nColorIndex = constrain(nColorIndex, 0, 279);
					
					//draw the pixels
					//need to flip this left/right. top/down shows correct, so instead of width * col we need width * (31-col)
					//need to swap x and y values to "rotate" camera viewport 90 degrees counter clockwise
					tft.fillRect((mnThermalPixelHeight * nRow) + mnCameraDisplayStartX, (mnThermalPixelWidth * (nCol - 4)) + mnCameraDisplayStartY, mnThermalPixelWidth, mnThermalPixelHeight, mnarrThermalDisplayColors[nColorIndex]);
				}
			}
		} else {
			for (uint8_t nRow = 0; nRow < 24; nRow++) {	
				//for (uint8_t nCol = 0; nCol < 32; nCol++) {
				for (uint8_t nCol = 0; nCol < 32; nCol++) {				
					float fTemp = mfarrTempFrame[nRow*32 + nCol];
					
					//clip temperature readings to defined range for color mapping
					//may want to increase color fidelity to accomodate larger range?
					fTemp = min(fTemp, MAX_CAMERA_TEMP);
					fTemp = max(fTemp, MIN_CAMERA_TEMP); 
					uint8_t nColorIndex = map(fTemp, MIN_CAMERA_TEMP, MAX_CAMERA_TEMP, 0, 279);
					nColorIndex = constrain(nColorIndex, 0, 279);
					
					//draw the pixels
					//need to flip this left/right. top/down shows correct, so instead of width * col we need width * (31-col)
					//tft.fillRect((mnThermalPixelWidth * nCol) + mnCameraDisplayStartX, (mnThermalPixelHeight * nRow) + mnCameraDisplayStartY, mnThermalPixelWidth, mnThermalPixelHeight, mnarrThermalDisplayColors[nColorIndex]);
					tft.fillRect((mnThermalPixelWidth * (31 - nCol)) + mnCameraDisplayStartX, (mnThermalPixelHeight * nRow) + mnCameraDisplayStartY, mnThermalPixelWidth, mnThermalPixelHeight, mnarrThermalDisplayColors[nColorIndex]);
				}
			}
		}
		
		mnLastCameraFrame = millis();		
	}
}

void PullMicData() {	
	//read() ONLY WORKS FOR SAMPLE RATE OF 16000!
	int nAvailable = PDM.available();
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

void ActivateTomServo() {
	mnRGBCooldown = 0;
	mnClimateCooldown = 0;
	mbHomeActive = false;
	mbRGBActive = false;
	mbTempActive = false;
	
	mbButton1Flag = false;
	mbButton2Flag = false;
	mbButton3Flag = false;
	
	SetActiveNeoPixelButton(5);
	
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
	
	ResetWireClock();
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