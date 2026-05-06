#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>

const char* WIFI_SSID     = "alisher";
const char* WIFI_PASSWORD = "19761980";
const char* SERVER_URL    = "https://powermonitoring-riea.onrender.com/status";

const int INPUT_PIN = 4;
const bool ACTIVE_HIGH = true;

int lastState = -1;
unsigned long lastDebug = 0;

void connectWifi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("WiFi ulanmoqda");
  unsigned long t0 = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - t0 < 20000) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();
  if (WiFi.status() == WL_CONNECTED) {
    Serial.print("IP: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("WiFi ulanmadi!");
  }
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

  Serial.print(">>> POST ");
  Serial.print(state);
  Serial.print(" -> ");
  Serial.println(code);
  if (code > 0) {
    Serial.println(http.getString());
  }

  http.end();
  return code >= 200 && code < 300;
}

void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println();
  Serial.println("=== Power Monitor ESP32 ===");
  Serial.print("INPUT_PIN: GPIO");
  Serial.println(INPUT_PIN);

  pinMode(INPUT_PIN, INPUT_PULLDOWN);

  connectWifi();

  int raw = digitalRead(INPUT_PIN);
  int logical = ACTIVE_HIGH ? raw : !raw;
  lastState = logical;
  Serial.print("Boshlang'ich pin holati: ");
  Serial.println(logical ? "HIGH" : "LOW");
  sendState(logical ? "HIGH" : "LOW");
}

void loop() {
  int raw = digitalRead(INPUT_PIN);
  int logical = ACTIVE_HIGH ? raw : !raw;

  if (millis() - lastDebug > 1000) {
    lastDebug = millis();
    Serial.print("[debug] pin=");
    Serial.print(raw);
    Serial.print("  logical=");
    Serial.print(logical ? "HIGH" : "LOW");
    Serial.print("  wifi=");
    Serial.println(WiFi.status() == WL_CONNECTED ? "ON" : "OFF");
  }

  if (logical != lastState) {
    delay(30);
    int raw2 = digitalRead(INPUT_PIN);
    if (raw2 == raw) {
      lastState = logical;
      Serial.print("*** O'ZGARISH: ");
      Serial.println(logical ? "HIGH" : "LOW");
      sendState(logical ? "HIGH" : "LOW");
    }
  }

  delay(20);
}
