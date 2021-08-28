 /**************************************************************************
 * SINDEN LIGHT GUN LED BORDER AND KEYBOARD CONTROL BOARD
 * 
 * Version: 05E - Fade added to sinden border 
 * Version: 05E - With USB Serial interface
 * Version: 05D - With Coin IO changed to gun drawer switch
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
 **************************************************************************/

#include <FastLED.h>
#include <Keyboard.h>

/*CONFIGURABLE VARIABLES*/
/*Define LED settings*/
#define LED_STRIP0_PIN      18
#define LED_STRIP1_PIN      19
 
#define STRIP0_NUM_LEDS     141
#define STRIP1_NUM_LEDS     8

#define LED_TYPE            WS2811
#define COLOR_ORDER         GRB

#define BRIGHTNESS          10
#define FRAMES_PER_SECOND   120
int LED_SW_SECONDS =        10;
int sindenBrightness =      255;
CRGB sindenCol =            CRGB(255,255,255);
int fadeStep =              3;

/*Define KB variables*/
long pressTime =            1000; // 1 second between key presses for macro commands*/
#define                     LAYOUT LAYOUT_US // currently available layouts: LAYOUT_US, LAYOUT_DE 

/* connect a button to this pin and 5V (resistor of appropriate value recommended to bring DIO pin to GND*/
#define A_PIN 2 /* ESC */
#define B_PIN 3 /* LOAD - F7 */
#define C_PIN 4 /* UP */
#define D_PIN 5 /* DOWN */
#define E_PIN 6 /* LEFT */
#define F_PIN 7 /* RIGHT */
#define G_PIN 8 /* BACKSPACE */
#define H_PIN 9 /* ENTER */
#define I_PIN 0 /* P - PAUSE - Pin RXD */
#define J_PIN 1 /* SAVE - Pin TXD */
#define K_PIN 10 /* START PLAYER 1 */
#define N_PIN 16 /* START PLAYER 2 */
#define L_PIN 14 /* GUN Drawer */
#define M_PIN 15 /* Spare */


/* CODE DRIVEN VARIABLES */ // Change not recommended
#define KEY_MENU 0xED
int val;
int n = LOW;
bool logging = false; // Enable serial logging for systematic events (set to false to only log rec'd requests)

CRGB leds0[STRIP0_NUM_LEDS];
CRGB leds1[STRIP1_NUM_LEDS];
int brightness = BRIGHTNESS;
int brightness0 = BRIGHTNESS;
int brightness1 = BRIGHTNESS;
bool LEDRotate = true; // LED Strips set to rotate through modes
bool brightReq = false; // When 'B' rec'd via serial, triggers it to then receive a brightness level (0-255)
bool swTimeReq = false; // Wgeb 'T' rec'd via serial, triggers it to then recieve tranisition time for pattern rotation

bool lastButtonState = HIGH;
bool L_State = HIGH;
int keyPressCount = 0;
String lastKey;
long keyPressMillis = millis();
bool sinden = false; // Enable Sinder border on leds1 set
bool sinden_First_Loop = true; // tracks first loop of sinden loops for fade automaion
bool sinderFadeComplete = false; // tracks if sinden LED fade up is complete

#define ARRAY_SIZE(A) (sizeof(A) / sizeof((A)[0]))

const byte numChars = 32;
char receivedChars[numChars]; // an array to store the received data
String SerialRcd = "";
boolean newData = false;

