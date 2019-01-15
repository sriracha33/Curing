#include <Wire.h>
#include <EEPROM.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_SHT31.h>
#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include <ESP8266WebServer.h>


#include "WifiSettings.h"
/*Define WIFI SSID and Password if not using separate header file. Add up to 3.
#define WIFI_SSID "ssid"
#define WIFI_PASSWORD "password"
#define WIFI_SSID2 "ssid2"
#define WIFI_PASSWORD2 "password2"
#define WIFI_SSID3 "ssid3"
#define WIFI_PASSWORD3 "password3"
*/

//configuration defines
#define TEMP_DEADBAND 2
#define HUMIDITY_DEADBAND 3
#define TEMP_MAX 90
#define TEMP_MIN 40
#define HUMIDITY_MAX 90
#define HUMIDITY_MIN 20
#define UPDATE_DELAY 1000
#define CHANGE_TIMEOUT 5000
#define MENU_TIMEOUT 30000

ESP8266WiFiMulti wifiMulti;
Adafruit_SHT31 sht31 = Adafruit_SHT31();
Adafruit_SSD1306 display = Adafruit_SSD1306(128, 32, &Wire);

ESP8266WebServer server(80);    // Create a webserver object that listens for HTTP request on port 80
void handleXML();             // function prototypes for XML file.
void handleCommand();         // function prototypes for handling commands.
void handleNotFound();

float humidity;
float temperature;
uint32_t currentTime,lastUpdateTime,lastMenuTime,lastChangeTime;
uint8_t buttons,lastbuttons,changedbuttons;
uint8_t menu=0;
uint8_t humiditySP,temperatureSP;
boolean tempOn,humidityOn;
boolean tempControl,humidityControl;
boolean saved=false;
String xml;
int32_t rssi;
IPAddress ip;


//ESP8266
#define BUTTON_A  0
#define BUTTON_B 16
#define BUTTON_C  2

#define TEMP_PIN 12
#define HUMIDITY_PIN 13

void setup(){
  Serial.begin(115200);
  
  delay(500);  //needed so display will start up on power loss without a reset

  //read saved config data from eeprom
  EEPROM.begin(4);
  byte temp=EEPROM.read(0);
  tempControl=temp&0x01;
  humidityControl=temp&0x02;
  temperatureSP=EEPROM.read(1);
  humiditySP=EEPROM.read(2);
  tempOn=false;
  humidityOn=false;

  //startup display
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  display.display();
  display.setTextSize(1);
  display.setTextColor(WHITE);

  //setup pins for buttons and control pins for humidity and temp relays
  pinMode(BUTTON_A, INPUT_PULLUP);
  pinMode(BUTTON_B, INPUT_PULLUP);
  pinMode(BUTTON_C, INPUT_PULLUP);
  pinMode(TEMP_PIN, OUTPUT);
  pinMode(HUMIDITY_PIN, OUTPUT);

  //start wifi
  #if defined(WIFI_SSID) && defined(WIFI_PASSWORD)
    wifiMulti.addAP(WIFI_SSID, WIFI_PASSWORD);
  #endif
  #if defined(WIFI_SSID2) && defined(WIFI_PASSWORD2)
    wifiMulti.addAP(WIFI_SSID2, WIFI_PASSWORD2);
  #endif
  #if defined(WIFI_SSID3) && defined(WIFI_PASSWORD3)
    wifiMulti.addAP(WIFI_SSID3, WIFI_PASSWORD3);
  #endif

   
  display.clearDisplay();
  display.setCursor(0,0);
  display.print("Connecting");
  display.display();
  
  //wait a while for wifi to connect.  needed?
  while ((wifiMulti.run() != WL_CONNECTED) && millis()<8000)
  {
    delay(500);
    display.print(".");
    display.display();
  }

  //set up temperature sensor. 0x44 or 0x45
  sht31.begin(0x44);

  //initialize buttons press states
  buttons=0;
  lastbuttons=0;
  changedbuttons=0;

  //set up web server
  server.on("/data.xml", HTTP_GET, handleXML);      // Call the 'handleXML' function when a client requests URI "/data.xml"
  server.on("/command.cgi", HTTP_POST, handleCommand);  // Call the 'handleRoot' function when a client requests URI "/command with post parameters"
  server.onNotFound(handleNotFound);                // When a client requests an unknown URI (i.e. something other than above), call 
  server.begin();
  
  //reserve space for xml response.
  xml.reserve(512);
  
  lastUpdateTime=millis()-UPDATE_DELAY;
}

