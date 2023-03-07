// TODO: Allow for multiple btns/leds
#include <stdarg.h>
#include <string.h>
#include "Pins.h"
#include "WifiHelpers.h"

// ================================ Configure
const String btn_id = "demo";
const int wifi = BasementWifi; // BasementWifi UpstairsWifi
// ================================ / Configure

const int btnPin = D4;
const int redPin = D3;
const int greenPin = D2;
const int bluePin = D1;
const int colorPins[3] = { redPin, greenPin, bluePin };
const int off[3] = { 0, 0, 0 };
const int dim_yel[3] = { 100, 60, 0 };
const int dim_red[3] = { 100, 0, 0 };

int currentR = 0;
int currentG = 0;
int currentB = 0;

unsigned long onUntil = 0;
unsigned long flashInterval = 0;
const String data = "{\\\"btn_id\\\":\\\"" + btn_id + "\\\"}";
// ruby -e 'require "json"; p({                                                                                                                                                                                                                     0.120s
//   event_name: "Pullups",
//   notes: "5",
// }.to_json.gsub("\"", "\\\""))'

bool pressed = false;

unsigned long lastDebounceTime = 0;
unsigned long debounceDelay = 2000;

void setup() {
  Serial.begin(115200);
  pinMode(btnPin, INPUT_PULLUP);
  for (int thisColor = 0; thisColor < 3; thisColor++) {
    pinMode(colorPins[thisColor], OUTPUT);
  }
  setRGB(off);

  wifiInit(
    debugMode ? BasementWifi : wifi,
    // Should probably authorize this...
    String("{\"command\":\"subscribe\",\"identifier\":\"{\\\"channel\\\":\\\"TaskChannel\\\",\\\"channel_id\\\":\\\"") + btn_id + String("\\\"}\"}"),
    getState,
    onMessageCallback
  );
}

void setRGB(const int colRGB[3]) {
  for (int thisColor = 0; thisColor < 3; thisColor++) {
    analogWrite(colorPins[thisColor], colRGB[thisColor]);
  }
}

bool btnPressed() {
  return digitalRead(btnPin) == LOW; // Low because pullup and ground connection
}

void loop() {
  wifiLoop(); // Wifi connection + retry

  while (!wifiConnected) {
    if (millis() % 600 > 300) {
      setRGB(off);
    } else {
      setRGB(dim_red);
    }

    delay(100);
    return;
  }
  while (!wsConnected) {
    if (millis() % 600 > 300) {
      setRGB(off);
    } else {
      setRGB(dim_yel);
    }

    delay(100);
    return;
  }

  if (onUntil > 0 && onUntil < millis()) {
    setRGB(off);
    onUntil = 0;
    flashInterval = 0;
  }
  if (flashInterval > 0) {
    if (millis() % (flashInterval*2) < flashInterval) {
      setRGB(off);
    } else {
      int rgb[3] = { currentR, currentG, currentB };
      setRGB(rgb);
    }
  }

  if (!debounce()) { return; }

  if (pressed) {
    if (!btnPressed()) {
      if (debugMode) { Serial.println("Released"); }
      pressed = false;
    }
  } else {
    if (btnPressed()) {
      if (debugMode) { Serial.println("Pressed"); }
      pressed = true;
      lastDebounceTime = millis();
      post();
    }
  }
}

void post() {
  String str = "{\"command\":\"message\",\"identifier\":\"{\\\"channel\\\":\\\"TaskChannel\\\",\\\"channel_id\\\":\\\"" + btn_id + "\\\"}\",\"data\":\"";
  str += data;
  str += "\"}";
  client.send(str);
  if (debugMode) { Serial.println("Sent!"); }
}

void getState() {
  // post("{\\\"action\\\":\\\"receive\\\",\\\"get\\\":\\\"true\\\"}");
  // On send, server should respond with the current state the btn is supposed to display
}
void onMessageCallback(WebsocketsMessage message) {
  setRGB(off); // This resets the LED, but also doubles to turn the LED off after connecting to WS
  // "{\"rgb\":\"0,255,0\",\"for_ms\":1000}"

  const char* rgbstart = strstr(message.c_str(), "\"rgb\":\""); // find the start of the "rgb" key
  if (rgbstart != NULL) {
    rgbstart += 7; // "rgb":" skip the length of the key "rgb" and the delimiter ":"
    const char* rgbend = strchr(rgbstart, '\"'); // find the end of the value string
    if (rgbend != NULL) {
      int r, g, b;
      sscanf(rgbstart, "%d,%d,%d", &r, &g, &b); // parse the string and extract the three integer values
      const int rgb[3] = { r, g, b };
      if (debugMode) {
        Serial.print("setRGB(");
        Serial.print(r);
        Serial.print(",");
        Serial.print(g);
        Serial.print(",");
        Serial.print(b);
        Serial.println(")");
      }
      currentR = r;
      currentG = g;
      currentB = b;
      setRGB(rgb);
    }
  }

  onUntil = 0; // Reset in case the next update doesn't include values
  const char* untilstart = strstr(message.c_str(), "\"for_ms\":\"");
  const char* untilempty = strstr(message.c_str(), "\"for_ms\":\"\"");
  if (untilstart != NULL && untilempty == NULL) {
    if (debugMode) { Serial.println("Found for_ms"); }
    untilstart += 10; // "for_ms":"
    const char* untilend = strchr(untilstart, '\"'); // find the end of the value string
    if (untilend != NULL) {
      int until;
      sscanf(untilstart, "%d", &until);
      if (debugMode) {
        Serial.print("Until:");
        Serial.println(until);
      }
      if (until > 0) {
        onUntil = millis() + until;
      }
    }
  }

  flashInterval = 0; // Reset in case the next update doesn't include values
  const char* flashstart = strstr(message.c_str(), "\"flash\":\"");
  const char* flashempty = strstr(message.c_str(), "\"flash\":\"\"");
  if (flashstart != NULL && flashempty == NULL) {
    if (debugMode) { Serial.println("Found flash"); }
    flashstart += 9; // "flash":"
    const char* flashend = strchr(flashstart, '\"');
    if (flashend != NULL) {
      int flash;
      sscanf(flashstart, "%d", &flash);
      if (debugMode) {
        Serial.print("Flash:");
        Serial.println(flash);
      }
      if (flash > 0) {
        flashInterval = flash;
      }
    }
  }
}

bool debounce() {
  return (millis() - lastDebounceTime) > debounceDelay;
}
