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

int timerValue = 300;  // start timer at 5 mins (300 seconds)
bool timerStatus = false; // holds status of timer (on/off)
int lastButtonState = HIGH;

int ledState = LOW;
unsigned long timerDelay = 1000; // send time every second

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

/*
COMMENTING THIS OUT FOR GITHUB POST

  // Route for POST request to start timer
  server.on("/start-timer", HTTP_POST, [](AsyncWebServerRequest *request){
    // If the timer is already running, do nothing
    if (timerStatus) {
      request->send(200, "text/plain", "Timer already started");
      return;
    }
    // Set the timer status to "on"
    timerStatus = true;
    // Reset the timer value to 5 minutes
    timerValue = 300;
    // Turn on the LED
    digitalWrite(LED_PIN, HIGH);
    request->send(200, "text/plain", "Timer started");
  });
  // Route for POST request to stop timer
  server.on("/stop-timer", HTTP_POST, [](AsyncWebServerRequest *request){
    // If the timer is already stopped, do nothing
    if (!timerStatus) {
      request->send(200, "text/plain", "Timer already stopped");
      return;
    }
    // Set the timer status to "off"
    timerStatus = false;
    // Set the timer value to 0
    timerValue = 0;
    // Turn off the LED
    digitalWrite(LED_PIN, LOW);
    request->send(200, "text/plain", "Timer stopped");
  });
  // Route for getting the timer value in JSON format
  server.on("/timer-value", HTTP_GET, [](AsyncWebServerRequest *request){
    String json = "{\"timerValue\": " + String(timerValue) + "}";
    request->send(200, "application/json", json);
  });

  */

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

/* 
COMMENTING THIS OUT FOR GITHUB POST 

  // Read the current button state
  int currentButtonState = digitalRead(BUTTON_PIN);
  // If the button state has changed from HIGH to LOW, start or stop the timer
  if (currentButtonState == LOW && lastButtonState == HIGH) {
    if (timerStatus) {
      // Stop the timer
      timerStatus = false;
      timerValue = 0;
      digitalWrite(LED_PIN, LOW);
    } else {
      // Start the timer
      timerStatus = true;
      timerValue = 300;
      digitalWrite(LED_PIN, HIGH);
    }
  } 
  // Change the last button state to the current state
  lastButtonState = currentButtonState;

  // If the timer is on, decrement the timer value every second
  if (timerStatus) {
    if (millis() - lastTime > timerDelay) {
      lastTime = millis();
      timerValue--;
      // If the timer has reached 0, turn off the LED and set the timer status to off
      if (timerValue <= 0) {
        digitalWrite(LED_PIN, LOW);
        timerStatus = false;
      }
    }
  }

*/

  if ((millis() - lastTime) > potDelay) {
    // Send Events to the client with the Sensor Readings every 5 seconds
    events.send("ping",NULL,millis());
    events.send(getSensorReadings().c_str(),"new_readings" ,millis());  // this sends an event "new_readings"
    Serial.println("Reading Sent");
    lastTime = millis();
  }
}
