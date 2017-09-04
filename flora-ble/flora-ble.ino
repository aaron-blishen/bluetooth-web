
#include <Adafruit_ATParser.h>
#include <Adafruit_BLE.h>
#include <Adafruit_BLEBattery.h>
#include <Adafruit_BLEEddystone.h>
#include <Adafruit_BLEGatt.h>
#include <Adafruit_BLEMIDI.h>
#include <Adafruit_BluefruitLE_SPI.h>
#include <Adafruit_BluefruitLE_UART.h>
#include "BluefruitConfig.h"

#include <Adafruit_Sensor.h>
#include <Adafruit_LSM303_U.h>

/* Assign a unique ID to this sensor at the same time */
Adafruit_LSM303_Accel_Unified accel = Adafruit_LSM303_Accel_Unified(54321);
Adafruit_LSM303_Mag_Unified mag = Adafruit_LSM303_Mag_Unified(54320);


Adafruit_BluefruitLE_UART ble(BLUEFRUIT_HWSERIAL_NAME, BLUEFRUIT_UART_MODE_PIN);

#define FACTORYRESET_ENABLE         0
#define MINIMUM_FIRMWARE_VERSION    "0.7.7"
#define MODE_LED_BEHAVIOUR          "MODE"


// Value for low pass filter
const float alpha = 0.5;

// Filtered values
double fXg = 0;
double fYg = 0;
double fZg = 0;


void displaySensorDetails(void)
{
  sensor_t sensor;
  accel.getSensor(&sensor);
  Serial.println("------------------------------------");
  Serial.print  ("Sensor:       "); Serial.println(sensor.name);
  Serial.print  ("Driver Ver:   "); Serial.println(sensor.version);
  Serial.print  ("Unique ID:    "); Serial.println(sensor.sensor_id);
  Serial.print  ("Max Value:    "); Serial.print(sensor.max_value); Serial.println(" m/s^2");
  Serial.print  ("Min Value:    "); Serial.print(sensor.min_value); Serial.println(" m/s^2");
  Serial.print  ("Resolution:   "); Serial.print(sensor.resolution); Serial.println(" m/s^2");
  Serial.println("------------------------------------");
  Serial.println("");
  delay(500);
}

void setup(void)
{

 //Setup accelerometer and wait for a connection
#ifndef ESP8266
  while (!Serial);     // will pause Zero, Leonardo, etc until serial console opens
#endif
  Serial.begin(9600);
  Serial.println("Accelerometer Test"); Serial.println("");

  /* Initialise the sensor */
  if (!accel.begin() && !mag.begin())
  {
    /* There was a problem detecting the ADXL345 ... check your connections */
    Serial.println("Ooops, no LSM303 detected ... Check your wiring!");
    while (1);
  }

  /* Display some basic information on this sensor */
  displaySensorDetails();

  accel.enableAutoRange(true);            /* Auto-gain ... switches automatically between 1x and 16x */
  mag.enableAutoRange(true);


  /* Initialise the module */
  Serial.print(F("Initialising the Bluefruit LE module: "));

  if ( !ble.begin(VERBOSE_MODE) )
  {
    error(F("Couldn't find Bluefruit, make sure it's in CoMmanD mode & check wiring?"));
  }
  Serial.println( F("OK!") );


  /* Disable command echo from Bluefruit */
  ble.echo(false);

  //Serial.println("Requesting Bluefruit info:");
  /* Print Bluefruit information */
  //ble.info();

  Serial.println(F("Connect to Flora in UART Mode"));
  Serial.println();

  ble.setMode(BLUEFRUIT_MODE_COMMAND);

  ble.verbose(false);  // debug info is a little annoying after this point!


  /* Wait for connection */
  while (!ble.isConnected()) {
    delay(500);
    Serial.println(F("Not connected"));
  }

  Serial.println( F("Switching to DATA mode!") );
  ble.setMode(BLUEFRUIT_MODE_DATA);

}

void loop(void)
{
  /* Get a new sensor event */
  sensors_event_t accelEvent, magEvent;

  accel.getEvent(&accelEvent);
  mag.getEvent(&magEvent);


  /* Serialise values as JSON over the UART serial */
  ble.print("{");
  if (accelEvent.acceleration.x)

  {
    double pitch, roll;

   //Low Pass Filter
    fXg = accelEvent.acceleration.x * alpha + (fXg * (1.0 - alpha));
    fYg = accelEvent.acceleration.y * alpha + (fYg * (1.0 - alpha));
    fZg = accelEvent.acceleration.z * alpha + (fZg * (1.0 - alpha));
 
    //Roll & Pitch Equations
    roll  = (atan2(-fYg, fZg)*180.0)/M_PI;
    pitch = (atan2(fXg, sqrt(fYg*fYg + fZg*fZg))*180.0)/M_PI;
    
    ble.print("'AccelX':'"); ble.print(fXg); ble.print("','AccelY':'"); ble.print(fYg); ble.print("','AccelZ':'"); ble.print(fZg);     ble.print("','Pitch':'");ble.print(pitch);ble.print("','Roll':'");ble.print(roll);ble.print("'"); 

    /* Display the results (acceleration is measured in m/s^2) */
    Serial.print("AccelX: "); Serial.print(fXg); Serial.print("  ");
    Serial.print("AccelY: "); Serial.print(fYg); Serial.print("  ");
    Serial.print("AccelZ: "); Serial.print(fZg); Serial.print("  "); 
     Serial.print("Pitch: "); Serial.print(pitch); Serial.print("  "); 
    Serial.print("Roll: "); Serial.print(roll); Serial.print("  "); 

  }
  /* Serialise compass values if they exist */
  if (magEvent.magnetic.x)
  {
    if (accelEvent.acceleration.x) {
      // Print a comma to format the json fromn the previous entry
      ble.print(",");
    }

    ble.print("'MagX':'"); ble.print(magEvent.magnetic.x); ble.print("','MagY':'"); ble.print(magEvent.magnetic.y); ble.print("','MagZ':'"); ble.print(magEvent.magnetic.z);
    /* Display the results (acceleration is measured in m/s^2) */
    Serial.print("MagX: "); Serial.print(magEvent.magnetic.x); Serial.print("  ");
    Serial.print("MayY: "); Serial.print(magEvent.magnetic.y); Serial.print("  ");
    Serial.print("MagZ: "); Serial.print(magEvent.magnetic.z); Serial.print("  "); 
  }
  
  Serial.println();
  ble.println("'}");


  /* Delay before the next sample */
  delay(30);
}

// A small helper
void error(const __FlashStringHelper*err) {
  Serial.println(err);
  while (1);
}

