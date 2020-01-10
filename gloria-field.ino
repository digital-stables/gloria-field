#include <Adafruit_NeoPixel.h>
#include "GravityRtc.h"
#include "Wire.h"

GravityRtc rtc;     //RTC Initialization

#define RELAY_PIN 15
#define TMP36_PIN A0
#define NEOPIXEL_PIN        14
// How many NeoPixels are attached to the Arduino?
#define NUMPIXELS 1
#define DELAYVAL 500
long lastTime=0L;
long millisecondsToBlink=10000;

boolean relayState=true;

#include <avr/sleep.h>
#include <avr/wdt.h>
#include <avr/power.h>


const byte CHIP_ENABLE = 12;
const byte CHIP_SELECT = 4;

// watchdog interrupt
ISR (WDT_vect) 
  {
  wdt_disable();  // disable watchdog
  }  // end of WDT_vect

#include <SPI.h>
#include "nRF24L01.h"
Adafruit_NeoPixel pixels(NUMPIXELS, NEOPIXEL_PIN, NEO_GRB + NEO_KHZ800);
const float InternalReferenceVoltage = 1.086; // as measured
#include "RF24.h"

// Radio pipe addresses for the 2 nodes to communicate.
const uint64_t pipes[2] = { 0xF0F0F0F0E1LL, 0xF0F0F0F0D2LL };


void loopOverPrimaries(){
   pixels.clear(); // Set all pixel colors to 'off'
  pixels.setPixelColor(0, pixels.Color(0, 255, 0));
  pixels.show();   // Send the updated pixel colors to the hardware.
  delay(DELAYVAL); // Pause before next pass through loop

    pixels.setPixelColor(0, pixels.Color(255, 0, 0));
  pixels.show();   // Send the updated pixel colors to the hardware.
  delay(DELAYVAL); // Pause before next pass through loop


 pixels.setPixelColor(0, pixels.Color(0,0, 255));
  pixels.show();   // Send the updated pixel colors to the hardware.
  delay(DELAYVAL); // Pause before next pass through loop


   pixels.clear(); // Set all pixel colors to 'off'
   pixels.show();   
}


//
// sourcez; https://forum.arduino.cc/index.php?topic=204429.0
//
float getSourceVoltage(void) // Returns actual value of Vcc (x 100)
    {
       
     // For 1284/644
     const long InternalReferenceVoltage = 1065L;  // Adjust this value to your boards specific internal BG voltage x1000
        // REFS1 REFS0          --> 0 1, AVcc internal ref. -Selects AVcc reference
        // MUX4 MUX3 MUX2 MUX1 MUX0  --> 11110 1.1V (VBG)         -Selects channel 30, bandgap voltage, to measure
     ADMUX = (0<<REFS1) | (1<<REFS0) | (0<<ADLAR)| (1<<MUX4) | (1<<MUX3) | (1<<MUX2) | (1<<MUX1) | (0<<MUX0);

     delay(50);  // Let mux settle a little to get a more stable A/D conversion
        // Start a conversion 
     ADCSRA |= _BV( ADSC );
        // Wait for it to complete
     while( ( (ADCSRA & (1<<ADSC)) != 0 ) );
        // Scale the value
     int results = (((InternalReferenceVoltage * 1023L) / ADC) + 5L) / 10L; // calculates for straight line value
     float voltage = results/100.0;
	return voltage;

    }


void sendVoltage (float reading)
  {
  // Set up nRF24L01 radio on SPI bus plus pins 9 & 10
  
  RF24 radio(CHIP_ENABLE, CHIP_SELECT);
  
  power_all_enable();
  digitalWrite (SS, HIGH);
  SPI.begin ();
  digitalWrite (CHIP_ENABLE, LOW); 
  digitalWrite (CHIP_SELECT, HIGH);
  
  //
  // Setup and configure rf radio
  //
  radio.begin();

  // optionally, increase the delay between retries & # of retries
  radio.setRetries(15, 15);

  // optionally, reduce the payload size.  seems to improve reliability
  radio.setPayloadSize(8);

  radio.openWritingPipe (pipes[0]);
  radio.openReadingPipe (1, pipes[1]);

  radio.startListening ();
  delay (10);
  radio.stopListening ();

  
  
  delay (10);
  
  bool ok = radio.write (&reading, sizeof reading);
  
  radio.startListening ();
  radio.powerDown ();
  
  SPI.end ();
  // set pins to OUTPUT and LOW  
  for (byte i = 9; i <= 13; i++)
    {
    pinMode (i, OUTPUT);    
    digitalWrite (i, LOW); 
    }  // end of for loop
  ADCSRA = 0;  // disable ADC
  power_all_disable();
  
  }