void setup() {

  delay(1500); // 3 second delay for recovery

  Serial.begin(9600);
  Serial.println("Serial commands:-"); 
  Serial.println("   T = set pattern transition time");
  Serial.println("   B = set brightness, ");
  Serial.println("   L = toggle full serial logging");
  Serial.println("   M+/- = switch pattern");
  Serial.println("   R = toggle pattern rotation");
  Serial.println("   S+/- = to enable sinder border, S to toggle");
  Serial.print("");
  Serial.println("< Arduino Button KB / LED Controller is ready >");
    
  // tell FastLED about the LED strip configuration
  FastLED.addLeds<LED_TYPE,LED_STRIP0_PIN,COLOR_ORDER>(leds0, STRIP0_NUM_LEDS).setCorrection(TypicalLEDStrip);
  FastLED.addLeds<LED_TYPE,LED_STRIP1_PIN,COLOR_ORDER>(leds1, STRIP1_NUM_LEDS).setCorrection(TypicalLEDStrip);
  
  // set master brightness control
  FastLED.setBrightness(BRIGHTNESS);
  
  Keyboard.begin();  // Initialise the library.

  /* set the button pin as input and activate the internal pullup resistor */
  pinMode(A_PIN, INPUT);
  pinMode(B_PIN, INPUT);
  pinMode(C_PIN, INPUT);
  pinMode(D_PIN, INPUT);
  pinMode(E_PIN, INPUT);
  pinMode(F_PIN, INPUT);
  pinMode(G_PIN, INPUT);
  pinMode(H_PIN, INPUT);
  pinMode(I_PIN, INPUT);
  pinMode(J_PIN, INPUT);
  pinMode(K_PIN, INPUT); 
  pinMode(L_PIN, INPUT); 
  pinMode(M_PIN, INPUT); 
  pinMode(N_PIN, INPUT); 

  /* set the internal LED pin (normally D13) as output */
  pinMode(LED_BUILTIN, OUTPUT);
}

// List of patterns to cycle through.  Each is defined as a separate function below.
typedef void (*SimplePatternList[])();
SimplePatternList gPatterns = { rainbow, rainbowWithGlitter, confetti, sinelon, juggle, bpm };

uint8_t gCurrentPatternNumber = 0; // Index number of which pattern is current
uint8_t gHue = 0; // rotating "base color" used by many of the patterns6

static const char *patternList[] = {"rainbow", "rainbowWithGlitter", "confetti", "sinelon", "juggle", "bpm"};
  
