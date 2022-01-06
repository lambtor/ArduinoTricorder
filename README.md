# ArduinoTricorder
WIP: list of digital assets that could be used with arduino tricorder 

<ul>
<li><strong>Home Screen: </strong>          showing a very simplified equivalent of windows BGInfo</li>
<li><strong>RGB Color Scanner: </strong>    OK</li>
<li><strong>Climate Scanner:</strong>		   OK - updates bar graphs to reflect data without requiring internal loops</li>
<li><strong>Left-side LED: </strong>        OK, stacks on color scanner mode</li>
<li><strong>Power LED:</strong>             Functional, will either cycle colors or show color based on battery power %</li>
<li><strong>Button Support:</strong>        OK - using easybutton library pulled from elsewhere on github</li>
<li><strong>Microphone Spectrum <br/>
Visualization:</strong>         OK - not the most accurate thing, but has a very high refresh rate.  Uses AdafruitZero_FFT library for FFT calculations.</li>
<li><strong>Thermal camera:</strong>       OK</li>
<li><strong>Sound playback <br/>
via trigger on <br/>
separate board:</strong>       OK</li>
<li><strong>Magnetometer for <br/>
door close to sleep:</strong>  OK</li>
<li><strong>ID LED (8) color <br/>
change via <br/>
potentiometer:</strong>  OK</li> 
	<li><strong>Neopixels above buttons:</strong>	OK - set #define NEOPIXEL_LED_COUNT 3 if you only have PWR / EMRG / ID wired up </li>
	<li><strong>Tom Servo Mode:</strong> 	OK - repeating, "canned" animation</li>
  </ul>
<br/>
Draw operations on the display from the arduino aren't that fast.  The FFT draw code calculates a list of bar heights, then uses fillRect() to create the bars.  At the end of the bar drawing operations, these bar heights are stored in a "previous" bar height array.  From that point on, each bar draw operation calculates the difference between "new" bar heights and "previous".  If "new" is larger than "previous", it only draws enough to extend the "previous" height to the "new" one.  If "new" is less than "previous", it draws black lines to shorten the "previous" height down to the "new" one.  This relative display update method removes the use of a full screen blank operation, and minimizes flickering.<br/>
<br/>
Refrences used:<br/>
<a href="https://github.com/adafruit/Adafruit_nRF52_Arduino/blob/master/variants/feather_nrf52840_sense/variant.h">Adafruit board pinout map</a><br/>
<a href="https://github.com/evert-arias/EasyButton">Easybutton arduino library</a><br/><br/>

![decibel demo gif](https://github.com/lambtor/ArduinoTricorder/blob/master/decibel.gif?raw=true)

![board pinout](https://github.com/lambtor/ArduinoTricorder/blob/master/tricorderV10-pinoutLT.png)

<br/>
Reference of all parts used in build demoed on youtube:
![all_parts](https://github.com/lambtor/ArduinoTricorder/blob/master/AllParts.JPG)
