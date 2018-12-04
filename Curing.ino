#include <DHTesp.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <ESP8266WiFi.h>
#include <EEPROM.h>

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

float humidity;
float temperature;
int lastUpdateTime;
uint8_t saveCountdown=0;
int updateDelay;
uint8_t buttons,lastbuttons,changedbuttons;
uint8_t menu=0;
uint8_t humiditySP,temperatureSP;

//ESP8266
#define BUTTON_A  0
#define BUTTON_B 16
#define BUTTON_C  2

void setup()
{
  Serial.begin(115200);
  
  delay(50);  //needed so display will start up on power loss without a reset

  EEPROM.begin(4);
  temperatureSP=EEPROM.read(0);
  humiditySP=EEPROM.read(1);
  
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  display.display();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  delay(1000);

  pinMode(BUTTON_A, INPUT_PULLUP);
  pinMode(BUTTON_B, INPUT_PULLUP);
  pinMode(BUTTON_C, INPUT_PULLUP);

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  WiFi.setAutoReconnect(true);
  
  display.clearDisplay();
  display.setCursor(0,0);
  display.print("Connecting");
  display.display();
  while ((WiFi.status() != WL_CONNECTED) && millis()<5000)
  {
    delay(500);
    display.print(".");
    display.display();
  }

  dht.setup(12, DHTesp::DHT22);
  updateDelay=dht.getMinimumSamplingPeriod();

  buttons=0;
  lastbuttons=0;
  changedbuttons=0;

  //temperatureSP=60;
  //humiditySP=65;
  
  lastUpdateTime=millis()-updateDelay;
}

void loop(){
  //check for current time to see if we need an update
  int currentTime=millis();
  
  //check for buttons presses and process
  buttons=0x00|(!digitalRead(BUTTON_A))|(!digitalRead(BUTTON_B)<<1)|(!digitalRead(BUTTON_C)<<2);
  changedbuttons=buttons^lastbuttons;
  if (changedbuttons){
    if (changedbuttons&buttons&0x01){
      if (menu==1){
        if (temperatureSP<TEMP_MAX){
          temperatureSP++;
          saveCountdown=6;
        }
      }
      if (menu==2){
        if (humiditySP<HUMIDITY_MAX){
          humiditySP++;
          saveCountdown=6;
        }
      }
    }
    if (changedbuttons&buttons&(0x01<<1)){
      menu++;
      if (menu>3) menu=0;
    }
    if (changedbuttons&buttons&(0x01<<2)) {
      if (menu==1){
        if (temperatureSP>TEMP_MIN){
          temperatureSP--;
          saveCountdown=6;
        }
      }
      if (menu==2){
        if (humiditySP>HUMIDITY_MIN){
          humiditySP--;
          saveCountdown=6;
        }
      }
    }
    UpdateDisplay();
  }
  lastbuttons=buttons;

  //Control Code
  if (saveCountdown==0){
    //do stuff
  }
  
  //update readings and screen
  if ((currentTime-lastUpdateTime)>=updateDelay) {
    lastUpdateTime+=updateDelay;
    humidity = dht.getHumidity();
    temperature = dht.toFahrenheit(dht.getTemperature());
    if (saveCountdown==1){
      EEPROM.write(0,temperatureSP);
      EEPROM.write(1,humiditySP);
      EEPROM.commit();
      Serial.println("save");
    }
    saveCountdown--;
    UpdateDisplay();
    if (WiFi.status() != WL_CONNECTED) WiFi.reconnect();
  }
  
  yield();
}

void UpdateDisplay(){
  display.clearDisplay();
  if (menu==0){
    display.setTextSize(1);
    display.setCursor(10,9);
    display.print("Temp.");
    display.setCursor(10,17);
    display.print(temperature,1);
    display.print((char)247);
    display.setCursor(16,25);
    display.print(temperatureSP);
    display.print((char)247);
    display.setCursor(66,9);
    display.print("Humidity");
    display.setCursor(75,17);
    display.print(humidity,1);
    display.print("%");
    display.setCursor(81,25);
    display.print(humiditySP);
    display.print("%");
    display.setCursor(0,0);
    display.setTextSize(1);
    display.print(WiFi.localIP());
    display.setCursor(90,0);
    display.setTextSize(1);
    display.print(WiFi.RSSI());
    display.print("dBm");
  }
  else if (menu==1){
    display.setTextSize(1);
    display.setCursor(10,9);
    display.print("Temp.");
    display.setCursor(10,17);
    display.print(temperature,1);
    display.print((char)247);
    display.setCursor(16,25);
    display.print(temperatureSP);
    display.print((char)247);
  }
  else if (menu==2){
    display.setCursor(66,9);
    display.print("Humidity");
    display.setCursor(75,17);
    display.print(humidity,1);
    display.print("%");
    display.setCursor(81,25);
    display.print(humiditySP);
    display.print("%");
  }
  else if (menu==3){
    display.setCursor(0,0);
    display.print(WiFi.localIP());
    display.setCursor(90,0);
    display.print(WiFi.RSSI());
    display.print("dBm");
  }
  display.display();
}