void loop() {
  // Call the current pattern function once, updating the 'leds' array
  gPatterns[gCurrentPatternNumber]();
  
  if (LEDRotate == true) {
    EVERY_N_SECONDS_I(timingObj, LED_SW_SECONDS ) { // change patterns periodically
      gCurrentPatternNumber = (gCurrentPatternNumber + 1) % ARRAY_SIZE( gPatterns); 
      if (logging == true) {
        Serial.print("LED Pattern Mode = '");
        Serial.print(patternList[gCurrentPatternNumber]);
        Serial.print("', LED Pattern Change Time = ");
        Serial.println(LED_SW_SECONDS);
      }
      timingObj.setPeriod(LED_SW_SECONDS); 
    }
  } 

  // send the 'leds' array out to the actual LED strip
  FastLED[0].showLeds(brightness0);
  
  if (sinden == true) {
    if (sinden_First_Loop) {
      fadeToBlackBy( leds1, STRIP1_NUM_LEDS, 100);
      FastLED.clear();
    }
    if (!sinderFadeComplete) Sinden_Fade_up();
   } else {
    brightness1 = brightness;
    sinden_First_Loop = true;
    sinderFadeComplete = false;
  }
  FastLED[1].showLeds(brightness1); 

  // insert a delay to keep the framerate modest
  FastLED.delay(1000/FRAMES_PER_SECOND); 

 
  // do some periodic updates
  EVERY_N_MILLISECONDS( 20 ) { gHue++; } // slowly cycle the "base color" through the rainbow
  
  int A_BUTTON = 0;
  int B_BUTTON = 0;
  int C_BUTTON = 0;
  int D_BUTTON = 0;
  int E_BUTTON = 0;
  int F_BUTTON = 0;
  int G_BUTTON = 0;
  int H_BUTTON = 0;
  int I_BUTTON = 0;
  int J_BUTTON = 0;
  int K_BUTTON = 0;
  int L_BUTTON = 0;
  int M_BUTTON = 0;
  int N_BUTTON = 0;

  /*Check if buttons are pressed (compensates for interference)*/
  for(int i=0; i<10; i++)
  {
    if (digitalRead(A_PIN) == HIGH) A_BUTTON++;
    if (digitalRead(B_PIN) == HIGH) B_BUTTON++;
    if (digitalRead(C_PIN) == HIGH) C_BUTTON++;
    if (digitalRead(D_PIN) == HIGH) D_BUTTON++;
    if (digitalRead(E_PIN) == HIGH) E_BUTTON++;
    if (digitalRead(F_PIN) == HIGH) F_BUTTON++;
    if (digitalRead(G_PIN) == HIGH) G_BUTTON++;
    if (digitalRead(H_PIN) == HIGH) H_BUTTON++;
    if (digitalRead(I_PIN) == HIGH) I_BUTTON++;
    if (digitalRead(J_PIN) == HIGH) J_BUTTON++;
    if (digitalRead(K_PIN) == HIGH) K_BUTTON++;
    if (digitalRead(L_PIN) == HIGH) L_BUTTON++;
    if (digitalRead(M_PIN) == HIGH) M_BUTTON++;
    if (digitalRead(N_PIN) == HIGH) N_BUTTON++;

    delayMicroseconds(100);
  }
  
  if (A_BUTTON > 5) {
    if (lastButtonState == HIGH) {
      if (lastKey == "A_Pin") keyPressCount++; else keyPressCount = 1;
      if (keyPressCount >= 6 && ((millis() - keyPressMillis) <= pressTime)) {
        Keyboard.press(KEY_LEFT_CTRL);
        Keyboard.press(KEY_LEFT_ALT);
        Keyboard.press(KEY_DELETE);
        delay(100);                       // Wait for the computer to register the press.
        Keyboard.releaseAll(); 
        keyPressMillis = millis(); 
        keyPressCount = 0;
        lastButtonState = LOW;
      } else {
        Keyboard.press(KEY_ESC);
        delay(50);                       // Wait for the computer to register the press.
        Keyboard.releaseAll();
        keyPressMillis = millis(); 
        lastButtonState = LOW;
      }
    }
    lastKey = "A_Pin";
  }
  else if (B_BUTTON > 5) {    //SAVE BUTTON
    if (lastButtonState == HIGH) {
      if (lastKey == "B_Pin") keyPressCount++; else keyPressCount = 1;
      if (keyPressCount >= 4 && ((millis() - keyPressMillis) <= pressTime)) {
        Keyboard.press(KEY_ESC);
        delay(150);                       // Wait for the computer to register the press.
        Keyboard.releaseAll(); 
        Keyboard.press(KEY_TAB);
        delay(50);                       // Wait for the computer to register the press.
        Keyboard.releaseAll(); 
        keyPressMillis = millis(); 
        keyPressCount = 0;
        lastButtonState = LOW;
      } else {
        Keyboard.press(KEY_F7);
        delay(50);                       // Wait for the computer to register the press.
        Keyboard.releaseAll();
        keyPressMillis = millis(); 
        lastButtonState = LOW;
      }
    }
    lastKey = "B_Pin";
  }
  else if (C_BUTTON > 5) {
    if (lastButtonState == HIGH) {
      Keyboard.press(KEY_UP_ARROW);   
      delay(50);                       // Wait for the computer to register the press.
      Keyboard.releaseAll();
      lastButtonState = LOW;
    }
    lastKey = "C_Pin";
  }
  else if (D_BUTTON > 5) {
    if (lastButtonState == HIGH) {
      Keyboard.press(KEY_DOWN_ARROW);   
      delay(50);                       // Wait for the computer to register the press.
      Keyboard.releaseAll();
      lastButtonState = LOW;
    }
    lastKey = "D_Pin";
  }
  else if (E_BUTTON > 5) {
    if (lastButtonState == HIGH) {
      Keyboard.press(KEY_LEFT_ARROW);   
      delay(50);                       // Wait for the computer to register the press.
      Keyboard.releaseAll();
      lastButtonState = LOW;
    }
    lastKey = "E_Pin";
  }
  else if (F_BUTTON > 5) {
    if (lastButtonState == HIGH) {
      Keyboard.press(KEY_RIGHT_ARROW);
      delay(50);                       // Wait for the computer to register the press.
      Keyboard.releaseAll();
      lastButtonState = LOW;
    }
    lastKey = "F_Pin";
  }
  else if (G_BUTTON > 5) {
    if (lastButtonState == HIGH) {
      Keyboard.press(KEY_BACKSPACE);   
      delay(50);                       // Wait for the computer to register the press.
      Keyboard.releaseAll();
      lastButtonState = LOW;
    }
    lastKey = "G_Pin";
  }
  else if (H_BUTTON > 5) {
    if (lastButtonState == HIGH) {
      Keyboard.press(KEY_RETURN);
      delay(100);                       // Wait for the computer to register the press.
      Keyboard.releaseAll();
      lastButtonState = LOW;
    }
    lastKey = "H_Pin";
  }
  else if (I_BUTTON > 5) {
    if (lastButtonState == HIGH) {
      Keyboard.press('p');   
      delay(50);                       // Wait for the computer to register the press.
      Keyboard.releaseAll(); 
      lastButtonState = LOW;
    }
    lastKey = "I_Pin";
  }
  else if (J_BUTTON > 5) {
    if (lastButtonState == HIGH) {
      Keyboard.press(KEY_LEFT_SHIFT);
      Keyboard.press(KEY_F7);
      delay(50);                       // Wait for the computer to register the press.
      Keyboard.releaseAll(); 
      lastButtonState = LOW;
    }
    lastKey = "J_Pin";
  }
  else if (M_BUTTON > 5) {
    if (lastButtonState == HIGH) {
      Keyboard.press('6');
      delay(50);                       // Wait for the computer to register the press.
      Keyboard.releaseAll();
      lastButtonState = LOW;
    }
    lastKey = "M_Pin";
  } else if (K_BUTTON > 5) {
    if (lastButtonState == HIGH) {
      if (lastKey == "K_Pin") keyPressCount++; else keyPressCount = 1;
      if (lastKey == "N" && N_BUTTON > 5) {
        if(brightness > 0) brightness = brightness - 5;
        brightness0 = brightness;
        brightness1 = brightness;
        FastLED.setBrightness(brightness);
      }
      if (keyPressCount >= 3 && lastKey == "K_Pin" && ((millis() - keyPressMillis) <= pressTime)) {
        manualLEDSkip("-");
      } else {
        Keyboard.press('1');
        delay(50);                       // Wait for the computer to register the press.
        Keyboard.releaseAll();
        lastButtonState = LOW; 
      }
    }
    lastKey = "K_Pin";
  } else if (N_BUTTON > 5) {
    if (lastButtonState == HIGH) {
      if (lastKey == "N_Pin") keyPressCount++; else keyPressCount = 1;
      if (lastKey == "K" && K_BUTTON > 5) {
        if(brightness < 100) brightness = brightness + 5;
        brightness0 = brightness;
        brightness1 = brightness;
        FastLED.setBrightness(brightness);
      }
      else if (keyPressCount >= 3 && lastKey == "N_Pin" && ((millis() - keyPressMillis) <= pressTime)) {
        manualLEDSkip("+");
      } else {
        Keyboard.press('2');
        delay(50);                       // Wait for the computer to register the press.
        Keyboard.releaseAll();
        lastButtonState = LOW;
      }
    }
    lastKey = "N_Pin";
  }else {
    lastButtonState = HIGH;
  }

  // Gun Drawer activate
  if (L_BUTTON > 5) { // Drawer swith closed 
    if (L_State == HIGH) {
      Keyboard.press(KEY_LEFT_ALT);
      Keyboard.press('y');
      delay(100);                       // Wait for the computer to register the press.
      Keyboard.releaseAll();
      L_State = LOW;
    }
  } else { 
    if (L_State == LOW) {
      Keyboard.press(KEY_LEFT_ALT);
      Keyboard.press('u');
      delay(100);                       // Wait for the computer to register the press.
      Keyboard.releaseAll();
      L_State = HIGH;
    }
  }

  // Serial actions
  recvWithEndMarker();
  if (newData == true) {
    if (brightReq == true) { 
      if (isValidNumber(SerialRcd) && SerialRcd.toInt() >= 0 && SerialRcd.toInt() <= 255) {
        FastLED.setBrightness(SerialRcd.toInt());
        brightReq = false;
        brightness = SerialRcd.toInt();
        Serial.print("Brightness at ");      
        Serial.println(brightness);
        brightness0 = brightness;
        brightness1 = brightness;
      } else {
        Serial.println("invalid brightness level received, must be 0-255");
        brightReq = false;
      }
    } else if (swTimeReq == true) { 
      if (isValidNumber(SerialRcd) && SerialRcd.toInt() >= 0 && SerialRcd.toInt() <= 1000) {
        LED_SW_SECONDS = SerialRcd.toInt();
        swTimeReq = false;
        Serial.print("Pattern transition time set to ");      
        Serial.print(LED_SW_SECONDS);
        Serial.println(" seconds");
        LEDRotate = true;
      } else {
        Serial.println("invalid time period received, must be 0-1000 seconds");
        swTimeReq = false;
      }
    } else if (SerialRcd == "B" || SerialRcd == "b"){
      brightReq = true;
      Serial.print("Current brightness level = ");
      if (brightness0 == brightness1) {
        Serial.println(brightness);
      } else {
        Serial.print("Strip 1: ");
        Serial.print(brightness0);
        Serial.print(", Strip 2: ");
        Serial.println(brightness1);
      }
      Serial.println("Waiting to receive brightness level...");
    } else if (SerialRcd == "T" || SerialRcd == "t") {
      Serial.print("Current LED pattern transition time = ");
      Serial.print(LED_SW_SECONDS);
      Serial.println(" seconds");
      Serial.println("Waiting to receive new pattern transition time...");
      swTimeReq = true;
    } else if (SerialRcd == "S" || SerialRcd == "s") {
      if (sinden == false) {
        Serial.println("Sinden border activated");
        sinden = true;
      } else {
        Serial.println("Sinden border de-activated");
        sinden = false;
        brightness1 = brightness0;
      }
    } else if (SerialRcd == "L" || SerialRcd == "l") {
      if (logging == false) {
        Serial.println("Full event serial logging activated");
        logging = true;
      } else {
        Serial.println("Full event serial logging de-activated");
        logging = false;
      }
    } else if (SerialRcd == "R" || SerialRcd == "r") {
      if (LEDRotate == false) {
        Serial.println("Pattern rotation activated");
        LEDRotate = true;
      } else {
        Serial.println("Pattern rotation de-activated");
        LEDRotate = false;
      }
    } else if (SerialRcd == "S+" || SerialRcd == "s+") {
      if (sinden == false) {
        Serial.println("Sinden mode activated");
        sinden = true;
      }
    } else if (SerialRcd == "S-" || SerialRcd == "s-") {
      if (sinden == true) {
        Serial.println("Sinden mode de-activated");
        sinden = false;
      }
    } else if (SerialRcd == "B-" || SerialRcd == "b-") {
      if(brightness >0) brightness = brightness - 5;
      FastLED.setBrightness(brightness);
      Serial.print("Brightness at ");      
      Serial.println(brightness);
    } else if (SerialRcd == "B+" || SerialRcd == "b+"){
      if(brightness <255) brightness = brightness + 5;
      FastLED.setBrightness(brightness);
      Serial.print("Brightness at ");      
      Serial.println(brightness);
    } else if (SerialRcd == "M+" || SerialRcd == "m+"){
      manualLEDSkip("+");
    } else if (SerialRcd == "M-" || SerialRcd == "m-"){
      manualLEDSkip("-");
    } else if (SerialRcd == "M" || SerialRcd == "m"){
      manualLEDSkip("+");
    } else {
      Serial.print("Unrecognised serial date received: ");
      Serial.println(receivedChars);
    }
    newData = false;
  }
}

