#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include "RunningAverage.h"

//needed for WifiManager library
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>         //https://github.com/tzapu/WiFiManager

const char* ssid = "EAZHuis";
const char* password = "HommeJan54";

const char RESERVED_PINS[3] = {0,2};
const char FREE_PINS[1] = {12};
const char ENDSTOP_PIN = 15;//D8
const char BEGINSTOP_PIN = 13;//D7
const char MOTOR_C = 5;//D1
const char MOTOR_D = 4;//D2
const char MOTOR_ENA = 0;//D3
const char LDR_TOBEGIN_ON = 16; //D0
const char LDR_TOEND_ON = 14; //D5
const char LDR_ADC = A0;//ADC0; //

int slowLoopTimeout = millis();
void setup() {
  pinSetup();
  Serial.begin(115200);
  Serial.println("Booting");
  WiFiManager wifiManager;
  wifiManager.autoConnect("SolarRoof");
  otaSetup();
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
}


void loop() {
  ArduinoOTA.handle();
  if(millis()-slowLoopTimeout>25){
    slowLoopTimeout = millis();
    platformTask();
    sunSensorTask();
    sunTask();
  }
}

typedef enum sunstate {
  SUN_IDLE,
  SUN_MOVING_TOEND,
  SUN_WAITINGFORSUNDOWN,
  SUN_MOVING_TOBEGIN,
  SUN_WAITINGFORSUNRISE,  
}sunState_t;

typedef enum platformstate {
  PLATFORM_INBETWEEN,
  PLATFORM_ATEND,
  PLATFORM_ATBEGIN,
  PLATFORM_IDLE,
}platformState_t;

platformState_t platformState = PLATFORM_IDLE;

void platformTask(){
  if(!digitalRead(BEGINSTOP_PIN){
    platformState = PLATFORM_ATBEGIN;
  }else if(!digitalRead(ENDSTOP_PIN){
    platformState = PLATFORM_ATEND;
  }else{
    platformState = PLATFORM_INBETWEEN;
  }
}

sunState_t sunState = SUN_IDLE;
int sunTimeout = 0;
int sunMoveTime = 0;
const int PLATFORMMAXIMUMMOVETIME = 500;
const int SUNUPTHRESHOLD = 100;

void sunTask(){
  switch(sunState){
    case SUN_IDLE:
      if(platformState==PLATFORM_ATEND){
        if(sunToBeginAvgSlow.getAverage()<SUNUPTHRESHOLD){
        sunState = SUN_WAITINGFORSUNDOWN;
        }else{
          sunState = SUN_MOVING_TOBEGIN;
        }
      }else if(platformState==PLATFORM_ATBEGIN){
        if(sunToBeginAvgSlow.getAverage()<SUNUPTHRESHOLD){
        sunState = SUN_MOVING_TOEND;
        }else{
          sunState = SUN_WAITINGFORSUNRISE;
        }        
      }else(
        if(sunToBeginAvgSlow.getAverage()<SUNUPTHRESHOLD){
          if(sunToBeginAvgSlow.getAverage()>sunToEndAvgSlow.getAverage()){
            sunState = SUN_MOVING_TOBEGINNING;
          }else{
            sunState = SUN_MOVING_TOEND;
        }else{
          sunState = SUN_MOVING_TOBEGINNING;
        }            
      }
      
  }
}

const int SUNMEASURESPEED = 100; //milliseconds between samples
const int SUNFASTAVERAGE = 10;
const int SUNSLOWAVERAGE = 20;
RunningAverage sunToBeginAvgSlow(SUNSLOWAVERAGE);
RunningAverage sunToEndAvgSlow(SUNSLOWAVERAGE);
RunningAverage sunToBeginAvgFast(SUNFASTAVERAGE);
RunningAverage sunToEndAvgFast(SUNFASTAVERAGE);
int sunMeasureSamples = 0;
int sunMeasureTimeout = 0;

typedef enum sunsensorstate{
  SUN_MEASURING_TOBEGIN,
  SUN_MEASURING_TOEND,
  SUN_IDLE,
}sunSensorState_t;

sunSensorState_t sunSensorState;



void sunSensorTask(void){
  if(millis()-sunMeasureTimeout>SUNMEASURESPEED){
    sunMeasureTimeout += SUNMEASURESPEED;
    switch(sunSensorState){
      case SUN_MEASURING_TOBEGIN:
        sunToBeginAvgFast.addValue(analogRead(LDR_ADC));
        pinMode(LDR_TOBEGIN_ON,INPUT);
        digitalWrite(LDR_TOEND_ON,0);
        sunSensorState = SUN_MEASURING_TOEND;      
       break;
      case SUN_MEASURING_TOEND:
        sunToEndAvgFast.addValue(analogRead(LDR_ADC));
        pinMode(LDR_TOEND_ON,INPUT);
        digitalWrite(LDR_TOBEGIN_ON,0);
        sunSensorState = SUN_MEASURING_TOEND;      
       break;
      case SUN_IDLE:
        sunMeasureTimeout = millis();
        pinMode(LDR_TOEND_ON,INPUT);
        digitalWrite(LDR_TOBEGIN_ON,0);
        senSensorState = SUN_MEASURING_TOBEGIN;
        sunToBeginAvgFast.clear();
        sunToEndAvgFast.clear();
        sunMeasureSamples = 0;
       break;
    }
  }
  if(++sunMeasureSamples%SUNFASTAVERAGE==0){
    sunToBeginAvgSlow.addValue(sunToBeginAvgFast.getAverage());
    sunToEndAvgSlow.addValue(sunToEndAvgFast.getAverage());   
  }
}


void pinSetup(){
  pinMode(ENDSTOP_PIN,INPUT_PULLUP);
  pinMode(BEGINSTOP_PIN, INPUT_PULLUP);
  pinMode(MOTOR_C,OUTPUT);
  pinMode(MOTOR_D,OUTPUT);
  pinMode(MOTOR_ENA,OUTPUT);
  pinMode(LDR_TOBEGIN_ON,INPUT);
  pinMode(LDR_TOEND_ON,INPUT);
}

void otaSetup(){
    ArduinoOTA.onStart([]() {
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH)
      type = "sketch";
    else // U_SPIFFS
      type = "filesystem";

    // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
    Serial.println("Start updating " + type);
  });
  
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
  });
  
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR) Serial.println("End Failed");
  });
  
  ArduinoOTA.begin();
  Serial.println(" ");
}

