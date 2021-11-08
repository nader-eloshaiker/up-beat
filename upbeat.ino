#include <SPI.h>
#include <Wire.h>


// OLED Config
#include <Adafruit_SH110X.h>
#include <Adafruit_GFX.h>

#define i2c_Address 0x3c
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 128 // OLED display height, in pixels
#define OLED_RESET -1   //   QT-PY / XIAO

Adafruit_SH1107 display = Adafruit_SH1107(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);


// Sensor Config
#include "MAX30105.h"
#include "heartRate.h"

MAX30105 particleSensor;

const byte RATE_SIZE = 4; //Increase this for more averaging. 4 is good.
byte rates[RATE_SIZE]; //Array of heart rates
byte rateSpot = 0;
long lastBeat = 0; //Time at which the last beat occurred
long HEART_RATE_DETECTION = 50000;

float beatsPerMinute;
int beatAvg;
int displayBeatAvg = -1;


// Bitmaps
#include "UpBeatLogo.h"
#include "UpBeatHeartLogo.h"


void setup()
{
  Serial.begin(115200);

  display.begin(i2c_Address, true);
  display.display();
  delay(2000);
  display.clearDisplay();
  display.display();

  
  Serial.println("UpBeat...");

  // Initialize sensor
  if (!particleSensor.begin(Wire, I2C_SPEED_FAST)) //Use default I2C port, 400kHz speed
  {
    Serial.println("DEBUG: MAX30105 was not found. Please check wiring/power. ");
    while (1);
  }

  particleSensor.setup(); //Configure sensor with default settings
  particleSensor.setPulseAmplitudeRed(0x09); //Turn Red LED to low to indicate sensor is running
  particleSensor.setPulseAmplitudeGreen(0); //Turn off Green LED
}

void loop()
{
  long irValue = particleSensor.getIR();

  if (checkForBeat(irValue) == true)
  {
    //We sensed a beat!
    long delta = millis() - lastBeat;
    lastBeat = millis();

    beatsPerMinute = 60 / (delta / 1000.0);
    Serial.print("DEBUG: beatsPerMinute - ");
    Serial.println(beatsPerMinute);


    // count everything for demo
    // if (beatsPerMinute > 20 && beatsPerMinute < 255) {
      rates[rateSpot++] = (byte)beatsPerMinute; //Store this reading in the array
      rateSpot %= RATE_SIZE; //Wrap variable

      //Take average of readings
      beatAvg = 0;
      for (byte x = 0 ; x < RATE_SIZE ; x++)
        beatAvg += rates[x];
      beatAvg /= RATE_SIZE;
    // }
  }

  // quickly detect if it has lost contact with user 
  if (irValue < HEART_RATE_DETECTION) {
    beatAvg = 0;
  }

  // don't bother printing hte same thing on the screen if it hasn't changed
  if (displayBeatAvg == beatAvg) return;

  if (irValue < HEART_RATE_DETECTION) {
      display.clearDisplay();
      display.drawBitmap(19, 0, UpBeatLogo_bits, UpBeatLogo_width, UpBeatLogo_height, SH110X_WHITE);
      display.display();
      delay(10);
      Serial.println("DEBUG: No Heart");
  } else {
      display.clearDisplay();
      display.drawBitmap(19, 0, UpBeatHeartLogo_bits, UpBeatHeartLogo_width, UpBeatHeartLogo_height, SH110X_WHITE);
      display.setTextColor(SH110X_WHITE);
      display.setTextSize(3);
      display.setCursor(0,100);
      display.print("BPM");             
      display.setCursor(70,100);   
      display.print(beatAvg);
      display.display();
      delay(10);
      Serial.print("DEBUG: IR=");
      Serial.print(irValue);
      Serial.print(", BPM=");
      Serial.print(beatsPerMinute,0);
      Serial.print(", Avg BPM=");
      Serial.println(beatAvg);
   }

   lastBeat = millis();
   displayBeatAvg = beatAvg;
}