void recvWithEndMarker() {
  static byte ndx = 0;
  char endMarker = '\n';
  char rc;
  
  while (Serial.available() > 0 && newData == false) {
    rc = Serial.read();
    if (rc != endMarker) {
      receivedChars[ndx] = rc;
      ndx++;
      if (ndx >= numChars) {
        ndx = numChars - 1;
      }
    } else {
      receivedChars[ndx] = '\0'; // terminate the string
      SerialRcd = String(receivedChars); 
      ndx = 0;
      newData = true;
    }
  }
}

void manualLEDSkip(String n)
{
  FastLED.setBrightness(brightness);
  if (n == "+") {
    gCurrentPatternNumber = (gCurrentPatternNumber + 1) % ARRAY_SIZE( gPatterns);
    Serial.print("LED Pattern Mode = '");
    Serial.print(patternList[gCurrentPatternNumber]);
    LEDRotate = false;
  } else {
    gCurrentPatternNumber = (gCurrentPatternNumber - 1) % ARRAY_SIZE( gPatterns);
    Serial.print("LED Pattern Mode = ");
    Serial.print(patternList[gCurrentPatternNumber]);
    LEDRotate = false;
  }
  
  if (gCurrentPatternNumber != 0 ) {
    delay (200);
  } else {
    LEDRotate = true;
    fill_solid(leds0, STRIP0_NUM_LEDS, CRGB(0,0,0));
    FastLED.show();
    delay (250);
    fill_solid(leds0, STRIP0_NUM_LEDS, CRGB(0,0,255));
    FastLED.show();
    delay (250);
    fill_solid(leds0, STRIP0_NUM_LEDS, CRGB(0,0,0));
    FastLED.show();
    delay (250);
    fill_solid(leds0, STRIP0_NUM_LEDS, CRGB(0,0,255));
    FastLED.show();
    delay (250);
  }
}

