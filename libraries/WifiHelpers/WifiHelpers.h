#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClient.h>
#include <ArduinoWebsockets.h>

#define UpstairsWifi 0
#define BasementWifi 1

bool debugMode = true;

struct wifiCred {
  char* ssid;
  char* password;
};

const char* authorization = "Basic Base64EncodedUsernameAndPassword";
const wifiCred upstairs_creds = { "WifiSSID", "WifiPassword" };
const wifiCred basement_creds = { "WifiSSID2", "WifiPassword" };
const wifiCred wifis[2] = { upstairs_creds, basement_creds };

const char* websockets_server = "wss://ardesian.com/cable";

using namespace websockets;
WebsocketsClient client;

bool wifiConnected = false; // Single use for initial connection and set up
bool wsConnected = false;

unsigned long wifiDebounceDelay = 2000;
unsigned long lastWifiConnectTime = 0;

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

void _onMessageCallback(WebsocketsMessage message) {
  if (!strstr(message.c_str(), "\"type\":\"ping\"")) {
    if (debugMode) { Serial.print("Got Message: "); }
    if (debugMode) { Serial.println(message.c_str()); }
    fn_MessageCallback(message);
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

void wifiLoop() {
  if (wsConnected) {
    client.poll();
  } else {
    connectWifi();
  }
}
