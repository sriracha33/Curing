#include <DHTesp.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <ESP8266WiFi.h>

#include "Wifi.h"
/*Define WIFI SSID and Password if not using separate header file.
#define WIFI_SSID "ssid"
#define WIFI_PASSWORD "password"
*/

DHTesp dht;
Adafruit_SSD1306 display = Adafruit_SSD1306(128, 32, &Wire);

float humidity;
float temperature;
int lastUpdateTime;
int updateDelay;
uint8_t buttons,lastbuttons;

//ESP8266
#define BUTTON_A  0
#define BUTTON_B 16
#define BUTTON_C  2

void setup()
{
  Serial.begin(115200);
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
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    display.print(".");
    display.display();
  }
  display.clearDisplay();
  display.setCursor(0,0);
  display.print(WiFi.localIP());
  display.display();

  dht.setup(12, DHTesp::DHT22);
  updateDelay=dht.getMinimumSamplingPeriod();

  buttons=0;
  lastbuttons=0;
  
  lastUpdateTime=millis()-updateDelay;
}

void loop(){
  int currentTime=millis();
  buttons=0x00|(!digitalRead(BUTTON_A))|(!digitalRead(BUTTON_B)<<1)|(!digitalRead(BUTTON_C)<<2);
  if ((currentTime-lastUpdateTime)>=updateDelay) {
    lastUpdateTime+=updateDelay;
    humidity = dht.getHumidity();
    temperature = dht.toFahrenheit(dht.getTemperature());
    UpdateDisplay();
    Serial.println(buttons);
  }
}

void UpdateDisplay(){
  display.clearDisplay();
  display.setTextSize(2);
  display.setCursor(0,0);
  display.print(temperature,1);
  display.print((char)247);
  display.setCursor(65,0);
  display.print(humidity,1);
  display.print("%");
  display.setCursor(0,25);
  display.setTextSize(1);
  display.print(WiFi.localIP());
  display.display();
}






