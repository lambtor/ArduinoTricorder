# ArduinoTricorder
WIP list of assets that could be used with arduino tricorder 

<strong>Home Screen: </strong>          refactored, now showing a very simplified equivalent of windows BGInfo<br/>
<strong>RGB Color Scanner: </strong>    OK<br/>
<strong>Climate Scanner:</strong>		   OK - updates bar graphs to reflect data without requiring internal loops<br/>
<strong>Left-side LED: </strong>        OK, needs multi-mode support (stacking, unified flashing, etc)<br/>
<strong>Power LED:</strong>             Functional, will either cycle colors or show color based on battery power %<br/>
<strong>Button Support:</strong>        OK - using easybutton library pulled from elsewhere on github<br/>
<strong>Microphone Spectrum <br/>
<strong>Visualization:</strong>         OK - not the most accurate thing, but has a very high refresh rate.  Uses AdafruitZero_FFT library for FFT calculations.<br/>
<strong>Thermal camera:</strong>       OK<br/>
<strong>Sound playback <br/>
via trigger on <br/>
separate board:</strong>       OK<br/>
<strong>Magnetometer for <br/>
door close to sleep:</strong>  OK<br/>
<strong>ID LED (8) color <br/>
change via <br/>
potentiometer:</strong>  in progress
<br/>
<br/>
Draw operations on the display from the arduino aren't that fast.  The FFT draw code calculates a list of bar heights, then uses fillRect() to create the bars.  At the end of the bar drawing operations, these bar heights are stored in a "previous" bar height array.  From that point on, each bar draw operation calculates the difference between "new" bar heights and "previous".  If "new" is larger than "previous", it only draws enough to extend the "previous" height to the "new" one.  If "new" is less than "previous", it draws black lines to shorten the "previous" height down to the "new" one.  This relative display update method removes the use of a full screen blank operation, and minimizes flickering.<br/>
<br/>
Refrences used:<br/>
<a href="https://github.com/adafruit/Adafruit_nRF52_Arduino/blob/master/variants/feather_nrf52840_sense/variant.h">Adafruit board pinout map</a><br/>
<a href="https://github.com/evert-arias/EasyButton">Easybutton arduino library</a><br/><br/>

![decibel demo gif](https://github.com/lambtor/ArduinoTricorder/blob/master/decibel.gif?raw=true)

![board pinout](https://github.com/lambtor/ArduinoTricorder/blob/master/tricorderV10-pinout.jpg)


