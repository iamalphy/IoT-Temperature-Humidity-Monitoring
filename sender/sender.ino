/*
 * Sender side - Temperature and Humidity Monitoring System with Blynk
 * Master's case study (MSS-M) - IoT
 *
 * Hardware: Arduino Uno + DHT11 sensor + LoRa 433 MHz (transmitter)
 *
 * Reads temperature and humidity from the DHT11 and transmits the
 * values over a LoRa radio link every 2 seconds.
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
DHT dht(DHTPIN, DHTTYPE);

void setup() {
  Serial.begin(9600);
  while (!Serial);
  dht.begin();

  if (!LoRa.begin(433E6)) {
    Serial.println("LoRa initialization failed!");
    while (1);
  }
  Serial.println("LoRa initialized.");
}

void loop() {
  float h = dht.readHumidity();
  float t = dht.readTemperature();

  if (isnan(h) || isnan(t)) {
    Serial.println("Failed to read from DHT sensor!");
    return;
  }

  String data = "Temperature :" + String(t) + " , " + "Humidity :" + String(h);
  Serial.println("Sending: " + data);

  LoRa.beginPacket();
  LoRa.print(data);
  LoRa.endPacket();

  delay(2000); // Delay between readings
}
