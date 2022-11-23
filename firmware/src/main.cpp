#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <detail/mimetable.h>
#include <LittleFS.h>
#include "WEMOS_Motor.h"
#include "ArduinoJson.h"
#include "blinker.h"
#include "devicestate.h"

using namespace mime;

// configuration
const char AP_SSID[] PROGMEM = "RC Device 1";
const char AP_PSK[] PROGMEM = "rcdevice1";   // at least 8 characters
const char DNS_NAME[] PROGMEM = "rcdevice1"; // only a-z, 0-9 and - allowed
const auto NETWORK = 84;                     // third octet of IP address
const auto COMM_TIMOUT_MS = 1000;            // command timeout [ms]
const auto BATTERY_RESISTOR = 470.0;         // [kOhm]
const auto BATTERY_LIMIT = 4.0;              // [V]

// constants
const auto HTTP_PORT = 80;
const auto DNS_PORT = 53;
const auto SERIAL_SPEED = 115200;
const auto BATTERY_FACTOR = (BATTERY_RESISTOR + 320.0) / 102400.0;

// global vars
DNSServer dnsServer;
ESP8266WebServer webServer(HTTP_PORT);
Ticker publishTicker;
Ticker batteryTicker;
Blinker ledBlinker;
Motor steeringMotor(0x30, _MOTOR_A, 1000);
Motor primeMotor(0x30, _MOTOR_B, 1000);

DeviceState deviceState;
float battery = -1.0; // -1: unknown
unsigned long lastCommTime;
float throttle; // -1: backward, 0: stop, 1: forward
float steering; // -1: left, 0: straight, 1: right

void handlePublishState()
{
  // publish state to serial line
  Serial.print(F("State:"));
  Serial.print(deviceStateText(deviceState));
  Serial.print(F(" Battery:"));
  Serial.print(battery);
  Serial.print(F(" Throttle:"));
  Serial.print(throttle);
  Serial.print(F(" Steering:"));
  Serial.print(steering);
  Serial.println();

  // signal state over blink codes
  if (deviceState != READY)
  {
    ledBlinker.blink(deviceState);
  }
}

void handleCommand()
{
  // body present?
  if (!webServer.hasArg(F("plain")))
  {
    webServer.send(400, mimeTable[txt].mimeType, F("No HTTP body"));
    return;
  }
  // deserialize json
  StaticJsonDocument<128> jsonDoc;
  auto err = deserializeJson(jsonDoc, webServer.arg(F("plain")));
  if (err != DeserializationError::Ok)
  {
    webServer.send(400, mimeTable[txt].mimeType, F("Invalid JSON body"));
    return;
  }
  // check values
  float throttleIn = jsonDoc[F("Throttle")] | -99.0 /* unknown */;
  float steeringIn = jsonDoc[F("Steering")] | -99.0 /* unknown */;
  if (throttleIn < -1.0 || throttleIn > 1.0 || steeringIn < -1.0 || steeringIn > 1.0)
  {
    webServer.send(400, mimeTable[txt].mimeType, F("Value(s) out of range"));
    return;
  }
  // take values
  throttle = throttleIn;
  steering = steeringIn;
  lastCommTime = millis();
  webServer.send(200);
}

void handleTelemetry()
{
  // send telemetry data
  StaticJsonDocument<128> jsonDoc;
  jsonDoc[F("Battery")] = battery;
  jsonDoc[F("State")] = deviceStateText(deviceState);
  if (jsonDoc.overflowed())
  {
    webServer.send(500);
    return;
  }
  String jsonTxt;
  serializeJson(jsonDoc, jsonTxt);
  webServer.send(200, mimeTable[json].mimeType, jsonTxt);
}

void setup()
{
  // setup pins
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH); // led off

  // setup serial
  Serial.begin(SERIAL_SPEED);

  // start publish timer
  publishTicker.attach_ms_scheduled(3000, handlePublishState);

  // init file system
  if (!LittleFS.begin())
  {
    deviceState = FILESYSTEM_FAILED;
    return;
  }

  // setup wifi access point
  IPAddress apIP(192, 168, NETWORK, 1);
  IPAddress apMask(255, 255, 255, 0);
  if (!WiFi.mode(WIFI_AP))
  {
    deviceState = WIFI_FAILED;
    return;
  }
  if (!WiFi.softAPConfig(apIP, apIP, apMask))
  {
    deviceState = WIFI_FAILED;
    return;
  }
  if (!WiFi.softAP(FPSTR(AP_SSID), FPSTR(AP_PSK)))
  {
    deviceState = WIFI_FAILED;
    return;
  }

  // setup dns server
  if (!dnsServer.start(DNS_PORT, FPSTR(DNS_NAME), apIP))
  {
    deviceState = DNS_FAILED;
    return;
  }

  // setup web server
  webServer.on(F("/telemetry"), HTTP_GET, handleTelemetry);
  webServer.on(F("/command"), HTTP_PUT, handleCommand);
  webServer.serveStatic("/", LittleFS, "/", "no-cache");
  webServer.begin();

  // start battery timer
  batteryTicker.attach_ms_scheduled(
      1000, []()
      { battery = analogRead(A0) * BATTERY_FACTOR; });

  deviceState = READY;
  lastCommTime = millis();
}

void updateState()
{
  // handle low battery
  if (deviceState == READY && battery >= 0.0 && battery < BATTERY_LIMIT)
  {
    deviceState = LOW_BATTERY;
  }

  // handle command timeout
  auto lastComm = millis() - lastCommTime;
  if (deviceState == READY && lastComm >= COMM_TIMOUT_MS)
  {
    deviceState = COMMAND_TIMEOUT;
  }
  if (deviceState == COMMAND_TIMEOUT && lastComm < COMM_TIMOUT_MS)
  {
    deviceState = READY;
  }
}

void updateMotors()
{
  // drive motors
  if (deviceState == READY)
  {
    // prime motor
    if (throttle > 0.0)
    {
      primeMotor.setmotor(_CCW, 50);
    }
    else if (throttle < 0.0)
    {
      primeMotor.setmotor(_CW, 50);
    }
    else
    {
      primeMotor.setmotor(_STOP);
    }

    // steering motor
    if (steering > 0.0)
    {
      steeringMotor.setmotor(_CW);
    }
    else if (steering < 0.0)
    {
      steeringMotor.setmotor(_CCW);
    }
    else
    {
      steeringMotor.setmotor(_STOP);
    }

    // turn led on/off if motors active
    if (throttle != 0.0 || steering != 0.0)
    {
      digitalWrite(LED_BUILTIN, LOW); // led on
    }
    else
    {
      digitalWrite(LED_BUILTIN, HIGH); // led off
    }
  }
  else
  {
    // stop motors
    primeMotor.setmotor(_SHORT_BRAKE);
    steeringMotor.setmotor(_SHORT_BRAKE);

    // do not restart, if state changes to READY again
    throttle = 0.0;
    steering = 0.0;
  }
}

void loop()
{
  updateState();
  updateMotors();

  // handle DNS and web server
  dnsServer.processNextRequest();
  webServer.handleClient();
}
