# Sinden-Light-Gun-LED-Border-Button-Controller

This was developed for a custom arcade machine featuring sinden light guns. 

It is dual purpose code for the  Arduino Micro. It controls two individually addressable LED strips (one for a light border to surround the screen, the other for cabinet lighting). 
It is controllable via serial interface through USB connection (compatible with autohotkey/eventghost). 
Additionally it provides keyboard functionality for arcade cabinet buttons across the remaining free DIO pins.

This is a template shared as a basis for individuals to modify, the code is dirty in places, but functional. Please make pull requests if you improve the code and i'll endeavour to review and update.

/**************************************************************************
* SINDEN LIGHT GUN LED BORDER AND KEYBOARD CONTROL BOARD
* 
* Coded for use with two LED strips, Strip 0 is for general use, Strip 1 is for the display light border 
* Coded for use on an Audrino Micro, the Audrino must have USB controller (leonardos or equivalents)
* Serial commands can be sent via autohotkey/eventghost etc... options include:-  
  *    T = set pattern transition time");
  *    B = set brightness, ");
  *    L = toggle full serial logging");
  *    M+/- = switch pattern");
  *    R = toggle pattern rotation");
  *    S+/- = to enable sinder border, S to toggle");
*    
* NOTE: Serial commands must be terminated with "\n"
* 
* Main reference/source sites:- 
* https://github.com/FastLED/FastLED/wiki/Multiple-Controller-Examples
* https://www.arduino.cc/en/Reference/KeyboardModifiers 
* http://www.asciitable.com/
* 
**************************************************************************/
