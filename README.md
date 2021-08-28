# Sinden-Light-Gun-LED-Border-Button-Controller

This was developed for a custom arcade machine featuring sinden light guns. 

It is dual purpose code for the  Arduino Micro. It controls two individually addressable LED strips (one for a light border to surround the screen, the other for cabinet lighting). 
It is controllable via serial interface through USB connection (compatible with autohotkey/eventghost). 
Additionally it provides keyboard functionality for arcade cabinet buttons across the remaining free DIO pins.

The light border should ideally use the 144 LED/Meter strip lighting behind a diffuser, however succcess has been had with 60 LED/Meter strip without diffuser if you are on a budget.

4:3 Games can be played at native resolution (without stretching) by adjusting the Sinden SW Cursor Offset tab settings to: X Offset:-012.5, X Ratio:1.25, Y Offset:0, Y Ratio:01.
You can automate the 4:3 configuration loading via lanuchbox (or other), by having multiple installs of the sinden SW which are closed and run by your game lanucher.

This is a template shared as a basis for individuals to modify, the code is dirty in places, but functional. Please make pull requests if you improve the code and i'll endeavour to review and update.

/**************************************************************************
* SINDEN LIGHT GUN LED BORDER AND KEYBOARD CONTROL BOARD
* 
* Coded for use with two LED strips, Strip 0 is for general use, Strip 1 is for the display light border 
* Coded for use on an Audrino Micro, the Audrino must have USB controller (leonardos or equivalents)
* Serial commands can be sent via autohotkey/eventghost etc... options include:-  
  *    T = set pattern transition time
  *    B = set brightness
  *    L = toggle full serial logging
  *    M+/- = switch pattern
  *    R = toggle pattern rotation
  *    S+/- = to enable sinder border, S to toggle
*    
* NOTE: Serial commands must be terminated with "\n"
* 
* Main reference/source sites:- 
* https://github.com/FastLED/FastLED/wiki/Multiple-Controller-Examples
* https://www.arduino.cc/en/Reference/KeyboardModifiers 
* http://www.asciitable.com/
* 
**************************************************************************/
