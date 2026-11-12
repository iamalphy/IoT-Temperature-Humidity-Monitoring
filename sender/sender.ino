/*
 * Sender side - Temperature and Humidity Monitoring System with Blynk
 * Master's case study (MSS-M) - IoT
 *
 * Hardware: Arduino Uno + DHT11 sensor + LoRa 433 MHz (transmitter)
 *
 * Reads temperature and humidity from the DHT11 and transmits the
 * values over a LoRa radio link every SEND_INTERVAL_MS (30 seconds by
 * default). The interval is intentionally well above the DHT11's 2 s
 * minimum sampling period and stays within Blynk's message rate limits.
 *
 * Libraries required:
 *   - LoRa (sandeepmistry/Arduino-LoRa)
 *   - DHT sensor library (Adafruit)
 *
 * Wiring:
 *   DHT11  -> Arduino Uno
 *   GND    -> GND
 *   VCC    -> 5V
 *   DATA   -> D2
 *
 *   LoRa   -> Arduino Uno (default pins)
 *   VCC    -> 3.3V
 *   GND    -> GND
 *   NSS    -> D10
 *   RST    -> D9
 *   DIO0   -> D2
 *   SCK    -> D13
 *   MISO   -> D12
 *   MOSI   -> D11
 */

#include <LoRa.h>
#include <DHT.h>

#define DHTPIN 2 // DHT11 data pin
#define DHTTYPE DHT11

// --- timing configuration -------------------------------------------------
// Transmit one reading every 30 seconds. Lower this only if you accept more
// frequent DHT11 reads and higher Blynk traffic.
const unsigned long SEND_INTERVAL_MS = 30000UL;

DHT dht(DHTPIN, DHTTYPE);

unsigned long lastSendMs = 0;
bool firstSend = true;

void setup() {
  Serial.begin(9600);
  dht.begin();

  if (!LoRa.begin(433E6)) {
    Serial.println("LoRa initialization failed!");
    while (1);
  }
  Serial.println("LoRa initialized.");
}

void loop() {
  const unsigned long now = millis();

  // Non-blocking scheduler: never delay() the whole loop, so the board stays
  // responsive and the interval does not drift with sensor read time.
  if (!firstSend && (now - lastSendMs) < SEND_INTERVAL_MS) {
    return;
  }
  firstSend = false;
  lastSendMs = now;

  float h = dht.readHumidity();
  float t = dht.readTemperature();

  // DHT11 occasionally returns NaN; retry once before giving up for this cycle.
  if (isnan(h) || isnan(t)) {
    delay(2000);
    h = dht.readHumidity();
    t = dht.readTemperature();
  }

  if (isnan(h) || isnan(t)) {
    Serial.println("Failed to read from DHT sensor - skipping this cycle.");
    return;
  }

  String data = "Temperature :" + String(t) + " , " + "Humidity :" + String(h);
  Serial.println("Sending: " + data);

  LoRa.beginPacket();
  LoRa.print(data);
  LoRa.endPacket();

  Serial.println("Sent.");
}
