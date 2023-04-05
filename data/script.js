// Get current sensor readings when the page loads  
window.addEventListener('load', getReadings);

// Create Temperature Gauge
var rpmGauge = new RadialGauge({
  renderTo: 'gauge-RPM',
  width: 400,
  height: 400,
  units: "x1000",
  minValue: 0,
  maxValue: 8,
  valueInt: 2,
  majorTicks: [
      "1",
      "2",
      "3",
      "4",
      "5",
      "6",
      "7",
      "8",

  ],
  minorTicks: 4,
  strokeTicks: true,
  highlights: [
      {
          "from": 7,
          "to": 8,
          "color": "#b091cf"
      }
  ],
  colorPlate: "#050505",
  borderShadowWidth: 0,
  borders: false,
  needleType: "line",
  colorNeedle: "#ba78e6",
  colorNeedleEnd: "#ba78e6",
  needleWidth: 2,
  needleCircleSize: 3,
  colorNeedleCircleOuter: "#ba78e6",
  needleCircleOuter: true,
  needleCircleInner: false,
  animationDuration: 1500,
  animationRule: "linear"
}).draw();
  
// Create Speed Gauge
var speedGauge = new RadialGauge({
  renderTo: 'gauge-speed',
  width: 400,
  height: 400,
  units: "km/h",
  minValue: 0,
  maxValue: 100,
  valueInt: 2,
  majorTicks: [
      "0",
      "20",
      "40",
      "60",
      "80",
      "100"

  ],
  minorTicks: 4,
  strokeTicks: true,
  highlights: [
      {
          "from": 80,
          "to": 100,
          "color": "#b091cf"
      }
  ],
  colorPlate: "#050505",
  borderShadowWidth: 0,
  borders: false,
  needleType: "line",
  colorNeedle: "#ba78e6",
  colorNeedleEnd: "#ba78e6",
  needleWidth: 2,
  needleCircleSize: 3,
  colorNeedleCircleOuter: "#ba78e6",
  needleCircleOuter: true,
  needleCircleInner: false,
  animationDuration: 1500,
  animationRule: "linear"
}).draw();

// Function to get current sensor readings on the webpage when it loads for the first time
function getReadings(){
  var xhr = new XMLHttpRequest();
  xhr.onreadystatechange = function() {   // onreadystatechange only called when page loads
    if (this.readyState == 4 && this.status == 200) {
      var myObj = JSON.parse(this.responseText);
      console.log(myObj);
      var speed = myObj.gaugeSpeed; // collects the initial data reading
      speedGauge.value = speed;
    }
  }; 
  xhr.open("GET", "/readings", true); // call the readings page in C++ code to send data back (SSE)
  xhr.send();
}

// variables and functionality for timer to react to button presses
var timerId;
var time = 300; // 300 seconds in 5 minutes; this will need adjusting for empty tank time

function startTimer() {
  timerId = setInterval(updateTimer, 1000); // update the timer value every 1000ms
}

function stopTimer() {
  clearInterval(timerId);
}

function updateTimer() {
  time--; // subtract one second
  // reprint to 'timer-text' ID on html
  var hours = Math.floor(time / 3600);
  var minutes = Math.floor((time % 3600) / 60);
  var seconds = time % 60;
  document.getElementById("timer-text").innerHTML = pad(hours) + ":" + pad(minutes) + ":" + pad(seconds);
}

function pad(num) {
  return num < 10 ? "0" + num : num;
}

// listen for events and execute when pages are visited
if (!!window.EventSource) {
  var source = new EventSource('/events');
  
  source.addEventListener('open', function(e) {
    console.log("Events Connected");
  }, false);

  source.addEventListener('error', function(e) {
    if (e.target.readyState != EventSource.OPEN) {
      console.log("Events Disconnected");
    }
  }, false);
  
  source.addEventListener('message', function(e) {
    console.log("message", e.data);
  }, false);
  
  // whenever a new reading is requested, send value to potentiometer
  source.addEventListener('new_readings', function(e) {
    console.log("new_readings", e.data);
    var myObj = JSON.parse(e.data);
    console.log(myObj);
    speedGauge.value = myObj.gaugeSpeed;
  }, false);

  // start_timer event will start counting down the timer
  source.addEventListener('start_timer', function(e) {
    console.log("start_timer", e.data);
    startTimer();
  }, false);

  // reset_timer event will set timer value back to 300s and stop counting
  source.addEventListener('reset_timer', function(e) {
    console.log("reset_timer", e.data);
    stopTimer();  // stop counting interval
    time = 300; // reset time val
  }, false);
}