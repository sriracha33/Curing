#include <Wire.h>
#include <EEPROM.h>
#include <DHTesp.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>

#include "Wifi.h"
/*Define WIFI SSID and Password if not using separate header file.
#define WIFI_SSID "ssid"
#define WIFI_PASSWORD "password"
*/

//configuration defines
#define TEMP_DEADBAND 1
#define HUMIDITY_DEADBAND 1
#define TEMP_MAX 90
#define TEMP_MIN 40
#define HUMIDITY_MAX 90
#define HUMIDITY_MIN 20

DHTesp dht;
Adafruit_SSD1306 display = Adafruit_SSD1306(128, 32, &Wire);

ESP8266WebServer server(80);    // Create a webserver object that listens for HTTP request on port 80
void handleXML();              // function prototypes for XML file.
void handleNotFound();

float humidity;
float temperature;
int lastUpdateTime;
uint8_t saveCountdown=0;
uint8_t menuCountdown=0;
int updateDelay;
uint8_t buttons,lastbuttons,changedbuttons;
uint8_t menu=0;
uint8_t humiditySP,temperatureSP;
boolean tempOn,humidityOn;
boolean tempControl,humidityControl;
boolean saved=false;

//ESP8266
#define BUTTON_A  0
#define BUTTON_B 16
#define BUTTON_C  2

#define TEMP_PIN 12
#define HUMIDITY_PIN 13
#define DHT_PIN 14

void setup(){
  Serial.begin(115200);
  
  delay(500);  //needed so display will start up on power loss without a reset

  EEPROM.begin(4);
  byte temp=EEPROM.read(0);
  tempControl=temp&0x01;
  humidityControl=temp&0x02;
  temperatureSP=EEPROM.read(1);
  humiditySP=EEPROM.read(2);
  //tempControl=true;
  //humidityControl=true;
  tempOn=false;
  humidityOn=false;
  
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  display.display();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  delay(1000);

  pinMode(BUTTON_A, INPUT_PULLUP);
  pinMode(BUTTON_B, INPUT_PULLUP);
  pinMode(BUTTON_C, INPUT_PULLUP);
  pinMode(TEMP_PIN, OUTPUT);
  pinMode(HUMIDITY_PIN, OUTPUT);

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  WiFi.setAutoReconnect(true);
  
  display.clearDisplay();
  display.setCursor(0,0);
  display.print("Connecting");
  display.display();
  while ((WiFi.status() != WL_CONNECTED) && millis()<8000)
  {
    delay(500);
    display.print(".");
    display.display();
  }

  dht.setup(DHT_PIN, DHTesp::DHT22);
  updateDelay=dht.getMinimumSamplingPeriod();

  buttons=0;
  lastbuttons=0;
  changedbuttons=0;

  server.on("/data.xml", HTTP_GET, handleXML);        // Call the 'handleRoot' function when a client requests URI "/"
  server.onNotFound(handleNotFound);           // When a client requests an unknown URI (i.e. something other than "/"), call 
  server.begin();
  
  lastUpdateTime=millis()-updateDelay;
}

void loop(){
  //check for current time to see if we need an update
  int currentTime=millis();

  server.handleClient();
  
  //check for buttons presses and process
  buttons=0x00|(!digitalRead(BUTTON_A))|(!digitalRead(BUTTON_B)<<1)|(!digitalRead(BUTTON_C)<<2);
  changedbuttons=buttons^lastbuttons;
  if (changedbuttons){
    if (changedbuttons&buttons&0x01){
      if (menu==0){
        menu=5;
      }
      else if (menu==1){
        if (temperatureSP<TEMP_MAX){
          temperatureSP++;
          saveCountdown=6;
        }
      }
      else if (menu==2){
        tempControl=!tempControl;
        saveCountdown=6;
      }
      else if (menu==3){
        if (humiditySP<HUMIDITY_MAX){
          humiditySP++;
          saveCountdown=6;
        }
      }
      else if (menu==4){
        humidityControl=!humidityControl;
        saveCountdown=6;
      }
      else if (menu==5){
        menu=0;
      }
    }
    if (changedbuttons&buttons&(0x01<<1)){
      if (menu<5){
        menu++;
        if (menu>4) menu=0;
      }
    }
    if (changedbuttons&buttons&(0x01<<2)) {
      if (menu==1){
        if (temperatureSP>TEMP_MIN){
          temperatureSP--;
          saveCountdown=6;
        }
      }
      else if (menu==2){
        tempControl=!tempControl;
        saveCountdown=6;
      }
      else if (menu==3){
        if (humiditySP>HUMIDITY_MIN){
          humiditySP--;
          saveCountdown=6;
        }
      }
      else if (menu==4){
        humidityControl=!humidityControl;
        saveCountdown=6;
      }
    }
    UpdateDisplay();
  }
  lastbuttons=buttons;

  //set countdown when not on menu 0.  After a period of time, go back to menu 0;
  if (menu==0) {menuCountdown=0;}
  else if (buttons) {menuCountdown=16;}
  //else if (menuCountdown==0 && !buttons) menuCountdown=11;
  
  
  //update readings and screen
  if ((currentTime-lastUpdateTime)>=updateDelay) {
    lastUpdateTime+=updateDelay;
    humidity = dht.getHumidity();
    temperature = dht.toFahrenheit(dht.getTemperature());
    if (saveCountdown==1){
      EEPROM.write(0,0x00|tempControl|(humidityControl<<1));
      EEPROM.write(1,temperatureSP);
      EEPROM.write(2,humiditySP);
      EEPROM.commit();
      saved=true;
    }
    else{
      saved=false;
    }
    
    if (saveCountdown>0) saveCountdown--;

    if (menuCountdown>0) {menuCountdown--;}
    if (menuCountdown==0) {menu=0;}
    
    //Control Code
    if (saveCountdown==0){
      if (tempControl){
        if (tempOn && temperature-temperatureSP>=TEMP_DEADBAND){
          tempOn=false;
        }
        if (!tempOn && temperatureSP-temperature>=TEMP_DEADBAND){
          tempOn=true;
        }
      }
      else {
        tempOn=false;
      }

      if (humidityControl){
        if (humidityOn&& humidity-humiditySP>=HUMIDITY_DEADBAND){
          humidityOn=false;
        }
        if (!humidityOn && humiditySP-humidity>=HUMIDITY_DEADBAND){
          humidityOn=true;
        }
      }
      else{
        humidityOn=false;
      }
    }
    digitalWrite(TEMP_PIN,tempOn);
    digitalWrite(HUMIDITY_PIN,humidityOn);
    

    
    UpdateDisplay();
    //if (WiFi.status() != WL_CONNECTED) WiFi.reconnect();
  }
  
  yield();
}

