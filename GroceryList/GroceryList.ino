#include <stdarg.h>
#include <string.h>
#include "Pins.h"
#include "WifiHelpers.h"

const int optCount = 5;
const int leds[optCount] = { D8, D7, D6, D5, D0 };
const int btns[optCount] = { D4, D3, D2, D1, RX };
bool onList[optCount] = { false, false, false, false, false };

const int listId = 1;
const char* items[optCount] = { "Bread", "Eggs", "Milk", "Cheese", "Butter" };

bool pressed = false;

unsigned long lastDebounceTime = 0;
unsigned long debounceDelay = 200;

void setup() {
  Serial.begin(115200);
  pinMode(D0, OUTPUT);
  for (int thisLed = 0; thisLed < optCount; thisLed++) {
    pinMode(leds[thisLed], OUTPUT);
  }
  for (int thisBtn = 0; thisBtn < optCount; thisBtn++) {
    pinMode(btns[thisBtn], INPUT_PULLUP);
  }

  wifiInit(
    UpstairsWifi,
    String("{\"command\":\"subscribe\",\"identifier\":\"{\\\"channel\\\":\\\"ListJsonChannel\\\",\\\"channel_id\\\":\\\"list_") + listId + String("\\\"}\"}"),
    getState,
    onMessageCallback
  );
}

void loop() {
  if (debounce()) {
    displayLeds();
  }

  // Wifi connection + retry
  wifiLoop();
}

void post(String data) {
  String str = "{\"command\":\"message\",\"identifier\":\"{\\\"channel\\\":\\\"ListJsonChannel\\\",\\\"channel_id\\\":\\\"list_";
  str += listId;
  str += "\\\"}\",\"data\":\"";
  str += data;
  str += "\"}";
  client.send(str);
}

void addItem(int itemIdx) {
  if (onList[itemIdx]) {
    onList[itemIdx] = false;
    String str = "{\\\"action\\\":\\\"receive\\\",\\\"remove\\\": \\\"";
    str += items[itemIdx];
    str += "\\\"}";
    post(str);
  } else {
    onList[itemIdx] = true;
    String str = "{\\\"action\\\":\\\"receive\\\",\\\"add\\\": \\\"";
    str += items[itemIdx];
    str += "\\\"}";
    post(str);
  }
}

void displayLeds() {
  bool somePressed = false;
  for (int thisBtn=0; thisBtn < optCount; thisBtn++) {
    if (digitalRead(btns[thisBtn]) == LOW) {
      somePressed = true;
      if (!pressed) {
        pressed = true;
        addItem(thisBtn);
      }
    } else {
      digitalWrite(leds[thisBtn], LOW);
    }
  }
  if (!somePressed) {
    pressed = false;
    lastDebounceTime = millis();
  }

  for (int thisItem=0; thisItem < optCount; thisItem++) {
    if (onList[thisItem]) {
      digitalWrite(leds[thisItem], HIGH);
    }
  }
}

void getState() {
  post("{\\\"action\\\":\\\"receive\\\",\\\"get\\\":\\\"true\\\"}");
}

char* dupStr(const char* frozenStr) {
  char* mem = (char*)malloc(strlen(frozenStr) + 1);

  return strcpy(mem, frozenStr);
}

void onMessageCallback(WebsocketsMessage message) {
  char* lwr_message = strlwr(dupStr(message.c_str()));
  // Serial.println(lwr_message);

  for (int thisItem=0; thisItem < optCount; thisItem++) {
    const char* itemName = items[thisItem];
    String quote = "\"";
    char itemChar[1000];
    (quote + items[thisItem] + quote).toCharArray(itemChar, 1000);
    char* lwr_item = strlwr(dupStr(itemChar));

    if (strstr(lwr_message, lwr_item)) {
      onList[thisItem] = true;
    } else {
      onList[thisItem] = false;
    }
  }
}

bool debounce() {
  return (millis() - lastDebounceTime) > debounceDelay;
}