void loop(){
  //check for current time to see if we need an update
  currentTime=millis();

  //don't let currentTime be zero.  it will be assigned to other times.  Zero will mean not set
  if (currentTime==0){
    currentTime++;
  }

  wifiMulti.run();
  server.handleClient();
  
  //check for buttons presses and process
  
  //get state of buttons
  buttons=0x00|(!digitalRead(BUTTON_A))|(!digitalRead(BUTTON_B)<<1)|(!digitalRead(BUTTON_C)<<2);
  //check if bottons have changed since last check
  changedbuttons=buttons^lastbuttons;
  
  //if they have changed, process each newly pressed button.
  if (changedbuttons){
    
    //Button A. Usually the up button.  Also toggles network screen from main menu.
    if (changedbuttons&buttons&0x01){
      if (menu==0){
        menu=5;
      }
      else if (menu==1){
        if (temperatureSP<TEMP_MAX){
          temperatureSP++;
          lastChangeTime=currentTime;
        }
      }
      else if (menu==2){
        tempControl=!tempControl;
        lastChangeTime=currentTime;
      }
      else if (menu==3){
        if (humiditySP<HUMIDITY_MAX){
          humiditySP++;
          lastChangeTime=currentTime;
        }
      }
      else if (menu==4){
        humidityControl=!humidityControl;
        lastChangeTime=currentTime;
      }
      else if (menu==5){
        menu=0;
      }
    }

    //Button B.  Menu change button.
    if (changedbuttons&buttons&(0x01<<1)){
      if (menu<5){
        menu++;
        if (menu>4) menu=0;
      }
    }

    //Button C. Usually the down button.
    if (changedbuttons&buttons&(0x01<<2)) {
      if (menu==1){
        if (temperatureSP>TEMP_MIN){
          temperatureSP--;
          lastChangeTime=currentTime;;
        }
      }
      else if (menu==2){
        tempControl=!tempControl;
        lastChangeTime=currentTime;;
      }
      else if (menu==3){
        if (humiditySP>HUMIDITY_MIN){
          humiditySP--;
          lastChangeTime=currentTime;;
        }
      }
      else if (menu==4){
        humidityControl=!humidityControl;
        lastChangeTime=currentTime;;
      }
    }
    UpdateDisplay();
  }
  lastbuttons=buttons;

  //start timing when not on menu 0.  After a period of MENU_TIMEOUT, go back to menu 0;
  if (menu==0) {lastMenuTime=0;}
  else if (buttons) {lastMenuTime=currentTime;}
  
  //update readings and screen.  Done after every UPDATE_DELAY in ms.
  if ((currentTime-lastUpdateTime)>=UPDATE_DELAY) {
    lastUpdateTime+=UPDATE_DELAY;
    xml="";
    temperature=sht31.readTemperature()*1.8+32;
    humidity=sht31.readHumidity();
    rssi=WiFi.RSSI();
    ip=WiFi.localIP();

    //save current values is CHANGE_TIMEOUT has passed
    if ((currentTime-lastChangeTime)>=CHANGE_TIMEOUT && lastChangeTime!=0) {  
      lastChangeTime=0;
      EEPROM.write(0,0x00|tempControl|(humidityControl<<1));
      EEPROM.write(1,temperatureSP);
      EEPROM.write(2,humiditySP);
      EEPROM.commit();
      saved=true;
    }
    else{
      saved=false;
    }

    //when MENU_TIMEOUT has elapsed from last menu button, go to main menu.
    if ((currentTime-lastMenuTime)>=MENU_TIMEOUT && lastMenuTime!=0) {
      lastMenuTime=0;
      menu=0;
    }
    
    /*Control Code*/

    //only process if no recent changes made.
    //if (saveCountdown==0){
    if (lastChangeTime==0){
      //process temperature control
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

      //process humidity control
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
  //update display with current values based on current menu.
  
  display.clearDisplay();
  
  //base display.  Shows summary of control values
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
    display.print(ip);
    display.setCursor(90,0);
    display.setTextSize(1);
    display.print(rssi);
    display.print("dBm");
    
  }

  //edit temperature setpoint
  if (menu==1){
    uint8_t x=69;
    uint8_t y=17;
    display.fillTriangle(x-2,y+4,x-4,y+2,x-4,y+6,WHITE);
  }

  //toggle temperature control
  if (menu==2){
    uint8_t x=98;
    uint8_t y=17;
    display.fillTriangle(x-2,y+4,x-4,y+2,x-4,y+6,WHITE);
  }

  //humidity setpoint
  if (menu==3){
    uint8_t x=69;
    uint8_t y=25;
    display.fillTriangle(x-2,y+4,x-4,y+2,x-4,y+6,WHITE);
  }

  //toggle humidity control
  if (menu==4){
    uint8_t x=98;
    uint8_t y=25;
    display.fillTriangle(x-2,y+4,x-4,y+2,x-4,y+6,WHITE);
  }

  //network page.
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
  if (xml.length()==0){
    //build xml with all required data and send it in response.
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
  }
  else{
    //do nothing.  data hasn't changed
  }
  server.send(200, "text/xml", xml);
}

void handleCommand(){
  //make sure command and value are proper
  if( !server.hasArg("command") || !server.hasArg("value") || server.arg("command") == NULL || server.arg("value") == NULL || server.arg("command").toInt() == 0){
    server.send(400, "text/plain", "400: Invalid Request");         // The request is invalid, so send HTTP status 400
    return;
  }

  //get integer value of command
  int command=server.arg("command").toInt();

  //process command.  If it is zero or undefined command, return error.
  if (command==1){tempControl=(boolean)server.arg("value").toInt();lastChangeTime=currentTime;}
  else if (command==2){temperatureSP=constrain(server.arg("value").toFloat(),TEMP_MIN,TEMP_MAX);lastChangeTime=currentTime;}
  else if (command==3){humidityControl=(boolean)server.arg("value").toInt();lastChangeTime=currentTime;}
  else if (command==4){humiditySP=constrain(server.arg("value").toFloat(),HUMIDITY_MIN,HUMIDITY_MAX);lastChangeTime=currentTime;}
  else {
    server.send(400, "text/plain", "400: Invalid Command");  
    return;
  }

  //send success response
  server.send(200, "text/plain", "Success");
}

void handleNotFound(){
  //any unknown page requested, return not found.
  server.send(404, "text/plain", "404: Not found");
}

