// Load Wi-Fi library
#include <ESP8266WiFi.h>
#include <Adafruit_NeoPixel.h>
#include <RGBConverter.h>

#define LED_BUILTIN 16

// Network credentials
const char *ssid = "WIFI SSID";
const char *password = "WIFI PASSWORD";

// Set web server port number to 80
WiFiServer controllerServer(80);
WiFiServer lightServer(81);

String lightHeader;
String controllerHeader;

//not sure if I really need to dupe this stuff for each server but this seems to work

// Current time
unsigned long cCurrentTime = millis();
unsigned long lCurrentTime = millis();
// Previous time
unsigned long cPreviousTime = 0;
unsigned long lPreviousTime = 0;
// Define timeout time in milliseconds (example: 2000ms = 2s)
const long cTimeoutTime = 2000;
const long lTimeoutTime = 2000;

#define PIN 5 //GPIO pin D1
#define NUM_LEDS 22 //1 meter strip @ 30 leds/meter

Adafruit_NeoPixel LEDStrip(NUM_LEDS, PIN, NEO_RGBW + NEO_KHZ800);
RGBConverter RGBConvert;
int rgbw[4];

void setup()
{
  Serial.begin(9600);
  LEDStrip.begin();
  LEDStrip.fill(LEDStrip.Color(0, 0, 0, 255)); //white by default, would like to have it store the last selected color in ROM at some point
  LEDStrip.show();
  // Initialize the output variables as outputs
  pinMode(LED_BUILTIN, OUTPUT);
  // Flash onboard LED on startup, just as a visual check
  digitalWrite(LED_BUILTIN, LOW);
  delay(1500);
  digitalWrite(LED_BUILTIN, HIGH);
  delay(1500);
  digitalWrite(LED_BUILTIN, LOW);
  delay(1500);
  digitalWrite(LED_BUILTIN, HIGH);

  // Connect to Wi-Fi network with SSID and password
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.hostname("lightcontroller"); //might enable using http://lightcontroller.local to access this in case the port changes. if nothing else, it looks cleaner than ESP-XYZ123 gibberish
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(100);
    Serial.print(".");
  }
  // Print local IP address and start web server
  Serial.println("");
  Serial.println("WiFi connected.");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  controllerServer.begin();
  lightServer.begin();
}

void parseRequest(String req)
{
  //first line of the header is the reponse, that's the part we want
  String line = req.substring(0, req.indexOf("\n"));
  //URL queried is in the form POST /url HTTP/1.1, we want the /URL part
  String request = line.substring(line.indexOf("/") + 1, line.indexOf("HTTP"));
  if (request.startsWith("setStripColor"))
  {
    byte rgb[3];
    request = request.substring(request.indexOf("?") + 1);
    double hue = request.substring(request.indexOf("h=") + 2, request.indexOf("s=") - 1).toDouble();
    double saturation = request.substring(request.indexOf("s=") + 2, request.indexOf("l=") - 1).toDouble();
    double value = request.substring(request.indexOf("l=") + 2).toDouble();

    RGBConvert.hsvToRgb(hue/360, 1, 1, rgb); //Hue to RGB conversion, with full saturation and brightness

    if(saturation == 0){ //full white
      LEDStrip.fill(LEDStrip.gamma32(LEDStrip.Color(0, 0, 0, 255)));
      LEDStrip.show();
    }
    else {
      int white = ((100 - saturation) / 100) * 255; //convert 0-100% saturation to 0-255 white
      if(saturation == 100){white = 0;} //full color
      LEDStrip.fill(LEDStrip.gamma32(LEDStrip.Color(uint8_t(rgb[1]), uint8_t(rgb[0]), uint8_t(rgb[2]), uint8_t(white)))); //for some ungodly reason you have to flip the R and G
      LEDStrip.show();
    }
  }
  else if(request.startsWith("allOff"))
  {
    LEDStrip.clear();
    LEDStrip.show();
  }
  else if(request.startsWith("setLightness"))
  {
    request = request.substring(request.indexOf("?") + 1);
    double value = request.substring(request.indexOf("l=") + 2).toDouble();
    int brightness = (value / 100) * 255; //convert 100-0% brightness to 0-255
    if(brightness == 0) brightness = 1; //if you set to 0 it turns the strip off until you change the color - setBrightness is lossy
    LEDStrip.setBrightness(uint8_t(brightness));
    LEDStrip.show();
  }
}

