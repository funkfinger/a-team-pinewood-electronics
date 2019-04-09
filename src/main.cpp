#include <Arduino.h>
#include <ArduinoLowPower.h>

// include SPI, MP3 and SD libraries
#include <SPI.h>
#include <Adafruit_VS1053.h>
#include <SD.h>

// for accellerometer...
#include <Wire.h>
#include <Adafruit_MMA8451.h>
#include <Adafruit_Sensor.h>

// for the onboard neopixel
#include <Adafruit_NeoPixel.h>

#include <MovingAverageFloat.h>

Adafruit_MMA8451 mma = Adafruit_MMA8451();


// These are the pins used
#define VS1053_RESET -1 // VS1053 reset pin (not used!)

// changed for m0 feather
#define VS1053_CS 6   // VS1053 chip select pin (output)
#define VS1053_DCS 10 // VS1053 Data/command select pin (output)
#define CARDCS 5      // Card chip select pin
#define VS1053_DREQ 9 // VS1053 Data request, ideally an Interrupt pin

Adafruit_VS1053_FilePlayer musicPlayer =
    Adafruit_VS1053_FilePlayer(VS1053_RESET, VS1053_CS, VS1053_DCS, VS1053_DREQ, CARDCS);


#define LED_PIN 8
#define LED_COUNT 1
Adafruit_NeoPixel strip(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);

// Buffer will be 16 samples long, it will take 16 * sizeof(float) = 64 bytes of RAM
MovingAverageFloat <16> filter;

void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println("\n\nA-Team Van Pinewood Derby code");

  delay(3000);
  if (! mma.begin()) {
    Serial.println("Couldnt start");
    // while (1);
  }
  Serial.println("MMA8451 found!");
  mma.setRange(MMA8451_RANGE_2_G);

  if (!musicPlayer.begin()) { // initialise the music player
    Serial.println(F("Couldn't find VS1053, do you have the right pins defined?"));
    // while (1); // hang....
  }

  delay(100);
  Serial.println(F("VS1053 found"));

  musicPlayer.sineTest(0x44, 500); // Make a tone to indicate VS1053 is working

  if (!SD.begin(CARDCS)) {
    Serial.println(F("SD failed, or not present"));
    // while (1); // don't do anything more
  }
  Serial.println("SD OK!");

  strip.begin();
  strip.clear();
  strip.setPixelColor(0, strip.Color(0, 150, 0));
  strip.show();
  delay(1000);

  // Set volume for left, right channels. lower numbers == louder volume!
  musicPlayer.setVolume(0, 0);
  musicPlayer.useInterrupt(VS1053_FILEPLAYER_PIN_INT); // DREQ int
  delay(100);
  Serial.println(F("Playing full track 001"));
  musicPlayer.startPlayingFile("2.wav");
  delay(500);

}

uint8_t countOnTrack = 0;
#define REQUIRED_COUNTS_ON_TRACK 10
bool onTrack() {
  sensors_event_t event;
  mma.getEvent(&event);
  float val = event.acceleration.x;
  Serial.print("x:"); Serial.println(val);
  if(((val > 1) | ((val < -1)))) {
    if(countOnTrack >= REQUIRED_COUNTS_ON_TRACK) {
      return true;
    } else {
       countOnTrack++;
    }
  } else {
    countOnTrack = 0;
  }
  return false;
}

bool moving() {
  sensors_event_t event;
  mma.getEvent(&event);
  return(false);
}

uint8_t count = 0;
uint32_t red =  strip.Color(50, 0, 0);
uint32_t blue =  strip.Color(0, 0, 50);
uint32_t green =  strip.Color(0, 50, 0);
uint32_t orange = strip.Color(50, 20, 0);
uint32_t purple = strip.Color(20, 0, 20);

unsigned long previousMillis=0;
uint16_t led_interval = 1000;
uint16_t position_interval = 800;

void loop() {
  unsigned long currentMillis = millis();
  if(moving()) {
    strip.setPixelColor(0, orange);
    strip.show();
  } else {
    if ((unsigned long)(currentMillis - previousMillis) >= position_interval) {
      if(onTrack()) {
        strip.setPixelColor(0, green);
      } else {
        strip.setPixelColor(0, purple);
      }
      strip.show();
      previousMillis = currentMillis;
    }
    // if ((unsigned long)(currentMillis - previousMillis) >= led_interval) {
    //   if(strip.getPixelColor(0) == red) {
    //     strip.setPixelColor(0, blue);
    //   } else {
    //     strip.setPixelColor(0, red);
    //   }
    //   strip.show();
    //   previousMillis = currentMillis;
    // }
  }
}