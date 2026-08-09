/*
 * Receiver side - Temperature and Humidity Monitoring System with Blynk
 * Master's case study (MSS-M) - IoT
 *
 * Hardware: ESP8266 + LoRa 433 MHz (receiver)
 *
 * Receives temperature and humidity over the LoRa radio link,
 * parses the values, and pushes them to the Blynk IoT cloud
 * dashboard (Virtual Pins V1 = temperature, V2 = humidity).
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

String receivedData = "";
float temperature = 0.0;
float humidity = 0.0;

void setup() {
  // Initialize Serial Monitor
  Serial.begin(115200);
  while (!Serial);

  // Initialize Wi-Fi
  Serial.println("Connecting to Wi-Fi...");
  WiFi.begin(ssid, pass);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWi-Fi Connected!");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());

  // Initialize Blynk
  Serial.println("Connecting to Blynk...");
  Blynk.begin(BLYNK_AUTH_TOKEN, ssid, pass);

  // Initialize LoRa
  Serial.println("Initializing LoRa...");
  LoRa.setPins(SS, RST, DI0);
  if (!LoRa.begin(433E6)) {
    Serial.println("LoRa Initialization Failed!");
    while (1);
  }
  Serial.println("LoRa Initialized.");
}

void loop() {
  // Check for LoRa packets
  int packetSize = LoRa.parsePacket();
  if (packetSize) {
    // Read received LoRa data
    receivedData = "";
    while (LoRa.available()) {
      receivedData += (char)LoRa.read();
    }
    Serial.print("Received Data: ");
    Serial.println(receivedData);

    // Parse the data (format: "Temperature :25.60 , Humidity :60.30")
    parseLoRaData(receivedData);

    // Send data to Blynk
    sendToBlynk();
  }

  // Run Blynk
  Blynk.run();
}

void parseLoRaData(String data) {
  // Look for "Temperature :" and "Humidity :" in the data
  int tempIndex = data.indexOf("Temperature :") + 13;  // 13 is the length of "Temperature :"
  int humIndex = data.indexOf("Humidity :") + 10;      // 10 is the length of "Humidity :"
  if (tempIndex > 12 && humIndex > 9) {  // Ensure valid indices
    // Extract temperature value
    int tempEnd = data.indexOf(" ", tempIndex);  // Find the space after the temperature
    temperature = data.substring(tempIndex, tempEnd).toFloat();

    // Extract humidity value
    humidity = data.substring(humIndex).toFloat();

    // Debug print
    Serial.print("Parsed Temperature: ");
    Serial.println(temperature);
    Serial.print("Parsed Humidity: ");
    Serial.println(humidity);
  } else {
    Serial.println("Invalid data format received.");
  }
}

void sendToBlynk() {
  Blynk.virtualWrite(V1, temperature);  // Send temperature to Virtual Pin V1
  Blynk.virtualWrite(V2, humidity);     // Send humidity to Virtual Pin V2
  Serial.println("Data sent to Blynk.");
}
