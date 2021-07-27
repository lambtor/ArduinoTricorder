# ArduinoTricorder
WIP list of assets that could be used with arduino tricorder 

Home Screen:           refactored, now showing a very simplified equivalent of windows BGInfo<br/>
RGB Color Scanner:     OK<br/>
Climate Scanner:		  OK - updates bar graphs to reflect data without requiring internal loops<br/>
Left-side LED:         OK, needs multi-mode support (stacking, unified flashing, etc)<br/>
Power LED:             Functional, will either cycle colors or show color based on battery power %<br/>
Button Support:        OK - using easybutton library pulled from elsewhere on github<br/>
Microphone Spectrum <br/>
Visualization:         OK - not the most accurate thing, but has a very high refresh rate.  Uses AdafruitZero_FFT library for FFT calculations.<br/>
<br/>
<br/>
Draw operations on the display from the arduino aren't that fast.  The FFT draw code calculates a list of bar heights, then uses fillRect() to create the bars.  At the end of the bar drawing operations, these bar heights are stored in a "previous" bar height array.  From that point on, each bar draw operation calculates the difference between "new" bar heights and "previous".  If "new" is larger than "previous", it only draws enough to extend the "previous" height to the "new" one.  If "new" is less than "previous", it draws black lines to shorten the "previous" height down to the "new" one.  This relative display update method removes the use of a full screen blank operation, and avoids screen flickering.

![decibel demo gif](https://github.com/lambtor/ArduinoTricorder/blob/master/decibel.gif?raw=true)

![board pinout](https://github.com/lambtor/ArduinoTricorder/blob/master/tricorderv10-pinout.jpg)

to do: 
sound playback via separate wav/ogg board<br/>

