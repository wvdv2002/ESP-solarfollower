#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <ArduinoOTA.h>
#include "RunningAverage.h"
//#include "Dusk2Dawn.h"

#include <ESP8266WiFi.h>
//needed for WifiManager library
#include <DNSServer.h>
//#include <WiFiManager.h>         //https://github.com/tzapu/WiFiManager
#include <ESPUI.h>
#include "states.h"

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

const int PLATFORMMAXIMUMMOVETIME = 500;
const int SUNUPTHRESHOLD = 100;
const int SUNMEASURESPEED = 100; //milliseconds between samples
const int SUNFASTAVERAGE = 10;
const int SUNSLOWAVERAGE = 20;



platformState_t platformState = PLATFORM_IDLE;
sunState_t sunState = SUN_IDLE;
sunSensorState_t sunSensorState = SUNSENSOR_IDLE;
platformMovingState_t platformMovingState = PLATFORM_STOPPING;

int sunTimeout = 0;
int sunMoveTime = 0;
int sunMeasureSamples = 0;
int sunMeasureTimeout = 0;


RunningAverage sunToBeginAvgSlow(SUNSLOWAVERAGE);
RunningAverage sunToEndAvgSlow(SUNSLOWAVERAGE);
RunningAverage sunToBeginAvgFast(SUNFASTAVERAGE);
RunningAverage sunToEndAvgFast(SUNFASTAVERAGE);

int slowLoopTimeout = millis();
int espuiLoopTimeout = millis();
void setup() {
  pinSetup();
  Serial.begin(115200);
  Serial.println("Booting");
//  WiFiManager wifiManager;
//  wifiManager.autoConnect("SolarRoof");
  
  WiFi.mode(WIFI_STA);
  //WiFi.setHostname(ssid);  
  WiFi.begin(ssid, password);

    while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  
  otaSetup();
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  Serial.println("Server started");
  espuiSetup();
}


void loop() {
  ArduinoOTA.handle();
  if(millis()-slowLoopTimeout>25){
    slowLoopTimeout = millis();
    platformTask();
    sunSensorTask();
    sunTask();
    espuiTask();
  }
}


void espuiTask(){
  if (millis() - espuiLoopTimeout > 2000) {
//    ESPUI.print("Millis:", String(millis()));
    ESPUI.print("SunStat:",sunState_str[sunState]);
    ESPUI.print("PfStat:",platformState_str[platformState]);
    ESPUI.print("SMStat:",sunSensorState_str[sunSensorState]);
    ESPUI.print("PfMov:",platformMovingState_str[platformMovingState]);
    ESPUI.print("SlowEnd:",String(sunToBeginAvgSlow.getFastAverage()));
    ESPUI.print("SlowBegin:",String(sunToEndAvgSlow.getFastAverage()));    
    ESPUI.print("FastEnd:",String(sunToBeginAvgFast.getFastAverage()));
    ESPUI.print("FastBegin:",String(sunToEndAvgFast.getFastAverage()));  
    espuiLoopTimeout = millis();
  }
}

void moveEndCallback(Control sender, int type) {
  switch (type) {
  case B_DOWN:
    movePlatform(1);
    break;
  case B_UP:
    movePlatform(0);
    break;
  }
}

void moveBeginCallback(Control sender, int type) {
  switch (type) {
  case B_DOWN:
    movePlatform(-1);
    break;
  case B_UP:
    movePlatform(0);
    break;
  }
}

void movePlatform(int dir){
  
  if(dir==-1){
    if(platformState != PLATFORM_ATBEGIN){
      platformMovingState = PLATFORM_MOVINGTOBEGIN;
      digitalWrite(MOTOR_D,0);
      digitalWrite(MOTOR_C,1);
      digitalWrite(MOTOR_ENA,1);
      Serial.println("mov beg");
    }
  }else if(dir==1){
    if(platformState != PLATFORM_ATEND){
      platformMovingState = PLATFORM_MOVINGTOEND;
      digitalWrite(MOTOR_C,0);
      digitalWrite(MOTOR_D,1);
      digitalWrite(MOTOR_ENA,1);    
      Serial.println("mov end");
    }
  }else{
    platformMovingState = PLATFORM_STOPPING;
    digitalWrite(MOTOR_ENA,0);
    Serial.println("stopping");
  }
  
}



