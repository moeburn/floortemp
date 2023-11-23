
#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <AsyncElegantOTA.h>
#include <BlynkSimpleEsp8266.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <RGBLed.h>
#include "time.h"
#include <Average.h>
#include <ESP8266HTTPClient.h>
#include <Arduino_JSON.h>

String openWeatherMapApiKey = "25a3e27db184bc57cf33bccc752291c9";
// Example:
//String openWeatherMapApiKey = "bd939aa3d23ff33d3c8f5dd1dd435";

// Replace with your country code and city
String city = "Stratford";
String countryCode = "CA";

String jsonBuffer;

Average<float> pm1Avg(30);
Average<float> pm25Avg(30);
Average<float> pm10Avg(30);
Average<float> wifiAvg(30);
Average<float> sunAvg(30);
bool rapidfire = false;
float pmavgholder;
float temperatureC;

#include <PMserial.h> // Arduino library for PM sensors with serial interface
#if defined(USE_HWSERIAL2)
#define MSG "PMSx003 on HardwareSerial2"
SerialPM pms(PMSx003, Serial2); // PMSx003, UART
#elif defined(USE_HWSERIAL1)
#define MSG "PMSx003 on HardwareSerial1"
SerialPM pms(PMSx003, Serial1); // PMSx003, UART
#else
#define MSG "PMSx003 on HardwareSerial"
SerialPM pms(PMSx003, Serial); // PMSx003, UART
#endif

#define RGBPIN2 15
#define RGBPIN1 0
#define RGBPIN3 12

const char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec = -14400;   //Replace with your GMT offset (seconds)
const int   daylightOffset_sec = 0;  //Replace with your daylight offset (seconds)


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
char remoteAuth[] = "X_pnRUFOab29d3aNrScsKq1dryQYdTw7";
char remoteAuth2[] = "8_-CN2rm4ki9P3i_NkPhxIbCiKd5RXhK";

unsigned long millisBlynk = 0;
unsigned long millisAvg = 0;
unsigned long millisSun = 0;

AsyncWebServer server(80);

WidgetTerminal terminal(V0);

int firstvalue = 1;
int zebraR, zebraG, zebraB, sliderValue;
int menuValue = 2;
float  pmR, pmG, pmB;
float old1p0, old2p5, old10, new1p0, new2p5, new10;

double inetTemp, inetWindspeed, inetWinddeg, inetWindgust;
bool rgbON = true;

String httpGETRequest(const char* serverName) {
  WiFiClient client;
  HTTPClient http;
    
  // Your Domain name with URL path or IP address with path
  http.begin(client, serverName);
  
  // Send HTTP POST request
  int httpResponseCode = http.GET();
  
  String payload = "{}"; 
  
  if (httpResponseCode>0) {
    Serial.print("HTTP Response code: ");
    Serial.println(httpResponseCode);
    payload = http.getString();
  }
  else {
    Serial.print("Error code: ");
    Serial.println(httpResponseCode);
  }
  // Free resources
  http.end();

  return payload;
}



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

WidgetBridge bridge1(V50);
WidgetBridge bridge2(V60);

void setup(void) {
  
     pinMode(RGBPIN1,HIGH);  // Blue led Pin Connected To D0 Pin   
   pinMode(RGBPIN2,HIGH);  // Green Led Pin Connected To D1 Pin   
   pinMode(RGBPIN3,HIGH);  // Red Led Connected To D2 Pin    

  Serial.begin(9600);
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
    delay(500);
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  delay(500);
    // Start the DS18B20 sensor
  sensors.begin();
  sensors.requestTemperatures(); 
      Blynk.config(auth, IPAddress(192, 168, 50, 197), 8080);
    Blynk.connect();
  terminal.println("=========Fv0.3============");
  terminal.print("Connected to ");
  terminal.println(ssid);
  terminal.print("IP address: ");
  terminal.println(WiFi.localIP());

  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(200, "text/plain", "I am floortemp, Monkeys fly out of my butt");
  });

  AsyncElegantOTA.begin(&server);    // Start ElegantOTA
  server.begin();
  terminal.println("HTTP server started");
  printLocalTime();
  
  pms.init();
    led.setColor(RGBLed::RED);
    terminal.flush();
    delay(500);  
    led.setColor(RGBLed::GREEN); 
    terminal.flush();
    delay(500);  
    // GREEN LED ON  

    // RED LED ON  
    led.setColor(RGBLed::BLUE);
    delay(100);  
    led.brightness(RGBLed::BLUE, 90);
    delay(100);  
    led.brightness(RGBLed::BLUE, 80);
    delay(100);  
    led.brightness(RGBLed::BLUE, 70);
    delay(100);  
    led.brightness(RGBLed::BLUE, 60);
    delay(100);  
    led.brightness(RGBLed::BLUE, 50);
    delay(100);  
    led.brightness(RGBLed::BLUE, 40);
    delay(100);  
    led.brightness(RGBLed::BLUE, 30);
    delay(100);  
    led.brightness(RGBLed::BLUE, 20);
    delay(100);  
    led.brightness(RGBLed::BLUE, 10);
    delay(100);  
    led.brightness(RGBLed::BLUE, 0);
    terminal.flush();


}

