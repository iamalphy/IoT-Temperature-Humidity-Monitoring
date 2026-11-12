/*
 * Receiver side - Temperature and Humidity Monitoring System with Blynk
 * Master's case study (MSS-M) - IoT
 *
 * Hardware: ESP8266 + LoRa 433 MHz (receiver)
 *
 * Receives temperature and humidity over the LoRa radio link, parses the
 * values, and pushes them to the Blynk IoT cloud dashboard (Virtual Pins
 * V1 = temperature, V2 = humidity).
 *
 * Reliability features
 * --------------------
 * - Offline ring buffer: readings received while Wi-Fi/Blynk is down are kept
 *   in RAM and flushed to the cloud once the connection returns, so short
 *   connectivity interruptions do not lose data.
 * - Non-blocking reconnect: the cloud link is retried instead of blocking the
 *   receiver, so radio packets keep being read during an outage.
 * - Hardware watchdog: the ESP8266 watchdog is fed during long operations.
 * - Supervised restart: if the radio stays silent or the cloud link cannot be
 *   re-established for too long, the board restarts itself to recover from a
 *   stuck state.
 *
 * Libraries required:
 *   - LoRa (sandeepmistry/Arduino-LoRa)
 *   - Blynk (Blynk Library v1.x for Blynk IoT)
 *   - ESP8266WiFi (built-in)
 *
 * Wiring (ESP8266 <-> LoRa):
 *   LoRa   -> ESP8266 GPIO
 *   VCC    -> 3.3V
 *   GND    -> GND
 *   SCK    -> GPIO14
 *   MISO   -> GPIO12
 *   MOSI   -> GPIO13
 *   NSS    -> GPIO15
 *   RST    -> GPIO5
 *   DIO0   -> GPIO4
 *
 * Blynk app setup:
 *   1. Create a new device template "Temperature and Humidity Monitor"
 *   2. Add Virtual Pins: V1 = Temperature, V2 = Humidity
 *   3. Paste your Template ID and Auth Token below
 *   4. Set your Wi-Fi SSID and password
 */

// ---------- FILL IN YOUR OWN VALUES (do not share real credentials) ----------
#define BLYNK_TEMPLATE_ID "YOUR_BLYNK_TEMPLATE_ID"
#define BLYNK_TEMPLATE_NAME "Temperature and Humidity Monitor"
#define BLYNK_AUTH_TOKEN "YOUR_BLYNK_AUTH_TOKEN"
// -----------------------------------------------------------------------------

#include <SPI.h>
#include <LoRa.h>
#include <BlynkSimpleEsp8266.h>

// LoRa Pins
#define SCK 14
#define MISO 12
#define MOSI 13
#define SS 15
#define RST 5
#define DI0 4

// Wi-Fi Credentials - replace with your own
char ssid[] = "YOUR_WIFI_SSID";
char pass[] = "YOUR_WIFI_PASSWORD";

// --- reliability configuration --------------------------------------------
const uint8_t BUFFER_SIZE = 32;                    // readings kept while offline
const unsigned long WDT_FEED_INTERVAL_MS = 1000;   // hardware watchdog feed period
const unsigned long RECONNECT_RETRY_MS = 5000;     // cloud reconnect attempt spacing
const unsigned long LORA_SILENCE_RESTART_MS = 600000UL;   // 10 min silence -> restart
const unsigned long BLYNK_DOWN_RESTART_MS = 900000UL;     // 15 min offline -> restart
const unsigned long FLUSH_SPACING_MS = 60;         // gap between buffered sends

struct Reading {
  float temperature;
  float humidity;
  unsigned long stamp;
};

Reading buffer[BUFFER_SIZE];
uint8_t bufferHead = 0;
uint8_t bufferCount = 0;
unsigned long readingsReceived = 0;
unsigned long readingsDropped = 0;

unsigned long lastPacketMs = 0;
unsigned long lastFeedMs = 0;
unsigned long lastReconnectMs = 0;
unsigned long lastFlushMs = 0;
unsigned long blynkDownSince = 0;
bool sawFirstPacket = false;

// Forward declarations
void parseLoRaData(const String &data);
void enqueue(float temperature, float humidity);
void flushOne();
void superviseLinks();