boolean isValidNumber(String str) {
  boolean isNum=false;
  for(byte i=0;i<str.length();i++) {
    isNum = isDigit(str.charAt(i));
    if(!isNum) return false;
  }
  return isNum;
}

void Sinden_Fade_up()
{
  int maxedLED = 0;
  sinden_First_Loop = false;
  for ( int i = 0; i < STRIP1_NUM_LEDS; i++ ) {
    CRGB col = leds1[i];
    if (col.r == sindenCol.r && col.g == sindenCol.g && col.b == sindenCol.b) {
      maxedLED = maxedLED + 1;
    } else {
      if(col.r + fadeStep <= sindenCol.r) {
        leds1[i].r = col.r + fadeStep;
      } else {
        leds1[i].r = sindenCol.r;
      }
      if(col.g + fadeStep<= sindenCol.g) {
        leds1[i].g = col.g + fadeStep;
      } else {
        leds1[i].g = sindenCol.g;
      }
      if(col.b + fadeStep <= sindenCol.b) {
        leds1[i].b = col.b + fadeStep;
      } else {
        leds1[i].b = sindenCol.b;
      }
    }
    //leds1[i].maximizeBrightness(255);  // 'FastLED_fade_counter' How high we want to fade up to 255 = maximum.

  }
  if (brightness1 <= (sindenBrightness - fadeStep)) {
    brightness1 = brightness1 + fadeStep;   // Increment
  } else {
    brightness1 = sindenBrightness;
  }

  
  //FastLED[1].showLeds(brightness1); 
  if (maxedLED >= STRIP1_NUM_LEDS -1 && brightness1 >= sindenBrightness) {
    sinderFadeComplete = true;
    Serial.println ("Sinden border fade in complete");
  }
}

