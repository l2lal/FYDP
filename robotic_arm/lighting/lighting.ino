// NeoPixel Ring simple sketch (c) 2013 Shae Erisson
// released under the GPLv3 license to match the rest of the AdaFruit NeoPixel library

#include <Adafruit_NeoPixel.h>
#ifdef __AVR__
  #include <avr/power.h>
#endif

// Which pin on the Arduino is connected to the NeoPixels?
// On a Trinket or Gemma we suggest changing this to 1
#define PIN            6

// How many NeoPixels are attached to the Arduino?
#define NUMPIXELS      16

// When we setup the NeoPixel library, we tell it how many pixels, and which pin to use to send signals.
// Note that for older NeoPixel strips you might need to change the third parameter--see the strandtest
// example for more information on possible values.
Adafruit_NeoPixel pixels = Adafruit_NeoPixel(NUMPIXELS, PIN, NEO_GRB + NEO_KHZ800);

int delayval = 500; // delay for half a second

void setup() {
  // This is for Trinket 5V 16MHz, you can remove these three lines if you are not using a Trinket
#if defined (__AVR_ATtiny85__)
  if (F_CPU == 16000000) clock_prescale_set(clock_div_1);
#endif
  // End of trinket special code
  pixels.begin(); // This initializes the NeoPixel library.
  Serial.begin(9600);
}

enum state{
  HOME = 0,
  PRESNAP = 1,
  SNAPPED = 2
};

const int Amp = 5;
const float omega = 0.5 * 2* 3.142;
const float offset = 7;
state current_state = HOME;
//TODO add checksum
void loop() {
  static int color = 10;

  if (Serial.available()){
    while(Serial.available() > 1) {
      Serial.read();
    }
    char rec = Serial.read();
    Serial.println(rec);
    //Serial.read();
    switch(rec) {
      case '0':
        current_state = HOME;
        Serial.println("HOME");
        break;
      case '1':
        current_state = PRESNAP;
        Serial.println("PRESNAP");
        break;
      case '2':
        current_state = SNAPPED;
        Serial.println("SNAPPED");
        break;
    } 
  }

  if (current_state == HOME) {
      float t = millis()/1000.0;
      //run the pulse routine
      color = Amp*sin(omega * t) + offset;
      for(int i=0;i<NUMPIXELS;i++){
        pixels.setPixelColor(i, pixels.Color(color,color,color)); // Moderately bright green color.
        pixels.show(); // This sends the updated pixel color to the hardware.
      }
  } else if (current_state == PRESNAP) {

      color = 10;
      for(int i = 0;i<NUMPIXELS;i++){
        pixels.setPixelColor(i, pixels.Color(color,color,color)); // Moderately bright green color.
        pixels.show(); // This sends the updated pixel color to the hardware.
      }
  } else if (current_state == SNAPPED) {
      int count = 10;
      color = 150;
      while(--count) {
        for(int i = 0;i<NUMPIXELS;i++){
          pixels.setPixelColor(i, pixels.Color(color,color,color)); // Moderately bright green color.
          pixels.show(); // This sends the updated pixel color to the hardware.
        }
        color /= 2;
        delay(8);
      }
      Serial.println(color);
      current_state = PRESNAP;
  }
}
