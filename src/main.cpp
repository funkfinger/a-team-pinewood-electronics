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

// Music Maker Featherwing....
#define VS1053_RESET -1 // VS1053 reset pin (not used!)
// changed for m0 feather
#define VS1053_CS 6   // VS1053 chip select pin (output)
#define VS1053_DCS 10 // VS1053 Data/command select pin (output)
#define CARDCS 5      // Card chip select pin
#define VS1053_DREQ 9 // VS1053 Data request, ideally an Interrupt pin

// NeoPixel...
#define NEOPIX_LED_PIN 8
#define NEOPIX_LED_COUNT 1

Adafruit_MMA8451 mma = Adafruit_MMA8451();
Adafruit_VS1053_FilePlayer musicPlayer =
    Adafruit_VS1053_FilePlayer(VS1053_RESET, VS1053_CS, VS1053_DCS, VS1053_DREQ, CARDCS);
Adafruit_NeoPixel strip(NEOPIX_LED_COUNT, NEOPIX_LED_PIN, NEO_GRB + NEO_KHZ800);

// Buffer will be 16 samples long, it will take 16 * sizeof(float) = 64 bytes of RAM
MovingAverageFloat<16> angle;
MovingAverageFloat<16> speedDown;

// state machine values...
enum State_enum
{
  NOT_RACING,
  AT_STARTING_LINE,
  MOVING,
  STOPPED,
  GETTING_PICKED_UP
};

uint8_t state = NOT_RACING;

void errorMode();
void startAccelerometer();
void startNeoPixel();
void startMusicMaker();
void startSdCard();
void setState();
void runStateMachine();
void updateAccelerometer();
bool onTrack();
bool movingDown();
bool stopped();
bool jostling();
bool still();

#define STARTING_LINE_ARRAY_SIZE 5
const char *startingLineFiles[] = {"start0.ogg", "start1.ogg", "start2.ogg", "start3.ogg", "start4.ogg", "start5.ogg"};
uint8_t startingLineFilePosition = 0;

#define MOVING_ARRAY_SIZE 1
const char *movingFiles[] = {"move0.ogg", "move2.ogg"};
uint8_t movingFilePosition = 0;

#define STOPPED_ARRAY_SIZE 0
const char *stoppedFiles[] = {"stop0.ogg"};
uint8_t stoppedFilePosition = 0;

#define GETTING_PICKED_UP_ARRAY_SIZE 1
const char *gettingPickedUpFiles[] = {"pick1.ogg", "pick0.ogg"};
uint8_t gettingPickedUpFilePosition = 0;

void setup()
{
  Serial.begin(115200);
  Serial.println(F("starting up...."));
  startAccelerometer();
  startNeoPixel();
  startSdCard();
  startMusicMaker();
}

uint8_t racing = false;

bool moving()
{
  sensors_event_t event;
  mma.getEvent(&event);
  return (false);
}

uint8_t count = 0;
uint32_t red = strip.Color(50, 0, 0);
uint32_t blue = strip.Color(0, 0, 50);
uint32_t green = strip.Color(0, 50, 0);
uint32_t orange = strip.Color(50, 20, 0);
uint32_t purple = strip.Color(20, 0, 20);

unsigned long previousMillis = 0;
uint16_t checkStateInterval = 200;

void loop()
{
  unsigned long currentMillis = millis();
  updateAccelerometer();
  if ((unsigned long)(currentMillis - previousMillis) >= checkStateInterval)
  {
    setState();

    // state = AT_STARTING_LINE;
    runStateMachine();
    previousMillis = currentMillis;
  }
}

void errorMode()
{
  while (1)
  {
    strip.setPixelColor(0, strip.Color(50, 0, 0));
    strip.show();
    delay(250);
    strip.setPixelColor(0, strip.Color(0, 0, 0));
    strip.show();
    delay(250);
  }
}

void startAccelerometer()
{
  if (!mma.begin())
  {
    Serial.println("Couldnt start");
    errorMode();
  }
  Serial.println("MMA8451 found!");
  mma.setRange(MMA8451_RANGE_2_G);
}

void startNeoPixel()
{
  strip.begin();
  strip.clear();
  strip.setPixelColor(0, strip.Color(0, 10, 0));
  strip.show();
  delay(1000);
  strip.clear();
  strip.show();
}

void startMusicMaker()
{
  if (!musicPlayer.begin())
  {
    Serial.println(F("Couldn't find VS1053, do you have the right pins defined?"));
    errorMode();
  }
  musicPlayer.sineTest(0x44, 500);
  Serial.println(F("VS1053 found"));
  // Set volume for left, right channels. lower numbers == louder volume!
  musicPlayer.setVolume(0, 0);
  musicPlayer.useInterrupt(VS1053_FILEPLAYER_PIN_INT); // DREQ int
}

void startSdCard()
{
  if (!SD.begin(CARDCS))
  {
    Serial.println(F("SD failed, or not present"));
    errorMode();
  }
  Serial.println("SD OK!");
}

void updateAccelerometer()
{
  sensors_event_t event;
  mma.getEvent(&event);
  angle.add(event.acceleration.y);
  speedDown.add(event.acceleration.z);
}

void runStateMachine()
{
  if (musicPlayer.stopped())
  {
    switch (state)
    {
    case AT_STARTING_LINE:
      Serial.println("AT_STARTING_LINE");
      startingLineFilePosition++;
      if (startingLineFilePosition > STARTING_LINE_ARRAY_SIZE)
        startingLineFilePosition = 0;
      musicPlayer.startPlayingFile(startingLineFiles[startingLineFilePosition]);
      break;
    default:
      Serial.println("STATE CASE NOT SET!");
    }
  }
}

#define REQUIRED_COUNTS_ON_TRACK 5
uint8_t countOnTrack = 0;
bool currentlyOnTrack = false;

bool onTrack()
{
  float val = angle.get();
  Serial.print("y:");
  Serial.println(val);
  if (((abs(val) > 1) & ((abs(val) < 8)))) // approxomate the angle i expect the track to start with
  {
    if (countOnTrack >= REQUIRED_COUNTS_ON_TRACK)
    {
      racing = true;
      return true;
    }
    else
    {
      countOnTrack++;
    }
  }
  else
  {
    countOnTrack = 0;
  }
  return false;
}

bool movingDown()
{
  return false;
}

bool stopped()
{
  return false;
}

bool jostling()
{
  return false;
}

bool still()
{
  return false;
}

void setState()
{
  if (racing)
  {
    switch (state)
    {
    case AT_STARTING_LINE:
      if (onTrack())
      {
        if (movingDown())
          state = MOVING;
      }
      else
      {
        racing = false;
        state = NOT_RACING;
      }
      break;
    case MOVING:
      if (stopped())
        state = STOPPED;
      break;
    case STOPPED:
      if (jostling())
        state = GETTING_PICKED_UP;
      break;
    case GETTING_PICKED_UP:
      if (still())
        state = NOT_RACING;
      racing = false;
      break;
    }
  }
  else
  {
    if (onTrack())
    {
      state = AT_STARTING_LINE;
      racing = true;
    }
  }
}