void setup() {
  Serial.begin(115200);
  delay(200);

  // Non-blocking start: attempt the Wi-Fi/cloud link, but never block the
  // receiver forever if the router or Blynk is unreachable at power-on.
  WiFi.begin(ssid, pass);
  Blynk.config(BLYNK_AUTH_TOKEN);
  Blynk.connect(10);

  LoRa.setPins(SS, RST, DI0);
  if (!LoRa.begin(433E6)) {
    Serial.println("LoRa Initialization Failed!");
    while (1) {
      ESP.wdtFeed();
      delay(100);
    }
  }
  Serial.println("LoRa Initialized.");

  lastFeedMs = millis();
}

void loop() {
  superviseLinks();

  int packetSize = LoRa.parsePacket();
  if (packetSize) {
    String receivedData = "";
    while (LoRa.available()) {
      receivedData += (char)LoRa.read();
    }
    lastPacketMs = millis();
    sawFirstPacket = true;
    readingsReceived++;
    Serial.print("Received: ");
    Serial.println(receivedData);
    parseLoRaData(receivedData);
  }

  // Push buffered readings whenever the cloud link is available.
  flushOne();

  Blynk.run();
}

void parseLoRaData(const String &data) {
  int tempIndex = data.indexOf("Temperature :");
  int humIndex = data.indexOf("Humidity :");
  if (tempIndex < 0 || humIndex < 0) {
    Serial.println("Invalid data format received.");
    return;
  }
  tempIndex += 13;  // length of "Temperature :"
  humIndex += 10;   // length of "Humidity :"

  int tempEnd = data.indexOf(" ", tempIndex);
  if (tempEnd < 0) tempEnd = data.indexOf(",", tempIndex);
  if (tempEnd < 0) {
    Serial.println("Could not parse temperature value.");
    return;
  }

  float temperature = data.substring(tempIndex, tempEnd).toFloat();
  float humidity = data.substring(humIndex).toFloat();

  Serial.print("Parsed T=");
  Serial.print(temperature);
  Serial.print(" H=");
  Serial.println(humidity);

  enqueue(temperature, humidity);
}

void enqueue(float temperature, float humidity) {
  Reading r;
  r.temperature = temperature;
  r.humidity = humidity;
  r.stamp = millis();

  if (bufferCount == BUFFER_SIZE) {
    // Buffer full: drop the oldest sample so the newest state is always kept.
    bufferHead = (bufferHead + 1) % BUFFER_SIZE;
    bufferCount--;
    readingsDropped++;
  }

  uint8_t tail = (bufferHead + bufferCount) % BUFFER_SIZE;
  buffer[tail] = r;
  bufferCount++;
}

void flushOne() {
  if (bufferCount == 0 || !Blynk.connected()) {
    return;
  }
  unsigned long now = millis();
  if (now - lastFlushMs < FLUSH_SPACING_MS) {
    return;
  }
  lastFlushMs = now;

  Reading r = buffer[bufferHead];
  Blynk.virtualWrite(V1, r.temperature);
  Blynk.virtualWrite(V2, r.humidity);

  bufferHead = (bufferHead + 1) % BUFFER_SIZE;
  bufferCount--;

  if (bufferCount == 0) {
    Serial.print("Buffer flushed. Received=");
    Serial.print(readingsReceived);
    Serial.print(" Dropped=");
    Serial.println(readingsDropped);
  }
}

void superviseLinks() {
  const unsigned long now = millis();

  // 1) Hardware watchdog: keep feeding it so long operations cannot reset us.
  if (now - lastFeedMs >= WDT_FEED_INTERVAL_MS) {
    ESP.wdtFeed();
    lastFeedMs = now;
  }

  // 2) Cloud link supervision: retry without blocking, restart if hopeless.
  if (Blynk.connected()) {
    blynkDownSince = 0;
  } else {
    if (blynkDownSince == 0) {
      blynkDownSince = now;
      Serial.println("Blynk disconnected - buffering locally.");
    }
    if (now - lastReconnectMs >= RECONNECT_RETRY_MS && WiFi.status() == WL_CONNECTED) {
      lastReconnectMs = now;
      Blynk.connect(5);
    }
    if (blynkDownSince != 0 && (now - blynkDownSince) > BLYNK_DOWN_RESTART_MS) {
      Serial.println("Cloud unreachable for too long - restarting.");
      delay(100);
      ESP.restart();
    }
  }

  // 3) Radio supervision: a silent LoRa link means the receiver is stuck.
  if (sawFirstPacket && (now - lastPacketMs) > LORA_SILENCE_RESTART_MS) {
    Serial.println("No LoRa packets for too long - restarting.");
    delay(100);
    ESP.restart();
  }
}
