#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClient.h>
#include <ArduinoWebsockets.h>

#define UpstairsWifi 0
#define BasementWifi 1

bool debugMode = true;

struct wifiCred {
  String ssid;
  String password;
};

// const char* authorization = "Basic Base64EncodedUsernameAndPassword";
// const wifiCred upstairs_creds = { "WifiSSID", "WifiPassword" };
// const wifiCred basement_creds = { "WifiSSID2", "WifiPassword" };
const char* authorization = "Basic Um9ja3N0ZXIxNjA6Ynl6bWl4LXRvZGdJaC13b2piZTc="; // NOCOMMIT
const wifiCred upstairs_creds = { "Nighthawk", "JuniperPoint!" }; // NOCOMMIT
const wifiCred basement_creds = { "CenturyLink7046", "5ut8pd6rf8ek9v" }; // NOCOMMIT
const wifiCred wifis[2] = { upstairs_creds, basement_creds };

// const char* websockets_server = "ws://localhost:3141/cable";
// Can't connect to insecure WS.
const char* websockets_server = "wss://ardesian.com/cable";

using namespace websockets;
WebsocketsClient client;

bool wifiConnected = false; // Single use for initial connection and set up
bool wsConnected = false;

unsigned long napTime = 1000 * 60 * 60 * 24 * 2.3; // 4.3 days after startup
unsigned long memoryCheckdelay = 1000 * 60; // One minute
unsigned long windDownDelay = 1000 * 60 * 5; // Five minutes
float startFreeMem = 0;
unsigned long memoryCheckLast = 0;
int tiredRatio = 25;

unsigned long wifiDebounceDelay = 2000;
unsigned long lastWifiConnectTime = 0;
unsigned long lastPingTime = 0;

typedef void (*wsFnPtr)(WebsocketsMessage);
static wsFnPtr fn_MessageCallback;

typedef void (*voidFnPtr)(void);
static voidFnPtr fn_SendState;

String subscriptionStr;

void subscribe() {
  client.connect(websockets_server);
  client.send(subscriptionStr);
  client.ping();
  fn_SendState();
}

void wifiInit(int wifiIdx, String new_subscriptionStr, voidFnPtr fn_SendState_set, wsFnPtr fn_MessageCallback_set) {//(void (*fnSendState_set)(), void (*fnMessageCallback_set)(WebsocketsMessage)) {
  if (debugMode) { Serial.println("Wifi connecting"); }
  wifiCred creds = wifis[wifiIdx];
  WiFi.begin(creds.ssid, creds.password);
  subscriptionStr = new_subscriptionStr;
  fn_SendState = fn_SendState_set;
  fn_MessageCallback = fn_MessageCallback_set;
}

void _onEventsCallback(WebsocketsEvent event, String data) {
  if (event == WebsocketsEvent::ConnectionOpened) {
    if (debugMode) { Serial.println("Connnection Opened"); }
    wsConnected = true;
  } else if (event == WebsocketsEvent::ConnectionClosed) {
    if (debugMode) { Serial.println("Connnection Closed"); }
    wsConnected = false;
  } else if (event == WebsocketsEvent::GotPing) {
    if (debugMode) { Serial.println("Got a Ping!"); }
  } else if (event == WebsocketsEvent::GotPong) {
    if (debugMode) { Serial.println("Got a Pong!"); }
  }
}

float memFreePercent() {
  float currentFreeMem = ESP.getFreeHeap();
  return (currentFreeMem / startFreeMem) * 100;
}

void _onMessageCallback(WebsocketsMessage message) {
  if (!strstr(message.c_str(), "\"type\":\"ping\"")) {
    if (debugMode) { Serial.print("Got Message: "); }
    if (debugMode) { Serial.println(message.c_str()); }
    if (debugMode) {
      Serial.print("Mem: ");
      Serial.println(memFreePercent());
    }
    fn_MessageCallback(message);
  } else {
    lastPingTime = millis();
  }
}

void connectWifi() {
  if (millis() % 500 > 250) {
    digitalWrite(D0, LOW);
  } else {
    digitalWrite(D0, HIGH);
  }

  if (millis() - lastWifiConnectTime < wifiDebounceDelay) { return; }

  lastWifiConnectTime = millis();

  if (wifiConnected) {
    if (!wsConnected) {
      if (debugMode) { Serial.println("Connecting WS"); }
      subscribe();
    }
  } else {
    if (WiFi.status() == WL_CONNECTED) {
      if (debugMode) { Serial.println("Wifi connected"); }
      digitalWrite(D0, LOW);
      wifiConnected = true;

      WiFi.setAutoReconnect(true);
      WiFi.persistent(true);

      // Setup Callbacks
      client.setInsecure();
      client.addHeader("Authorization", authorization);
      client.onMessage(_onMessageCallback);
      client.onEvent(_onEventsCallback);
    } else {
      if (debugMode) { Serial.println("Failed to connect"); }
    }
  }
}

void checkMemory() {
  if (startFreeMem == 0) { startFreeMem = ESP.getFreeHeap(); }
  if (memoryCheckLast + memoryCheckdelay < millis()) {
    memoryCheckLast = millis();
    if (memoryCheckLast > napTime) { return ESP.restart(); }
    if (memoryCheckLast + windDownDelay > napTime) { return; } // Small delay before naptime to prevent restarting while in use

    if (memFreePercent() < tiredRatio) {
      napTime = memoryCheckLast + windDownDelay;
    }
  }
}

void wifiLoop() {
  checkMemory();

  if (wsConnected) {
    client.poll();
  } else {
    connectWifi();
  }
}
