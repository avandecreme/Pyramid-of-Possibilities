#include "OctoWS2811.h"
#include "Adafruit_NeoPixel.h"
#include "Accelerometer.h"
#include "RNLights.h"
#include "RNLightsOctoWS2811.h"
#include "hsv2rgb.h"

#define FULL_STRIP 1

#if FULL_STRIP
#define LEDs 221
#define FIRST_LED 10
#define LAST_LED 230
#else
#define LEDs 60 
#define FIRST_LED 0
#define LAST_LED 59
#endif


const int ledsPerStrip = LAST_LED;

DMAMEM uint8_t displayMemory[ledsPerStrip*24];
uint8_t drawingMemory[ledsPerStrip*24];

OctoWS2811 leds(ledsPerStrip, displayMemory, drawingMemory);

RNLightsOctoWS2811 lights(leds, drawingMemory, FIRST_LED);

void setup() {
  delay(1000);
  Serial.begin(115200);
  Serial.println("Started");
  Serial.println(leds.numPixels());
  Serial.println(lights.getNumPixels());

}

void loop() {

  lights.reset();
  uint8_t baseHue = ( millis() / 10) % 256;
  int  i = lights.normalize(millis()/20);
  lights.setPixelHSV(i, baseHue, 255, 255);

  //  lights.setPixelColor(lights.normalize(millis()/20), 0, 255, 0);  
  //  for(int i = 0; i < lights.getNumPixels(); i++) 
  //   lights.setPixelHSV(i, baseHue + i*256 / lights.getNumPixels(), 255, 255);
  lights.show();
  delay(10);
  Serial.println(millis());
}


