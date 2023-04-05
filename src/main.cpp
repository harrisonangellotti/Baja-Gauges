/*
March 30, 2023

This script puts ESP32 into softAP mode to initialize its own Async server
And uses SPIFFS filesysten onboard ESP32 to store HTML, CSS, and JS files

* ^ files must be stored in 'data' folder on same root as 'src/main.cpp' *
I'm using platformio to upload to ESP32, so 'platformio.ini' same root too

Currently reads potentiometer value every 0.5 seconds and sends an event
request to web page, which updates the variable on website

using canvas-gauges library, the reading is displayed on a GUI
*/

// Include all libraries required for current project
#include <Arduino.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include "SPIFFS.h"
#include <Arduino_JSON.h>

// Network credentials
const char* ssid = "TestNet";
const char* password = "123456789";

// Initialize webserver on port 80
AsyncWebServer server(80);

// Create an Event Source on /events
// "events" object sends request to HTML to update data 
AsyncEventSource events("/events");

/*
GPIO Pins and Variables

Note GPIO pins 32-39 must be used for conversion
While EPS32 is in softAP mode
not sure why but read it somewhere
*/ 

const int LED_PIN = 21;
const int BUTTON_PIN = 22;
const int POT_PIN = 32; 

JSONVar data;

// Constants for LED and button pins
const int LED_PIN = 21;
const int BUTTON_PIN = 22;

// Variables to store LED state and button state
bool ledState = false;
bool buttonState = false;
bool lastButtonState = false;

// Timer to refresh pot reading every 500mS
unsigned long lastTime = 0;
unsigned long potDelay = 500; // send pot every 0.5 second

// Function to return reading from potentiometer as JSON string
String getSensorReadings(){
  unsigned int potValue = analogRead(POT_PIN);
  unsigned int mappedValue = map(potValue, 0, 4095, 0, 100);
  Serial.printf("Potentiometer reading: %d    |    Mapped reading: %d\n", potValue, mappedValue);
  data["potentiometer"] = String(mappedValue);
  String jsonString = JSON.stringify(data);
  return jsonString;
}

// Initialize SPIFFS
void initSPIFFS() {
  if (!SPIFFS.begin()) {
    Serial.println("An error has occurred while mounting SPIFFS");
  }
  Serial.println("SPIFFS mounted successfully");
}

void setup() {
  // Setup monitor and pin modes
  Serial.begin(115200);
  pinMode(LED_PIN, OUTPUT);
  pinMode(POT_PIN, INPUT);
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  
  // Set initial LED state
  digitalWrite(LED_PIN, ledState);

  // initialize ESP for softAP and print IP for browser
  WiFi.softAP(ssid, password);
  IPAddress IP = WiFi.softAPIP();
  Serial.println(IP);

  initSPIFFS(); // initialze file system on ESP32

  // server.on will route handle requests to the specified URI
  // pretty much just tells ESP what data to send when certain ("URL", method,[]..) are sent

  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/index.html", "text/html");
  });
  
  // send static webpage from SPIFFS
  server.serveStatic("/", SPIFFS, "/");
 
  // Route to get latest sensor reading at "IP/readings"
  server.on("/readings", HTTP_GET, [](AsyncWebServerRequest *request){
    String json = getSensorReadings();
    request->send(200, "application/json", json);
    json = String();
  });

  // only runs when you connect
  events.onConnect([](AsyncEventSourceClient *client){
    if(client->lastId()){
      Serial.printf("Client reconnected! Last message ID that it got is: %u\n", client->lastId());
    }
    // send event with message "hello!", id current millis
    // and set reconnect delay to 1 second
    client->send("hello!", NULL, millis(), 10000);
  });
  server.addHandler(&events);

  // initialize server
  server.begin();
}

void loop() {
  // Read button state
  buttonState = digitalRead(BUTTON_PIN);

  // Check if button state has changed
  if (buttonState != lastButtonState) {
    delay(50); // Debounce delay
    if (buttonState == LOW) {
      // Button is pressed, toggle LED state
      ledState = !ledState;
      digitalWrite(LED_PIN, ledState);
      Serial.println("Button pressed!");
    }
  }

  // Update last button state
  lastButtonState = buttonState;
  if ((millis() - lastTime) > potDelay) {
    // Send Events to the client with the Sensor Readings every 5 seconds
    events.send("ping",NULL,millis());
    events.send(getSensorReadings().c_str(),"new_readings" ,millis());  // this sends an event "new_readings"
    Serial.println("Reading Sent");
    lastTime = millis();
  }
}
