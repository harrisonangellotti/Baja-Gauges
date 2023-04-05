// Get current sensor readings when the page loads  
window.addEventListener('load', getReadings);

// Create Temperature Gauge
var gaugeTemp = new RadialGauge({
  renderTo: 'gauge-temperature',
  width: 300,
  height: 300,
  units: "x1000 RPM",
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
  
// Create Potentiometer Gauge
var gaugePot = new RadialGauge({
  renderTo: 'potentiometer',
  width: 300,
  height: 300,
  units: "km/h speed",
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

// Function to get current readings on the webpage when it loads for the first time
function getReadings(){
  var xhr = new XMLHttpRequest();
  xhr.onreadystatechange = function() {
    if (this.readyState == 4 && this.status == 200) {
      var myObj = JSON.parse(this.responseText);
      console.log(myObj);
      var pot = myObj.potentiometer;
      gaugePot.value = pot;
    }
  }; 
  xhr.open("GET", "/readings", true);
  xhr.send();
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
    gaugePot.value = myObj.potentiometer;
  }, false);
}

var timerId;
var time = 0;

function startTimer() {
  timerId = setInterval(updateTimer, 1000);
}

function stopTimer() {
  clearInterval(timerId);
}

function updateTimer() {
  time++;
  var hours = Math.floor(time / 3600);
  var minutes = Math.floor((time % 3600) / 60);
  var seconds = time % 60;
  document.getElementById("timer").innerHTML = pad(hours) + ":" + pad(minutes) + ":" + pad(seconds);
}

function pad(num) {
  return num < 10 ? "0" + num : num;
}
