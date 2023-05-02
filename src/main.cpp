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
const int STOP_BUTTON_PIN = 22;
const int RESET_BUTTON_PIN = 19;
const int POT_PIN = 32; 

JSONVar data;
JSONVar gearState;

// Variables to store LED state and button states
bool ledState = false;
bool stopButtonState = false;
bool lastStopButtonState = false;
bool resetButtonState = false;
bool lastResetButtonState = false;

// Timer to refresh pot reading every 500mS
unsigned long lastTime = 0;
unsigned long sensorReadingDelay = 250; // send sensor data every .25 second

/*  This is where sensor data will be collected and sent

  FUEL: Flow Rate Sensor / Buttons w Timer
  SPEED: Accelerometer
  RPMs: Tachometer / Encoder 
*/

// Function to return reading from potentiometer as JSON string
String getSensorReadings(){
  unsigned int potValue = analogRead(POT_PIN);
  unsigned int mappedValue = map(potValue, 0, 4095, 0, 100);
  Serial.printf("Potentiometer reading: %d    |    Mapped reading: %d\n", potValue, mappedValue);
  data["gaugeSpeed"] = String(mappedValue);
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
  pinMode(STOP_BUTTON_PIN, INPUT_PULLUP);
  pinMode(RESET_BUTTON_PIN, INPUT_PULLUP);
  
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
  stopButtonState = digitalRead(STOP_BUTTON_PIN);
  resetButtonState = digitalRead(RESET_BUTTON_PIN);

  // Check if stop button state has changed
  if (stopButtonState != lastStopButtonState) {    
    // debounce
    delay(50);  
    if (stopButtonState == LOW) {
      // Button is pressed
      if(ledState){ 
        // if LED was on, stop timer
        events.send("Timer Stopped", "stop_timer", millis());
      }
      else{  
        // if LED was off, start timer
        events.send("Timer Started", "start_timer", millis());
      }
      
      // toggle LED
      ledState = !ledState;   
      digitalWrite(LED_PIN, ledState);
      Serial.println("Start/stop button pressed!");  
    }
  }

  // Check if reset button state has changed
  if (resetButtonState != lastResetButtonState) {    
    // debounce
    delay(50);  
    if (resetButtonState == LOW) {
      // Button is pressed, reset timer
      events.send("Timer reset", "reset_timer", millis());

      // if LED is off, don't change it
      if(!ledState){}
      else{
        ledState = !ledState; 
        digitalWrite(LED_PIN, ledState);
      }
      Serial.println("Reset button pressed!"); 
    }
  }
    
  // Update last button states
  lastStopButtonState = stopButtonState;
  lastResetButtonState = resetButtonState;

  if(Serial.available() > 0) {
    int newState = Serial.read();
      if(newState == 1 || newState == 2 || newState == 3 || newState == 4) {
        gearState["gearState"] = String(newState);
        String jsonString = JSON.stringify(gearState);
        events.send(jsonString.c_str(), "gear_shift", millis()); 
      }
  }

  
  if ((millis() - lastTime) > sensorReadingDelay) {
    // Send Events to the client with the Sensor Readings every 0.25 seconds
    events.send("ping",NULL,millis());
    events.send(getSensorReadings().c_str(),"new_readings" ,millis());  // this sends an event "new_readings" with sensor data
    Serial.println("Reading Sent");
    lastTime = millis();
  }
}
