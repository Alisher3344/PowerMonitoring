#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>

const char* WIFI_SSID     = "WIFI_NOMI";
const char* WIFI_PASSWORD = "WIFI_PAROL";
const char* SERVER_URL    = "https://powermonitoring-riea.onrender.com/status";

const int INPUT_PIN = 4;
const bool ACTIVE_HIGH = true;

int lastState = -1;

void connectWifi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("WiFi ulanmoqda");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();
  Serial.print("IP: ");
  Serial.println(WiFi.localIP());
}

bool sendState(const char* state) {
  if (WiFi.status() != WL_CONNECTED) {
    connectWifi();
    if (WiFi.status() != WL_CONNECTED) return false;
  }

  WiFiClientSecure client;
  client.setInsecure();

  HTTPClient http;
  if (!http.begin(client, SERVER_URL)) {
    Serial.println("http.begin xato");
    return false;
  }
  http.addHeader("Content-Type", "application/json");

  String body = String("{\"state\":\"") + state + "\"}";
  int code = http.POST(body);

  Serial.print("POST ");
  Serial.print(state);
  Serial.print(" -> ");
  Serial.println(code);

  http.end();
  return code >= 200 && code < 300;
}

void setup() {
  Serial.begin(115200);
  delay(200);
  pinMode(INPUT_PIN, INPUT_PULLDOWN);
  connectWifi();
}

void loop() {
  int raw = digitalRead(INPUT_PIN);
  int logical = ACTIVE_HIGH ? raw : !raw;

  if (logical != lastState) {
    delay(30);
    if (digitalRead(INPUT_PIN) == raw) {
      lastState = logical;
      sendState(logical ? "HIGH" : "LOW");
    }
  }

  delay(50);
}