void loop(void) {
 
Blynk.run();
if (menuValue == 1) {
  led.setColor(zebraR, zebraG, zebraB);
}
 
    if  (millis() - millisBlynk >= 30000)  //if it's been 30 seconds 
    {
        readPMS();
        millisBlynk = millis();
        doWeather();
        sensors.requestTemperatures(); 
        sensors.requestTemperatures(); 
        temperatureC = sensors.getTempCByIndex(0);
        if ((temperatureC > -126) && (temperatureC != 85)) {Blynk.virtualWrite(V1, temperatureC);
        bridge1.virtualWrite(V54, temperatureC);
        bridge2.virtualWrite(V54, temperatureC);
        }
        Blynk.virtualWrite(V2, pm1Avg.mean());
        pmavgholder = pm25Avg.mean();
        Blynk.virtualWrite(V3, pmavgholder);
        bridge1.virtualWrite(V51, pmavgholder);
        bridge2.virtualWrite(V51, pmavgholder);
        Blynk.virtualWrite(V4, pm10Avg.mean());
        Blynk.virtualWrite(V5, pms.n0p3);
        Blynk.virtualWrite(V6, pms.n0p5);
        Blynk.virtualWrite(V7, pms.n1p0);
        Blynk.virtualWrite(V8, pms.n2p5);
        Blynk.virtualWrite(V9, pms.n5p0);
        Blynk.virtualWrite(V10, pms.n10p0);
        Blynk.virtualWrite(V11, wifiAvg.mean());
        Blynk.virtualWrite(V15, inetTemp);
        Blynk.virtualWrite(V16, inetWindspeed);
        Blynk.virtualWrite(V17, inetWindgust);
        Blynk.virtualWrite(V18, inetWinddeg);
        bridge1.virtualWrite(V55, inetTemp);
        bridge1.virtualWrite(V56, inetWindspeed);
        bridge1.virtualWrite(V57, inetWindgust);
        bridge1.virtualWrite(V58, inetWinddeg);
        bridge2.virtualWrite(V55, inetTemp);
        bridge2.virtualWrite(V56, inetWindspeed);
        bridge2.virtualWrite(V57, inetWindgust);
        bridge2.virtualWrite(V58, inetWinddeg);
        if (!rapidfire) {Blynk.virtualWrite(V12, sunAvg.mean());}
    }
    
    if  (millis() - millisAvg >= 1000)  //if it's been 1 second
    {
        wifiAvg.push(WiFi.RSSI());
        sunAvg.push(analogRead(A0));
        readPMS();
        millisAvg = millis();
    }
        if (rapidfire) 
    {
        Blynk.virtualWrite(V12, analogRead(A0));
    }
    
}

