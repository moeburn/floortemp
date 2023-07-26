
#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <AsyncElegantOTA.h>
#include <BlynkSimpleEsp8266.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <RGBLed.h>

#define RGBPIN2 15
#define RGBPIN1 0
#define RGBPIN3 12


RGBLed led(RGBPIN1, RGBPIN2, RGBPIN3, RGBLed::COMMON_CATHODE);

// GPIO where the DS18B20 is connected to
const int oneWireBus = 4;     

// Setup a oneWire instance to communicate with any OneWire devices
OneWire oneWire(oneWireBus);

// Pass our oneWire reference to Dallas Temperature sensor 
DallasTemperature sensors(&oneWire);

const char* ssid = "mikesnet";
const char* password = "springchicken";

char auth[] = "yRR6LtN228UywT1U4I6yDOzuZZzWcXYs"; //BLYNK

unsigned long millisBlynk = 0;

AsyncWebServer server(80);

WidgetTerminal terminal(V0);

int zebraR, zebraG, zebraB, sliderValue;
int menuValue = 2;
float  pmR, pmG, pmB;
bool rgbON = true;

BLYNK_WRITE(V15)
{
   menuValue = param.asInt(); // assigning incoming value from pin V1 to a variable
}

BLYNK_WRITE(V16)
{
     zebraR = param[0].asInt();
     zebraG = param[1].asInt();
     zebraB = param[2].asInt();
}

BLYNK_WRITE(V17)
{
      led.brightness(param.asInt()); // assigning incoming value from pin V1 to a variable   
}

void setup(void) {
     pinMode(RGBPIN1,HIGH);  // Blue led Pin Connected To D0 Pin   
   pinMode(RGBPIN2,HIGH);  // Green Led Pin Connected To D1 Pin   
   pinMode(RGBPIN3,HIGH);  // Red Led Connected To D2 Pin    

  Serial.begin(115200);
  WiFi.mode(WIFI_STA);
    WiFi.setPhyMode(WIFI_PHY_MODE_11B);
  WiFi.begin(ssid, password);
  Serial.println("");

  // Wait for connection
  while (WiFi.status() != WL_CONNECTED) {
    led.crossFade(RGBLed::RED, RGBLed::BLUE, 5, 500);  // Fade from RED to GREEN in 5 steps during 100ms 
    Serial.print(".");
  }
  led.crossFade(RGBLed::BLUE, RGBLed::GREEN, 20, 2000);  // Fade from RED to GREEN in 5 steps during 100ms 
    // Start the DS18B20 sensor
  sensors.begin();
      Blynk.config(auth, IPAddress(192, 168, 50, 197), 8080);
    Blynk.connect();
  terminal.println("");
  terminal.print("Connected to ");
  terminal.println(ssid);
  terminal.print("IP address: ");
  terminal.println(WiFi.localIP());

  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(200, "text/plain", "Monkeys fly out of my butt");
  });

  AsyncElegantOTA.begin(&server);    // Start ElegantOTA
  server.begin();
  terminal.println("HTTP server started");
  terminal.flush();

    terminal.println("GREEN");
led.setColor(RGBLed::GREEN); 
    terminal.flush();
    delay(1000);  
    // GREEN LED ON  
    terminal.println("RED");
led.setColor(RGBLed::RED);
    terminal.flush();
    delay(1000);  
    // RED LED ON  
    terminal.println("BLUE");
led.setColor(RGBLed::BLUE);
    terminal.flush();
    delay(1000);  
}

void loop(void) {
Blynk.run();
if (menuValue == 1) {
  led.setColor(zebraR, zebraG, zebraB);
}
 
    if  (millis() - millisBlynk >= 30000)  //if it's been 30 seconds OR we just booted up, skip the 30 second wait
    {
        millisBlynk = millis();
        sensors.requestTemperatures(); 
        float temperatureC = sensors.getTempCByIndex(0);
        terminal.println(temperatureC);
        terminal.flush();
        if (temperatureC > -126) {Blynk.virtualWrite(V1, temperatureC);}
    }
}