void espuiSetup(){
  ESPUI.label("SunStat:", COLOR_TURQUOISE, "IDLE");
  ESPUI.label("SMStat:", COLOR_EMERALD, "0");
  ESPUI.label("PfStat:", COLOR_WETASPHALT,"IDLE");
  ESPUI.label("PfMov:", COLOR_WETASPHALT,"IDLE");
  ESPUI.label("SlowEnd:", COLOR_SUNFLOWER,"0");
  ESPUI.label("SlowBegin:", COLOR_CARROT,"0");
  ESPUI.label("FastEnd:", COLOR_SUNFLOWER,"0");
  ESPUI.label("FastBegin:", COLOR_CARROT,"0");
  ESPUI.button("MoveToBegin", &moveBeginCallback, COLOR_PETERRIVER);
  ESPUI.button("MoveToEnd", &moveEndCallback, COLOR_PETERRIVER);
  ESPUI.begin("ESP32 Control");
}




void platformTask(){
  int oldPlatformState = platformState;
  if(!digitalRead(BEGINSTOP_PIN)){
    platformState = PLATFORM_ATBEGIN;
  }else if(!digitalRead(ENDSTOP_PIN)){
    platformState = PLATFORM_ATEND;
  }else{
    platformState = PLATFORM_INBETWEEN;
  }
  if(platformState == PLATFORM_ATBEGIN && platformMovingState == PLATFORM_MOVINGTOBEGIN){
    movePlatform(0);
  }else if(platformState == PLATFORM_ATEND && platformMovingState == PLATFORM_MOVINGTOEND){
    movePlatform(0);
  }
}


void sunTask(){
  switch(sunState){
    case SUN_IDLE:
      if(platformState==PLATFORM_ATEND){
        if(sunToBeginAvgSlow.getFastAverage()<SUNUPTHRESHOLD){
        sunState = SUN_WAITINGFORSUNDOWN;
        }else{
          sunState = SUN_MOVING_TOBEGIN;
        }
      }else if(platformState==PLATFORM_ATBEGIN){
        if(sunToBeginAvgSlow.getFastAverage()<SUNUPTHRESHOLD){
        sunState = SUN_MOVING_TOEND;
        }else{
          sunState = SUN_WAITINGFORSUNRISE;
        }        
      }else{
        if(sunToBeginAvgSlow.getFastAverage()<SUNUPTHRESHOLD){
          if(sunToBeginAvgSlow.getFastAverage()>sunToEndAvgSlow.getFastAverage()){
            sunState = SUN_MOVING_TOBEGIN;
          }else{
            sunState = SUN_MOVING_TOEND;
          }
         }else{
          sunState = SUN_MOVING_TOBEGIN;
        }            
      }
    }  
}

void sunSensorTask(void){
  if(millis()-sunMeasureTimeout>SUNMEASURESPEED){
    sunMeasureTimeout += SUNMEASURESPEED;
    switch(sunSensorState){
      case SUNSENSOR_MEASURING_TOBEGIN:
        sunToBeginAvgFast.addValue(analogRead(LDR_ADC));
        pinMode(LDR_TOBEGIN_ON,INPUT);
        digitalWrite(LDR_TOEND_ON,0);
        sunSensorState = SUNSENSOR_MEASURING_TOEND;      
       break;
      case SUNSENSOR_MEASURING_TOEND:
        sunToEndAvgFast.addValue(analogRead(LDR_ADC));
        pinMode(LDR_TOEND_ON,INPUT);
        digitalWrite(LDR_TOBEGIN_ON,0);
        sunSensorState = SUNSENSOR_MEASURING_TOEND;      
       break;
      case SUNSENSOR_IDLE:
        sunMeasureTimeout = millis();
        pinMode(LDR_TOEND_ON,INPUT);
        digitalWrite(LDR_TOBEGIN_ON,0);
        sunSensorState = SUNSENSOR_MEASURING_TOBEGIN;
        sunToBeginAvgFast.clear();
        sunToEndAvgFast.clear();
        sunMeasureSamples = 0;
       break;
    }
  }
  if(++sunMeasureSamples%SUNFASTAVERAGE==0){
    sunToBeginAvgSlow.addValue(sunToBeginAvgFast.getFastAverage());
    sunToEndAvgSlow.addValue(sunToEndAvgFast.getFastAverage());   
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