void readPMSverbose(void){
    pms.read();
  if (pms)
  { // successfull read
#if defined(ESP8266) || defined(ESP32)
    // print formatted results
    terminal.printf("PM1.0 %2d, PM2.5 %2d, PM10 %2d [ug/m3]\n",
                  pms.pm01, pms.pm25, pms.pm10);

    if (pms.has_number_concentration())
      terminal.printf("N0.3 %4d, N0.5 %3d, N1.0 %2d, N2.5 %2d, N5.0 %2d, N10 %2d [#/100cc]\n",
                    pms.n0p3, pms.n0p5, pms.n1p0, pms.n2p5, pms.n5p0, pms.n10p0);

    if (pms.has_temperature_humidity() || pms.has_formaldehyde())
      terminal.printf("%5.1f °C, %5.1f %%rh, %5.2f mg/m3 HCHO\n",
                    pms.temp, pms.rhum, pms.hcho);
#else
    // print the results
    terminal.print(F("PM1.0 "));
    terminal.print(pms.pm01);
    terminal.print(F(", "));
    terminal.print(F("PM2.5 "));
    terminal.print(pms.pm25);
    terminal.print(F(", "));
    terminal.print(F("PM10 "));
    terminal.print(pms.pm10);
    terminal.println(F(" [ug/m3]"));

    if (pms.has_number_concentration())
    {
      terminal.print(F("N0.3 "));
      terminal.print(pms.n0p3);
      terminal.print(F(", "));
      terminal.print(F("N0.5 "));
      terminal.print(pms.n0p5);
      terminal.print(F(", "));
      terminal.print(F("N1.0 "));
      terminal.print(pms.n1p0);
      terminal.print(F(", "));
      terminal.print(F("N2.5 "));
      terminal.print(pms.n2p5);
      terminal.print(F(", "));
      terminal.print(F("N5.0 "));
      terminal.print(pms.n5p0);
      terminal.print(F(", "));
      terminal.print(F("N10 "));
      terminal.print(pms.n10p0);
      terminal.println(F(" [#/100cc]"));
    }

    if (pms.has_temperature_humidity() || pms.has_formaldehyde())
    {
      terminal.print(pms.temp, 1);
      terminal.print(F(" °C"));
      terminal.print(F(", "));
      terminal.print(pms.rhum, 1);
      terminal.print(F(" %rh"));
      terminal.print(F(", "));
      terminal.print(pms.hcho, 2);
      terminal.println(F(" mg/m3 HCHO"));
    }
#endif
  }
  else
  { // something went wrong
    switch (pms.status)
    {
    case pms.OK: // should never come here
      break;     // included to compile without warnings
    case pms.ERROR_TIMEOUT:
      terminal.println(F(PMS_ERROR_TIMEOUT));
      break;
    case pms.ERROR_MSG_UNKNOWN:
      terminal.println(F(PMS_ERROR_MSG_UNKNOWN));
      break;
    case pms.ERROR_MSG_HEADER:
      terminal.println(F(PMS_ERROR_MSG_HEADER));
      break;
    case pms.ERROR_MSG_BODY:
      terminal.println(F(PMS_ERROR_MSG_BODY));
      break;
    case pms.ERROR_MSG_START:
      terminal.println(F(PMS_ERROR_MSG_START));
      break;
    case pms.ERROR_MSG_LENGTH:
      terminal.println(F(PMS_ERROR_MSG_LENGTH));
      break;
    case pms.ERROR_MSG_CKSUM:
      terminal.println(F(PMS_ERROR_MSG_CKSUM));
      break;
    case pms.ERROR_PMS_TYPE:
      terminal.println(F(PMS_ERROR_PMS_TYPE));
      break;
    }
  }
terminal.flush();
}

void printLocalTime()
{
  time_t rawtime;
  struct tm * timeinfo;
  time (&rawtime);
  timeinfo = localtime (&rawtime);
  terminal.print("-");
  terminal.print(asctime(timeinfo));
  terminal.print(" - ");
  terminal.flush();
}

void readPMS(void){
    pms.read();

        new1p0 = pms.pm01;
        new2p5 = pms.pm25;
        new10 = pms.pm10;
                if (firstvalue == 0)  //do not do this on the first run
        {
            if (new1p0 > 200) {new1p0 = old1p0;} //check for data spikes in particle counter, ignore data that is >200
            if (new2p5 > 200) {new2p5 = old2p5;} //data spikes ruin pretty graph
            if (new10 > 200) {new10 = old10;}
            if (new1p0 - old1p0 > 50) {new1p0 = old1p0;} //also ignore data that is >50 off from last data
            if (new2p5 - old2p5 > 50) {new2p5 = old2p5;}
            if (new10 - old10 > 50) {new10 = old10;}
        }
        pm1Avg.push(new1p0);
        pm25Avg.push(new2p5);
        pm10Avg.push(new10);
        old1p0 = new1p0; //reset data spike check variable
        old2p5 = new2p5;
        old10 = new10;
        firstvalue = 0;


}

