#include "Arduino.h"

#include <ESP8266WiFi.h>
#include <Wire.h>
#include <Adafruit_ADS1015.h>
#include <Adafruit_SHT31.h>

#include "Secrets.h"

Adafruit_ADS1115 ads;  /* Use this for the 16-bit version */
Adafruit_SHT31 sht31 = Adafruit_SHT31();

//Replace with your thingspeak API key,
String apiKey = APIKEY;
//WIFI credentials go here
const char* ssid     = SSID;
const char* password = PASSWORD;

const char* server = "api.thingspeak.com";
WiFiClient client;

void setup(void)
{
  Serial.begin(9600);

  //Connecting to SHT31
  if (! sht31.begin(0x44)) {   //0x45 is an alternative address
    Serial.println("Couldn't find SHT31");
    while (1) delay(1);
  }

  Serial.println("Getting single-ended readings from AIN0..3");
  Serial.println("ADC Range: +/- 6.144V (1 bit = 3mV/ADS1015, 0.1875mV/ADS1115)");

  // The ADC input range (or gain) can be changed via the following
  // functions, but be careful never to exceed VDD +0.3V max, or to
  // exceed the upper and lower limits if you adjust the input range!
  // Setting these values incorrectly may destroy your ADC!
  //                                                                ADS1015  ADS1115
  //                                                                -------  -------
  ads.setGain(GAIN_TWOTHIRDS);  	// 2/3x gain +/- 6.144V  1 bit = 3mV      0.1875mV (default)
  // ads.setGain(GAIN_ONE);        	// 1x gain   +/- 4.096V  1 bit = 2mV      0.125mV
  // ads.setGain(GAIN_TWO);        	// 2x gain   +/- 2.048V  1 bit = 1mV      0.0625mV
  // ads.setGain(GAIN_FOUR);       	// 4x gain   +/- 1.024V  1 bit = 0.5mV    0.03125mV
  // ads.setGain(GAIN_EIGHT);      	// 8x gain   +/- 0.512V  1 bit = 0.25mV   0.015625mV
  // ads.setGain(GAIN_SIXTEEN);    	// 16x gain  +/- 0.256V  1 bit = 0.125mV  0.0078125mV
  ads.begin();

  //Connecting to network
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
}

void loop(void)
{
  int16_t adc0;//, adc1, adc2, adc3;

  adc0 = ads.readADC_SingleEnded(0);
  Serial.print("AIN0: "); Serial.println(adc0);

  float t = sht31.readTemperature();
  float h = sht31.readHumidity();

  if (! isnan(t)) {  // check if 'is not a number'
    Serial.print("Temp *C = "); Serial.println(t);
  } else {
    Serial.println("Failed to read temperature");
  }

  if (! isnan(h)) {  // check if 'is not a number'
    Serial.print("Hum. % = "); Serial.println(h);
  } else {
    Serial.println("Failed to read humidity");
  }
  Serial.println();

  //Send to ThingSpeak
  if (client.connect(server,80)) {  //   "184.106.153.149" or api.thingspeak.com
    String postStr = apiKey;
           postStr +="&field1=";
           postStr += String(adc0);
           postStr +="&field2=";
           postStr += String(t);
           postStr +="&field3=";
           postStr += String(h);
           postStr += "\r\n\r\n";

     client.print("POST /update HTTP/1.1\n");
     client.print("Host: api.thingspeak.com\n");
     client.print("Connection: close\n");
     client.print("X-THINGSPEAKAPIKEY: "+apiKey+"\n");
     client.print("Content-Type: application/x-www-form-urlencoded\n");
     client.print("Content-Length: ");
     client.print(postStr.length());
     client.print("\n\n");
     client.print(postStr);
  }
  client.stop();

  delay(30000);
}
