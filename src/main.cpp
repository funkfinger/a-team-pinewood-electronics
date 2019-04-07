#include <Arduino.h>

// include SPI, MP3 and SD libraries
#include <SPI.h>
#include <Adafruit_VS1053.h>
#include <SD.h>

// These are the pins used
#define VS1053_RESET -1 // VS1053 reset pin (not used!)

// changed for m0 feather
#define VS1053_CS 6   // VS1053 chip select pin (output)
#define VS1053_DCS 10 // VS1053 Data/command select pin (output)
#define CARDCS 5      // Card chip select pin
#define VS1053_DREQ 9 // VS1053 Data request, ideally an Interrupt pin

Adafruit_VS1053_FilePlayer musicPlayer =
    Adafruit_VS1053_FilePlayer(VS1053_RESET, VS1053_CS, VS1053_DCS, VS1053_DREQ, CARDCS);

/// File listing helper
void printDirectory(File dir, int numTabs)
{
  while (true)
  {

    File entry = dir.openNextFile();
    if (!entry)
    {
      // no more files
      //Serial.println("**nomorefiles**");
      break;
    }
    for (uint8_t i = 0; i < numTabs; i++)
    {
      Serial.print('\t');
    }
    Serial.print(entry.name());
    if (entry.isDirectory())
    {
      Serial.println("/");
      printDirectory(entry, numTabs + 1);
    }
    else
    {
      // files have sizes, directories do not
      Serial.print("\t\t");
      Serial.println(entry.size(), DEC);
    }
    entry.close();
  }
}

void setup()
{
  Serial.begin(115200);

  // if you're using Bluefruit or LoRa/RFM Feather, disable the radio module
  //pinMode(8, INPUT_PULLUP);

  // Wait for serial port to be opened, remove this line for 'standalone' operation
  while (!Serial)
  {
    delay(1);
  }
  delay(500);
  Serial.println("\n\nAdafruit VS1053 Feather Test");

  if (!musicPlayer.begin())
  { // initialise the music player
    Serial.println(F("Couldn't find VS1053, do you have the right pins defined?"));
    while (1)
      ;
  }

  Serial.println(F("VS1053 found"));

  musicPlayer.sineTest(0x44, 500); // Make a tone to indicate VS1053 is working

  if (!SD.begin(CARDCS))
  {
    Serial.println(F("SD failed, or not present"));
    while (1)
      ; // don't do anything more
  }
  Serial.println("SD OK!");

  // list files
  printDirectory(SD.open("/"), 0);

  // Set volume for left, right channels. lower numbers == louder volume!
  musicPlayer.setVolume(0, 0);

  musicPlayer.useInterrupt(VS1053_FILEPLAYER_PIN_INT); // DREQ int

  // Play a file in the background, REQUIRES interrupts!
  Serial.println(F("Playing full track 001"));
  musicPlayer.playFullFile("1.wav");

  Serial.println(F("Playing track 002"));
  musicPlayer.startPlayingFile("2.wav");
}

void loop()
{
  Serial.print(".");
  // File is playing in the background
  if (musicPlayer.stopped())
  {
    Serial.println("Done playing music");
    delay(1000);
  // Play a file in the background, REQUIRES interrupts!
  Serial.println(F("Playing full track 001"));
  musicPlayer.playFullFile("1.wav");

  Serial.println(F("Playing track 002"));
  musicPlayer.startPlayingFile("2.wav");
    // while (1)
    // {
    //   delay(10); // we're done! do nothing...
    // }
  }
  if (Serial.available())
  {
    char c = Serial.read();

    // if we get an 's' on the serial console, stop!
    if (c == 's')
    {
      musicPlayer.stopPlaying();
    }

    // if we get an 'p' on the serial console, pause/unpause!
    if (c == 'p')
    {
      if (!musicPlayer.paused())
      {
        Serial.println("Paused");
        musicPlayer.pausePlaying(true);
      }
      else
      {
        Serial.println("Resumed");
        musicPlayer.pausePlaying(false);
      }
    }
  }
  delay(100);
}