void UpdateDisplay(){
  display.clearDisplay();
  if (menu!=5){
    display.setTextSize(1);
    display.setCursor(0,9);
    if (saved) display.print("*");
    display.setCursor(40,9);
    display.print("MV");
    display.setCursor(72,9);
    display.print("SP");
    display.setCursor(95,9);
    display.print("Ctrl");
    display.setCursor(0,17);
    display.print("Temp");
    display.setCursor(30,17);
    display.print(temperature,1);
    display.print((char)247);
    display.setCursor(69,17);
    display.print(temperatureSP);
    display.print((char)247);
    display.setCursor(98,17);
    display.print((tempControl ? "On" : "Off"));
    display.setCursor(122,17);
    display.print(int(tempOn));
    display.setCursor(0,25);
    display.print("Hum.");
    display.setCursor(30,25);
    display.print(humidity,1);
    display.print("%");
    display.setCursor(69,25);
    display.print(humiditySP);
    display.print("%");
    display.setCursor(98,25);
    display.print((humidityControl ? "On" : "Off"));
    display.setCursor(122,25);
    display.print(int(humidityOn));
    display.setCursor(0,0);
    display.setTextSize(1);
    display.print(WiFi.localIP());
    display.setCursor(90,0);
    display.setTextSize(1);
    display.print(WiFi.RSSI());
    display.print("dBm");
  }
  if (menu==1){
    uint8_t x=69;
    uint8_t y=17;
    display.fillTriangle(x-2,y+4,x-4,y+2,x-4,y+6,WHITE);
  }
  if (menu==2){
    uint8_t x=98;
    uint8_t y=17;
    display.fillTriangle(x-2,y+4,x-4,y+2,x-4,y+6,WHITE);
  }
  if (menu==3){
    uint8_t x=69;
    uint8_t y=25;
    display.fillTriangle(x-2,y+4,x-4,y+2,x-4,y+6,WHITE);
  }
  if (menu==4){
    uint8_t x=98;
    uint8_t y=25;
    display.fillTriangle(x-2,y+4,x-4,y+2,x-4,y+6,WHITE);
  }
  if (menu==5){
    display.setCursor(0,0);
    //display.print(": ");
    display.print(WiFi.SSID());
    display.setCursor(0,9);
    display.print("Signal: ");
    display.print(WiFi.RSSI());
    display.print("dBm");
    display.setCursor(0,17);
    display.print("IP: ");
    display.print(WiFi.localIP());
    display.setCursor(0,25);
    display.print("MAC:");
    display.print(WiFi.macAddress());
  }
  display.display();
}


/*******Web Functions******/

void handleXML(){
  String xml;
  xml="<?xml version='1.0' encoding='UTF-8'?>";
  xml+="<data>";
  xml+="<temperature>"+String(temperature)+"</temperature>";
  xml+="<temperatureSP>"+String(temperatureSP)+"</temperatureSP>";
  xml+="<tempControl>"+String(tempControl)+"</tempControl>";
  xml+="<tempOn>"+String(tempOn)+"</tempOn>";
  xml+="<humidity>"+String(humidity)+"</humidity>";
  xml+="<humiditySP>"+String(humiditySP)+"</humiditySP>";
  xml+="<humidityControl>"+String(humidityControl)+"</humidityControl>";
  xml+="<humidityOn>"+String(humidityOn)+"</humidityOn>";
  xml+="</data>";
  server.send(200, "text/xml", xml);
}

void handleNotFound(){
  server.send(404, "text/plain", "404: Not found");
}

