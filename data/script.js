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

// variables and functionality for timer to react to button presses
var timerId;
var initialTime = 300; // 300 seconds in 5 minutes; this will need adjusting for empty tank time
var time = initialTime;
resetTimer();

function startTimer() {
  timerId = setInterval(updateTimer, 1000); // update the timer value every 1000ms
}

function stopTimer() {
  clearInterval(timerId);
}

function resetTimer() {
  time = initialTime;
  // make dynamic so timer text is initialized to desired value
  var minutes = Math.floor((initialTime % 3600) / 60);
  var seconds = initialTime % 60;
  document.getElementById("timer-text").innerHTML = pad(minutes) + ":" + pad(seconds);
}

function updateTimer() {
  time--; // subtract one second
  // reprint to 'timer-text' ID on html
  var minutes = Math.floor((time % 3600) / 60);
  var seconds = time % 60;
  document.getElementById("timer-text").innerHTML = pad(minutes) + ":" + pad(seconds);
}

function shiftGear(gearState){
  var colour = document.querySelector(':root');
  var colourStyle = getComputedStyle(colour);

  if (gearState == 1){
    colourStyle.style.setProperty('--colourPark', 'white');
    colourStyle.style.setProperty('--colourReverse', 'darkgray');
    colourStyle.style.setProperty('--colourNeutral', 'darkgray');
    colourStyle.style.setProperty('--colourDrive', 'darkgray');
  } else if (gearState == 2){
    colourStyle.style.setProperty('--colourPark', 'darkgray');
    colourStyle.style.setProperty('--colourReverse', 'white');
    colourStyle.style.setProperty('--colourNeutral', 'darkgray');
    colourStyle.style.setProperty('--colourDrive', 'darkgray');
  } else if (gearState == 3){
    colourStyle.style.setProperty('--colourPark', 'darkgray');
    colourStyle.style.setProperty('--colourReverse', 'darkgray');
    colourStyle.style.setProperty('--colourNeutral', 'white');
    colourStyle.style.setProperty('--colourDrive', 'darkgray');
  } else if (gearState == 4){
    colourStyle.style.setProperty('--colourPark', 'darkgray');
    colourStyle.style.setProperty('--colourReverse', 'darkgray');
    colourStyle.style.setProperty('--colourNeutral', 'darkgray');
    colourStyle.style.setProperty('--colourDrive', 'white');
  }
  
}

function pad(num) {
  return num < 10 ? "0" + num : num;
}

// Function to get current sensor readings on the webpage when it loads for the first time
function getReadings(){
  var xhr = new XMLHttpRequest();
  xhr.onreadystatechange = function() {   // onreadystatechange only called when page loads
    if (this.readyState == 4 && this.status == 200) {
      var myObj = JSON.parse(this.responseText);
      console.log(myObj);
      var speed = myObj.gaugeSpeed; // collects the initial data reading
      speedGauge.value = speed;
      resetTimer();
    }
  }; 
  xhr.open("GET", "/readings", true); // call the readings page in C++ code to send data back (SSE)
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
    speedGauge.value = myObj.gaugeSpeed;
  }, false);

  // start_timer event will start counting down the timer
  source.addEventListener('start_timer', function(e) {
    console.log("start_timer", e.data);
    startTimer();
  }, false);

  // stop_timer event will stop the timer counting
  source.addEventListener('stop_timer', function(e) {
    console.log("stop_timer", e.data);
    stopTimer();  // stop counting interval
  }, false);

  // reset_timer event will stop the timer interval and reset value to initialTime
  source.addEventListener('reset_timer', function(e) {
    console.log("reset_timer", e.data);
    stopTimer();
    resetTimer();
  }, false);
  
  source.addEventListener('gear_shift', function(e) {
    console.log("gear_shift", e.data);
    var gearObj = JSON.parse(e.data);
    console.log(gearObj);
    speedGauge.value = myObj.gaugeSpeed;
    shiftGear(gearObj.gearState);
  }, false);


}