BLYNK_WRITE(V0)
{
    if (String("help") == param.asStr()) 
    {
    terminal.println("==List of available commands:==");
    terminal.println("wifi");
    terminal.println("particles");
    terminal.println("blink");
    terminal.println("rapidon");
    terminal.println("rapidoff");
    terminal.println("temp");
    terminal.println("weather");
    terminal.println("winddir");
     terminal.println("==End of list.==");
    }
        if (String("wifi") == param.asStr()) 
    {
        terminal.print("Connected to: ");
        terminal.println(ssid);
        terminal.print("IP address:");
        terminal.println(WiFi.localIP());
        terminal.print("Signal strength: ");
        terminal.println(WiFi.RSSI());
        printLocalTime();
    }
    if (String("particles") == param.asStr()) {
      readPMSverbose();
    }

    if (String("blink") == param.asStr()) {
      terminal.println("Blinking...");
  led.crossFade(RGBLed::BLUE, RGBLed::GREEN, 20, 2000);
    }
    if (String("rapidon") == param.asStr()) {
      rapidfire = true;
    }
        if (String("rapidoff") == param.asStr()) {
      rapidfire = false;
    }
    if (String("temp") == param.asStr()) {
    sensors.requestTemperatures(); 
    sensors.requestTemperatures(); 
        temperatureC = sensors.getTempCByIndex(0);
        terminal.print("Temp: ");
        terminal.println(temperatureC);
    }

if (String("weather") == param.asStr()) {
          String serverPath = "http://api.openweathermap.org/data/2.5/weather?q=" + city + "," + countryCode + "&APPID=" + openWeatherMapApiKey;
      
      jsonBuffer = httpGETRequest(serverPath.c_str());
      terminal.println(jsonBuffer);
      JSONVar myObject = JSON.parse(jsonBuffer);
  
      // JSON.typeof(jsonVar) can be used to get the type of the var
      if (JSON.typeof(myObject) == "undefined") {
        terminal.println("Parsing input failed!");
        return;
      }
    
      terminal.print("JSON object = ");
      terminal.println(myObject);
      terminal.print("Temperature: ");
      terminal.println(double((myObject["main"]["temp"])) - 273.15);
      terminal.print("Pressure: ");
      terminal.println(myObject["main"]["pressure"]);
      terminal.print("Humidity: ");
      terminal.println(myObject["main"]["humidity"]);
      terminal.print("Wind Speed kph: ");
      terminal.println(3.6 * double((myObject["wind"]["speed"])));
            terminal.print("Wind gust kph: ");
      terminal.println(3.6 * double((myObject["wind"]["gust"])));
            terminal.print("Wind direction deg: ");
      terminal.println(double((myObject["wind"]["deg"])));
    terminal.flush();
  }
if (String("winddir") == param.asStr()) {
            String serverPath = "http://api.openweathermap.org/data/2.5/weather?q=" + city + "," + countryCode + "&APPID=" + openWeatherMapApiKey;
      
      jsonBuffer = httpGETRequest(serverPath.c_str());
      terminal.println(jsonBuffer);
      JSONVar myObject = JSON.parse(jsonBuffer);
            terminal.print("Wind direction deg: ");
      terminal.println(double((myObject["wind"]["deg"])));
      terminal.print("Wind direction: ");
      int tempwinddir = (double((myObject["wind"]["deg"])));
      terminal.println(windDirection(tempwinddir));
     terminal.flush();
}

    terminal.flush();

}

BLYNK_CONNECTED() {
  bridge1.setAuthToken (remoteAuth);
  bridge2.setAuthToken (remoteAuth2);
}

void doWeather(){
           String serverPath = "http://api.openweathermap.org/data/2.5/weather?q=" + city + "," + countryCode + "&APPID=" + openWeatherMapApiKey;
      
      jsonBuffer = httpGETRequest(serverPath.c_str());
      JSONVar myObject = JSON.parse(jsonBuffer);
      inetTemp = (double((myObject["main"]["temp"])) - 273.15);
      inetWindspeed = (3.6 * double((myObject["wind"]["speed"])));
      inetWinddeg = (double((myObject["wind"]["deg"])));
      inetWindgust = (3.6 * double((myObject["wind"]["gust"])));
}

String windDirection(int temp_wind_deg)   //Source http://snowfence.umn.edu/Components/winddirectionanddegreeswithouttable3.htm
{
  switch(temp_wind_deg){
    case 0 ... 11:
      return "N";
      break;
    case 12 ... 33:
      return "NNE";
      break;
    case 34 ... 56:
      return "NE";
      break;
    case 57 ... 78:
      return "ENE";
      break;
    case 79 ... 101:
      return "E";
      break;
    case 102 ... 123:
      return "ESE";
      break;
    case 124 ... 146:
      return "SE";
      break;
    case 147 ... 168:
      return "SSE";
      break;
    case 169 ... 191:
      return "S";
      break;
    case 192 ... 213:
      return "SSW";
      break;
    case 214 ... 236:
      return "SW";
      break;
    case 237 ... 258:
      return "WSW";
      break;
    case 259 ... 281:
      return "W";
      break;
    case 282 ... 303:
      return "WNW";
      break;
    case 304 ... 326:
      return "NW";
      break;
    case 327 ... 348:
      return "NNW";
      break;
    case 349 ... 360:
      return "N";
      break;
    default:
      return "error";
      break;
  }
}