void rainbow() 
{
  // FastLED's built-in rainbow generator
  fill_rainbow(leds0, STRIP0_NUM_LEDS, gHue, 7);
  if (!sinden) fill_rainbow(leds1, STRIP1_NUM_LEDS, gHue, 7);
  
}

void rainbowWithGlitter() 
{
  // built-in FastLED rainbow, plus some random sparkly glitter
  rainbow();
  addGlitter(80);
}

void addGlitter( fract8 chanceOfGlitter) 
{
  if( random8() < chanceOfGlitter) {
    leds0[ random16(STRIP0_NUM_LEDS) ] += CRGB::White;
    if (!sinden) leds1[ random16(STRIP1_NUM_LEDS) ] += CRGB::White;
  }
}

void confetti() 
{
  // random colored speckles that blink in and fade smoothly
  fadeToBlackBy( leds0, STRIP0_NUM_LEDS, 10);
  if (!sinden) fadeToBlackBy( leds1, STRIP1_NUM_LEDS, 10);
  int pos0 = random16(STRIP0_NUM_LEDS);
  int pos1 = random16(STRIP1_NUM_LEDS);
  leds0[pos0] += CHSV( gHue + random8(64), 200, 255);
  if (!sinden) leds1[pos1] += CHSV( gHue + random8(64), 200, 255);
}