float getTemeprature(){
	int reading = analogRead(TMP36_PIN);  
	float voltage = reading * 5.0;
 	voltage /= 1024.0;  
	float temperatureC = (voltage - 0.5) * 100 ;  //converting from 10 mv per degree wit 500 mV offset
	return temperatureC;
}

void setup() {

  //clock_prescale_set (clock_div_2);

//  // set pins to OUTPUT and LOW  
//  for (byte i = 0; i <= A5; i++)
//    {
//    // skip radio pins
//    if (i >= 9 && i <= 13)
//      continue;
//    pinMode (i, OUTPUT);    
//    digitalWrite (i, LOW);  
//    }  // end of for loop
//
//  ADCSRA = 0;  // disable ADC
//  power_all_disable ();   // turn off all modules
  
  Serial.begin(9600);
  pinMode(TMP36_PIN, INPUT);
  pinMode(RELAY_PIN, OUTPUT);
  rtc.setup();

//Set the RTC time automatically: Calibrate RTC time by your computer time
  //rtc.adjustRtc(F(__DATE__), F(__TIME__));
  //Set the RTC time manually
 //rtc.adjustRtc(2019,12,17,2,16,43,0);  //Set time: 2017/6/19, Monday, 12:07:00
  // put your setup code here, to run once:
  pixels.begin();
  pixels.clear(); // Set all pixel colors to 'off'
  pixels.setPixelColor(0, pixels.Color(255, 255, 0));
  pixels.show();   // Send the updated pixel colors to the hardware.
  delay(DELAYVAL); // Pause before next pass through loop
 lastTime=millis();
 
  
 }

unsigned int counter;

void loop() {
	pixels.clear(); // Set all pixel colors to 'off'
	if(millis()-lastTime>millisecondsToBlink){
		if(relayState)digitalWrite(RELAY_PIN,HIGH);
    else digitalWrite(RELAY_PIN,LOW);
   rtc.read();
  Serial.print(rtc.year);
  Serial.print("/");//month
  Serial.print(rtc.month);
  Serial.print("/");//day
  Serial.print(rtc.day);
  Serial.print(" ");//week
 // Serial.print(rtc.week);
  Serial.print(" ");//hour
  Serial.print(rtc.hour);
  Serial.print(":");//minute
  Serial.print(rtc.minute);
  Serial.print(":");//second
  Serial.print(rtc.second);
		
		float temperature = getTemeprature();
   Serial.print("    relayState=");
    Serial.print(relayState);
		Serial.print(" temp=");
    Serial.print(temperature);
		float battVolts=getSourceVoltage();
		Serial.print("  bandgap=");
		Serial.println(battVolts);
		loopOverPrimaries();
  	lastTime=millis();  
    if(relayState)pixels.setPixelColor(0, pixels.Color(255, 0, 0));
    else pixels.setPixelColor(0, pixels.Color(0 ,255, 0));
     
    pixels.show();   // Send the updated pixel colors to the hardware .
    delay(DELAYVAL); // Pause before next pass through loo 
    relayState=!relayState;   


// every 64 seconds send a reading    
//  if ((++counter & 7 ) == 0)
//    {
//      pixels.setPixelColor(0, pixels.Color(0 ,255, 255));
//     
//    pixels.show();
//    
//    
//    sendVoltage (battVolts);
//
//    
//     if(relayState)pixels.setPixelColor(0, pixels.Color(255, 0, 0));
//    else pixels.setPixelColor(0, pixels.Color(0 ,255, 0));
//     
//    pixels.show();
//    } // end of 64 seconds being up
//
//    // clear various "reset" flags
//  MCUSR = 0;     
//  // allow changes, disable reset
//  WDTCSR = bit (WDCE) | bit (WDE);
//  // set interrupt mode and an interval 
//  WDTCSR = bit (WDIE) | bit (WDP3) | bit (WDP0);    // set WDIE, and 8 seconds delay
//  wdt_reset();  // pat the dog
//  
//  set_sleep_mode (SLEEP_MODE_PWR_DOWN);  
//  sleep_enable();
//  sleep_cpu ();  
//  sleep_disable();
                                                          
	}
}