void loop()
{
  WiFiClient controllerSite = controllerServer.available(); // Listen for incoming clients on port 80
  //this is the control site with GUI
  if (controllerSite)
  {                                    // If a new client connects,
    String currentLine = "";           // make a String to hold incoming data from the client
    cCurrentTime = millis();
    cPreviousTime = cCurrentTime;
    while (controllerSite.connected() && cCurrentTime - cPreviousTime <= cTimeoutTime)
    { // loop while the client's connected
      cCurrentTime = millis();
      if (controllerSite.available())
      {                                 // if there's bytes to read from the client,
        char c = controllerSite.read(); // read a byte, then
        controllerHeader += c;          // add it to the header
        if (c == '\n')
        { // if the byte is a newline character
          // if the current line is blank, you got two newline characters in a row.
          // that's the end of the client HTTP request, so send a response:
          if (currentLine.length() == 0)
          {
            // HTTP headers always start with a response code (e.g. HTTP/1.1 200 OK)
            // and a content-type so the client knows what's coming, then a blank line:
            controllerSite.println("HTTP/1.1 200 OK");
            controllerSite.println("Content-type:text/html");
            controllerSite.println("Connection: close");
            controllerSite.println();

            // Display the HTML web page
            controllerSite.println("<!doctype html> <html lang='en'> <head>");
            controllerSite.println("<meta charset='utf-8'> <title>Lamp</title>");
            controllerSite.println("<meta name='description' content='Controls the overhead light in Camps room'>");
            controllerSite.println("<meta name='HandheldFriendly' content='True'>");
            controllerSite.println("<meta name='MobileOptimized' content='320'>");
            controllerSite.println("<meta name='viewport' content='width=device-width, initial-scale=1, user-scalable=no'>");
            controllerSite.println("<meta http-equiv='cleartype' content='on'>");
            controllerSite.println("<link rel='shortcut icon' href='https://i.imgur.com/7oqpPEz.png'>");
            controllerSite.println("<meta name='apple-mobile-web-app-capable' content='yes'>");
            controllerSite.println("<meta name='apple-mobile-web-app-status-bar-style' content='black'>");
            controllerSite.println("<meta name='apple-mobile-web-app-title' content='Light Controller'>");
            controllerSite.println("<style>");
            controllerSite.println("html, body {");
            controllerSite.println("width: 100%;");
            controllerSite.println("height: 100vh;");
            controllerSite.println("-webkit-overflow-scrolling: touch;");
            controllerSite.println("position: static;");
            controllerSite.println("margin: 0;");
            controllerSite.println("padding: 0;");
            controllerSite.println("overflow: hidden; }");
            controllerSite.println("body {");
            controllerSite.println("background-color: rgb(41, 41, 41);");
            controllerSite.println("overflow: hidden;");
            controllerSite.println("position: absolute;");
            controllerSite.println("display: flex;");
            controllerSite.println("flex-direction: column;");
            controllerSite.println("align-items: center;");
            controllerSite.println("justify-content: space-evenly; }");
            controllerSite.println(".main {");
            controllerSite.println("position: relative;");
            controllerSite.println("display: flex;");
            controllerSite.println("align-items: center;");
            controllerSite.println("justify-content: center; }");
            controllerSite.println("#power.off #powerbutton {");
            controllerSite.println("stroke: black !important; }");
            controllerSite.println("#color {");
            controllerSite.println("height: 384px;");
            controllerSite.println("width: 384px; }");
            controllerSite.println(".dragbutton {");
            controllerSite.println("pointer-events: none; }");
            controllerSite.println(".slider {");
            controllerSite.println("-webkit-appearance: slider-vertical;");
            controllerSite.println("writing-mode: bt-lr;");
            controllerSite.println("width: 8%;");
            controllerSite.println("height: 220px;");
            controllerSite.println("margin: 0;");
            controllerSite.println("padding: 0;");
            controllerSite.println("--bg-color: #ffffff; }");
            controllerSite.println("#saturation::before {");
            controllerSite.println("content: ' ';");
            controllerSite.println("height: 16px;");
            controllerSite.println("width: 16px;");
            controllerSite.println("background-image: url(https://i.imgur.com/ibpTHxV.png);");
            controllerSite.println("background-size: 16px;");
            controllerSite.println("position: absolute;");
            controllerSite.println("top: 15.5%;");
            controllerSite.println("right: 82.6%; }");
            controllerSite.println("#saturation::after {");
            controllerSite.println("content: ' ';");
            controllerSite.println("height: 32px;");
            controllerSite.println("width: 32px;");
            controllerSite.println("background-image: url('https://i.imgur.com/j3Vh5eK.png');");
            controllerSite.println("background-size: 32px;");
            controllerSite.println("position: absolute;");
            controllerSite.println("top: 77.7%;");
            controllerSite.println("right: 81.5%;");
            controllerSite.println("filter: brightness(0) saturate(100%)invert(99%) sepia(0%) saturate(7500%) hue-rotate(110deg) brightness(114%) contrast(101%); }");
            controllerSite.println("#lightness::before {");
            controllerSite.println("content: ' ';");
            controllerSite.println("height: 32px;");
            controllerSite.println("width: 32px;");
            controllerSite.println("background-image: url('https://i.imgur.com/PFOU0MM.png');");
            controllerSite.println("background-size: 32px;");
            controllerSite.println("position: absolute;");
            controllerSite.println("top: 11.5%;");
            controllerSite.println("right: 13.6%; }");
            controllerSite.println("#lightness::after {");
            controllerSite.println("content: ' ';");
            controllerSite.println("height: 32px;");
            controllerSite.println("width: 32px;");
            controllerSite.println("background-image: url('https://i.imgur.com/j3Vh5eK.png');");
            controllerSite.println("background-size: 32px;");
            controllerSite.println("position: absolute;");
            controllerSite.println("top: 78.5%;");
            controllerSite.println("right: 13.6%; }");
            controllerSite.println(".slider::-webkit-slider-runnable-track {");
            controllerSite.println("border-radius: 50px;");
            controllerSite.println("border: 0.2px solid #ffffff; }");
            controllerSite.println(".slider::-moz-range-track {");
            controllerSite.println("border-radius: 50px;");
            controllerSite.println("border: 0.2px solid #ffffff; }");
            controllerSite.println(".slider::-ms-track {");
            controllerSite.println("border-radius: 50px;");
            controllerSite.println("border: 0.2px solid #ffffff; }");
            controllerSite.println("#saturation::-webkit-slider-runnable-track {");
            controllerSite.println("background: linear-gradient(var(--bg-color), white); }");
            controllerSite.println("#saturation::-moz-range-track {");
            controllerSite.println("background: var(--bg-color); }");
            controllerSite.println("#saturation::-ms-track {");
            controllerSite.println("background: var(--bg-color); }");
            controllerSite.println("#lightness {");
            controllerSite.println("--percent: 100%; }");
            controllerSite.println("#lightness::-webkit-slider-runnable-track {");
            controllerSite.println("background: linear-gradient(white calc(100% - var(--percent)), yellow calc(100% - var(--percent) + 2%)); }");
            controllerSite.println("#lightness::-moz-range-track {");
            controllerSite.println("background: var(--bg-color); }");
            controllerSite.println("#lightness::-ms-track {");
            controllerSite.println("background: var(--bg-color); }");
            controllerSite.println("@media screen and (max-width: 1000px) {");
            controllerSite.println("#color { width: calc(384px / 1.3714); }");
            controllerSite.println(".slider { height: calc(220px / 1.3714); }");
            controllerSite.println("#saturation::before {");
            controllerSite.println("right: 89%;");
            controllerSite.println("top: 22%; }");
            controllerSite.println("#saturation::after {");
            controllerSite.println("right: 87.2%;");
            controllerSite.println("top: 70.5%; }");
            controllerSite.println("#lightness::before {");
            controllerSite.println("right: 5%;");
            controllerSite.println("top: 18%; }");
            controllerSite.println("#lightness::after {");
            controllerSite.println("right: 5%;");
            controllerSite.println("top: 72%; } }");
            controllerSite.println("</style> </head>");
            controllerSite.println("<body ontouchmove = 'function(event){event.preventDefault();}'>");
            controllerSite.println("<main class='main'>");
            controllerSite.println("<input orient='vertical' class='slider' id='saturation' type='range' min='0' max='100' value='100'");
            controllerSite.println("onchange='setSaturation(this.value)'>");
            controllerSite.println("<svg id='color' xmlns='http://www.w3.org/2000/svg' viewBox='-64 -64 2176 2176'>");
            controllerSite.println("<defs> <pattern id='rainbow' patternUnits='userSpaceOnUse' width='4096' height='4096'>");
            controllerSite.println("<image xlink:href='https://i.imgur.com/ibpTHxV.png' x='0' y='0' width='2048' height='2048' />");
            controllerSite.println("</pattern> </defs>");
            controllerSite.println("<circle class='touchwheel' id='colorwheel' fill='url(#rainbow)' stroke='none' stroke-width='1' cx='1024'");
            controllerSite.println("cy='1024' r='1024' />");
            controllerSite.println("<g id='power' class='off'>");
            controllerSite.println("<circle id='powerswitch' fill='white' stroke='none' stroke-width='1' cx='1024' cy='1024' r='920' />");
            controllerSite.println("<g id='powerbutton' fill='none' stroke='#000' stroke-linecap='round'");
            controllerSite.println("transform='scale(7.5 7.5) translate(32 2)'>");
            controllerSite.println("<path");
            controllerSite.println("d='m141.34 86.157a64.233 61.71 0 0 1 24.417 69.107 64.233 61.71 0 0 1-61.26 43.154 64.233 61.71 0 0 1-61.26-43.154 64.233 61.71 0 0 1 24.417-69.107'");
            controllerSite.println("stroke-width='15.74' />");
            controllerSite.println("<rect x='104.1' y='64.946' width='.80181' height='47.307' stroke-linejoin='round' stroke-width='16' /> </g> </g>");
            controllerSite.println("<circle id='dragbutton' fill='none' stroke='white' stroke-width='30' cx='1024' cy='1200' r='55' />");
            controllerSite.println("</svg>");
            controllerSite.println("<input orient='vertical' class='slider' id='lightness' type='range' min='0' max='100' value='100'");
            controllerSite.println("onchange='setLightness(this.value)'>");
            controllerSite.println("</main>");
            controllerSite.println("<script type='text/javascript'>");
            controllerSite.println("let color = { h: 0, s: 100, l: 50 };");
            controllerSite.println("let state = false;");
            controllerSite.println("const setNewPos = (event) => {");
            controllerSite.println("event = event || window.event;");
            controllerSite.println("window.e = event;");
            controllerSite.println("const width = (document.getElementById('color').width.animVal.value) / 2;");
            controllerSite.println("let xCoord, yCoord;");
            controllerSite.println("if (event.type.includes('touch')) {");
            controllerSite.println("//this is fucking cursed but it sometimes works ");
            controllerSite.println("event.preventDefault();");
            controllerSite.println("const MAGIC_NUMBER = 2.9;");
            controllerSite.println("xCoord = event.layerX - width / MAGIC_NUMBER - width;");
            controllerSite.println("yCoord = (-1 * event.layerY) + width / MAGIC_NUMBER + width;");
            controllerSite.println("} else { xCoord = event.offsetX - width;");
            controllerSite.println("yCoord = (-1 * event.offsetY) + width; }");
            controllerSite.println("const colorWheelSize = document.getElementById('colorwheel').r.baseVal.value;");
            controllerSite.println("let thetaDeg = Math.atan(yCoord / xCoord) * 180 / Math.PI;");
            controllerSite.println("if (xCoord < 0) { //Q2 + Q3");
            controllerSite.println("thetaDeg += 180; } else {");
            controllerSite.println("if (yCoord < 0) { //Q4");
            controllerSite.println("thetaDeg += 360; } }");
            controllerSite.println("color.h = thetaDeg;");
            controllerSite.println("document.getElementById('powerbutton').style.stroke = `hsl(${color.h}, 100%, 50%)`");
            controllerSite.println("document.getElementById('saturation').style.setProperty('--bg-color', `hsl(${color.h}, 100%, 50%)`);");
            controllerSite.println("document.getElementById('dragbutton').setAttribute('cx', ((975 * Math.cos((thetaDeg / 180) * Math.PI)) +");
            controllerSite.println("colorWheelSize));");
            controllerSite.println("document.getElementById('dragbutton').setAttribute('cy', ((975 * Math.sin((thetaDeg / 180) * Math.PI)) * -");
            controllerSite.println("1 + colorWheelSize));");
            controllerSite.println("document.getElementById('colorwheel').onmousemove = setNewPos;");
            controllerSite.println("document.getElementById('colorwheel').onmouseup = clearDrag;");
            controllerSite.println("document.getElementById('colorwheel').ontouchmove = setNewPos;");
            controllerSite.println("document.getElementById('colorwheel').ontouchend = clearDrag; }");
            controllerSite.println("const clearDrag = () => {");
            controllerSite.println("sendData();");
            controllerSite.println("document.getElementById('dragbutton').onmouseup = document.getElementById('colorwheel').removeEventListener(");
            controllerSite.println("'mousemove', setNewPos);");
            controllerSite.println("document.getElementById('colorwheel').onmousemove = null;");
            controllerSite.println("document.getElementById('colorwheel').onmouseup = null;");
            controllerSite.println("document.getElementById('colorwheel').ontouchmove = null;");
            controllerSite.println("document.getElementById('colorwheel').ontouchend = null; }");
            controllerSite.println("const togglePower = () => {");
            controllerSite.println("document.getElementById('power').classList.toggle('off');");
            controllerSite.println("clearDrag(); state = !state;");
            controllerSite.println("if(state){sendData();} else{turnOff();}}");
            controllerSite.println("document.querySelectorAll('.touchwheel').forEach(item => {");
            controllerSite.println("item.addEventListener('mousedown', setNewPos);");
            controllerSite.println("item.addEventListener('touchstart', setNewPos); })");
            controllerSite.println("Array.from(document.getElementById('power').children).forEach(item => {");
            controllerSite.println("item.addEventListener('click', togglePower); })");
            controllerSite.println("const setSaturation = (value) => {");
            controllerSite.println("color.s = value; sendData(); }");
            controllerSite.println("const setLightness = (value) => {");
            controllerSite.println("color.l = parseInt(value);");
            controllerSite.println("document.getElementById('lightness').style.setProperty('--percent', value + '%');");
            controllerSite.println("let HTTP = new XMLHttpRequest();");
            controllerSite.println("let url =`http://" + WiFi.localIP().toString() + ":81/setLightness?l=${color.l}`");
            controllerSite.println("HTTP.open('POST', url);");
            controllerSite.println("HTTP.send(); }");
            controllerSite.println("const sendData = () => {");
            controllerSite.println("if (state) {");
            controllerSite.println("let HTTP = new XMLHttpRequest();");
            controllerSite.println("let url =`http://" + WiFi.localIP().toString() + ":81/setStripColor?h=${color.h}&s=${color.s}&l=${color.l}`");
            controllerSite.println("HTTP.open('POST', url);");
            controllerSite.println("HTTP.send(); } }");
            controllerSite.println("const turnOff = () => {");
            controllerSite.println("let HTTP = new XMLHttpRequest();");
            controllerSite.println("let url =`http://" + WiFi.localIP().toString() + ":81/allOff`");
            controllerSite.println("HTTP.open('POST', url);");
            controllerSite.println("HTTP.send(); }");
            controllerSite.println("</script> </body> </html>");
            // The HTTP response ends with another blank line
            controllerSite.println();
            // Break out of the while loop
            break;
          }
          else
          { // if you got a newline, then clear currentLine
            currentLine = "";
          }
        }
        else if (c != '\r')
        {                   // if you got anything else but a carriage return character,
          currentLine += c; // add it to the end of the currentLine
        }
      }
    }
    // Clear the header variable
    controllerHeader = "";
    // Close the connection
    controllerSite.stop();
  }

  WiFiClient lightSite = lightServer.available();
  if (lightSite)
  {                                    // If a new client connects,
    String lightCurrentLine = "";      // make a String to hold incoming data from the client
    lCurrentTime = millis();
    lPreviousTime = lCurrentTime;
    while (lightSite.connected() && lCurrentTime - lPreviousTime <= lTimeoutTime)
    { // loop while the client's connected
      lCurrentTime = millis();
      if (lightSite.available())
      {                            // if there's bytes to read from the client,
        char c = lightSite.read(); // read a byte, then
        lightHeader += c;          // add it to the header
        if (c == '\n')
        { // if the byte is a newline character
          // if the current line is blank, you got two newline characters in a row.
          // that's the end of the client HTTP request, so send a response:
          if (lightCurrentLine.length() == 0)
          {
            // HTTP headers always start with a response code (e.g. HTTP/1.1 200 OK)
            // and a content-type so the client knows what's coming, then a blank line:
            lightSite.println("HTTP/1.1 200 OK");
            lightSite.println("Content-type:text/html");
            lightSite.println("Access-Control-Allow-Origin:*");
            lightSite.println("Connection: close");
            lightSite.println();
            // Turn on the lights
            parseRequest(lightHeader);
            // Break out of the while loop
            break;
          }
          else
          { // if you got a newline, then clear currentLine
            lightCurrentLine = "";
          }
        }
        else if (c != '\r')
        {                        // if you got anything else but a carriage return character,
          lightCurrentLine += c; // add it to the end of the currentLine
        }
      }
    }
    // Clear the header variable
    lightHeader = "";
    // Close the connection
    lightSite.stop();
  }
}