void sinelon()
{
  // a colored dot sweeping back and forth, with fading trails
  fadeToBlackBy( leds0, STRIP0_NUM_LEDS, 20);
  if (!sinden) fadeToBlackBy( leds1, STRIP1_NUM_LEDS, 20);
  int pos0 = beatsin16( 13, 0, STRIP0_NUM_LEDS-1 );
  int pos1 = beatsin16( 13, 0, STRIP1_NUM_LEDS-1 );
  leds0[pos0] += CHSV( gHue, 255, 192);
  if (!sinden) leds1[pos1] += CHSV( gHue, 255, 192);
}

void bpm()
{
  // colored stripes pulsing at a defined Beats-Per-Minute (BPM)
  uint8_t BeatsPerMinute = 62;
  CRGBPalette16 palette = PartyColors_p;
  uint8_t beat = beatsin8( BeatsPerMinute, 64, 255);
  for( int i = 0; i < STRIP0_NUM_LEDS; i++) { //9948
    leds0[i] = ColorFromPalette(palette, gHue+(i*2), beat-gHue+(i*10));
  }
  if (!sinden) for( int i = 0; i < STRIP1_NUM_LEDS; i++) { //9948
    leds1[i] = ColorFromPalette(palette, gHue+(i*2), beat-gHue+(i*10));
  }
}

void juggle() {
  // eight colored dots, weaving in and out of sync with each other
  fadeToBlackBy( leds0, STRIP0_NUM_LEDS, 20);
  if (!sinden) fadeToBlackBy( leds1, STRIP1_NUM_LEDS, 20);
  byte dothue = 0;
  for( int i = 0; i < 8; i++) {
    leds0[beatsin16( i+7, 0, STRIP0_NUM_LEDS-1 )] |= CHSV(dothue, 200, 255);
    dothue += 32;
  }
  if (!sinden) for( int i = 0; i < 8; i++) {
    leds1[beatsin16( i+7, 0, STRIP1_NUM_LEDS-1 )] |= CHSV(dothue, 200, 255);
    dothue += 32;
  }
} 
