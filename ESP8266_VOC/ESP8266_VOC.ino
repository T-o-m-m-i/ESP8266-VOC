//TODO:
//
//
#include "Arduino.h"
#include <SPI.h>
#include <SD.h>
#include <ESP8266WiFi.h>
#include <Wire.h>
#include <Adafruit_ADS1015.h>
#include <Adafruit_SHT31.h>
#include <Adafruit_BME280.h>
#include <NTPClient.h>
#include <WiFiUdp.h>

#include "Secrets.h"


WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "fi.pool.ntp.org", 7200);
//strDateTime datetime;

Adafruit_ADS1115 ads;  /* Use this for the 16-bit version */
Adafruit_SHT31 sht31 = Adafruit_SHT31();

Adafruit_BME280 bme; // I2C

//Replace with your thingspeak API key,
String apiKey = APIKEY;
//WIFI credentials go here
const char* ssid     = SSID;
const char* password = PASSWORD;

const char* server = "api.thingspeak.com";
WiFiClient client;

const unsigned long interval = 300000;  //5min  15min=900000
unsigned long previousMillis = 300000;

uint16_t voc;
float temp;
float humid;

float bme_temp;
float bme_pressure;
float bme_humid;

const int chipSelect = D8;  //For SD card.
File root;
bool noSD = false;

void setup(void)
{
	voc = 0;
	temp = 0;
	humid = 0;

	Serial.begin(9600);

	Serial.println("\r\nWaiting for SD card to initialise...");
	if (!SD.begin(chipSelect)) { // CS is D8 in this example
	    Serial.println("Initialisation failed!");
	    noSD = true;
	    //return;
	} else {
	Serial.println("Initialisation completed.");
	}

	//Connecting to SHT31
	if (! sht31.begin(0x44))  //0x45 is an alternative address
	{
		Serial.println("Couldn't find the SHT31 sensor.");
		while (1);
	}

	if (!bme.begin()) {
		Serial.println("Couldn't find the BME280 sensor.");
		while (1);
	}

	// The ADC input range (or gain) can be changed via the following
	// functions, but be careful never to exceed VDD +0.3V max, or to
	// exceed the upper and lower limits if you adjust the input range!
	// Setting these values incorrectly may destroy your ADC!
	//                                                                ADS1015  ADS1115
	//                                                                -------  -------
	// ads.setGain(GAIN_TWOTHIRDS);  	// 2/3x gain +/- 6.144V  1 bit = 3mV      0.1875mV (default)
	ads.setGain(GAIN_ONE);        	// 1x gain   +/- 4.096V  1 bit = 2mV      0.125mV
	// ads.setGain(GAIN_TWO);        	// 2x gain   +/- 2.048V  1 bit = 1mV      0.0625mV
	// ads.setGain(GAIN_FOUR);       	// 4x gain   +/- 1.024V  1 bit = 0.5mV    0.03125mV
	// ads.setGain(GAIN_EIGHT);      	// 8x gain   +/- 0.512V  1 bit = 0.25mV   0.015625mV
	// ads.setGain(GAIN_SIXTEEN);    	// 16x gain  +/- 0.256V  1 bit = 0.125mV  0.0078125mV
	ads.begin();

	//Connecting to network
	Serial.println();
	Serial.print("Connecting to ");
	Serial.println(ssid);

	if (WiFi.status() != WL_CONNECTED) {
	  WiFi.begin(ssid, password);
	}

	while (WiFi.status() != WL_CONNECTED) {
	delay(500);
	Serial.print(".");
	}

	Serial.println("");
	Serial.println("WiFi connected.");
	Serial.print("IP address: ");
	Serial.println(WiFi.localIP());

	timeClient.begin();
}

void loop(void)
{
	//Send to ThingSpeak
	if ((unsigned long)(millis() - previousMillis) >= interval)
	{
		timeClient.update();

		for(int i=0; i<10; i++)
		{
			delay(100);
			voc = (voc + ads.readADC_SingleEnded(0)) / 2.0;
			temp = (temp + sht31.readTemperature()) / 2.0;
			humid = (humid + sht31.readHumidity()) / 2.0;
		}

		//Discard the first measurement.
		for(int i=0; i<2; i++)
		{
			bme_temp = bme.readTemperature();
			bme_pressure = bme.readPressure();
			bme_humid = bme.readHumidity();
		}
///----------------------------------------------
		if(!noSD)
		{
			File file = SD.open("data.txt", FILE_WRITE); // FILE_WRITE opens file for writing and moves to the end of the file, returns 0 if not available
			if(file)
			{
				//Serial.println(timeClient.getFormattedTime());
				String dataStr = String(timeClient.datetime.day) + "."
								+ String(timeClient.datetime.month) + "."
								+ String(timeClient.datetime.year) + ","
								+ String(timeClient.datetime.hour + 2) + ":"
								+ String(timeClient.datetime.minute) + ":"
								+ String(timeClient.datetime.second) + ","
								+ String(voc) + ","
								+ String(temp) + ","
								+ String(humid) + ","
								+ String(bme_temp) + ","
								+ String(bme_pressure) + ","
								+ String(bme_humid);
				file.println(dataStr);
				file.close();
				//Serial.println(dataStr);
				//Serial.println("Data written to SD.");
			}
		}
		else
		{
			String dataStr = String(timeClient.datetime.day) + "."
							+ String(timeClient.datetime.month) + "."
							+ String(timeClient.datetime.year) + ","
							+ String(timeClient.datetime.hour + 2) + ":"
							+ String(timeClient.datetime.minute) + ":"
							+ String(timeClient.datetime.second) + ","
							+ String(voc) + ","
							+ String(temp) + ","
							+ String(humid) + ","
							+ String(bme_temp) + ","
							+ String(bme_pressure) + ","
							+ String(bme_humid);

							Serial.println(dataStr);
							Serial.println("Data NOT written to SD.");
		}
///----------------------------------------------
		if (client.connect(server,80)) //   "184.106.153.149" or api.thingspeak.com
		{
			String postStr = apiKey;
			postStr +="&field1=";
			postStr += String(voc);
			postStr +="&field2=";
			postStr += String(temp);
			postStr +="&field3=";
			postStr += String(humid);
			postStr +="&field4=";
			postStr += String(bme_temp);
			postStr +="&field5=";
			postStr += String(bme_pressure);
			postStr +="&field6=";
			postStr += String(bme_humid);
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
///----------------------------------------------
		previousMillis = millis();
	}
}
