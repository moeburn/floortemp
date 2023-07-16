
#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <AsyncElegantOTA.h>
#include <BlynkSimpleEsp8266.h>
#include <OneWire.h>
#include <DallasTemperature.h>

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

void setup(void) {
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.println("");

  // Wait for connection
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
    // Start the DS18B20 sensor
  sensors.begin();
      Blynk.config(auth, IPAddress(192, 168, 50, 197), 8080);
    Blynk.connect();
  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(200, "text/plain", "Monkeys fly out of my butt");
  });

  AsyncElegantOTA.begin(&server);    // Start ElegantOTA
  server.begin();
  Serial.println("HTTP server started");
}

void loop(void) {
Blynk.run();
    if  (millis() - millisBlynk >= 30000)  //if it's been 30 seconds OR we just booted up, skip the 30 second wait
    {
        millisBlynk = millis();
        sensors.requestTemperatures(); 
        float temperatureC = sensors.getTempCByIndex(0);
        Serial.println(temperatureC);
        Blynk.virtualWrite(V1, temperatureC);
    }
}
