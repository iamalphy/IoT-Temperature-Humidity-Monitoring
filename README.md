# IoT Temperature and Humidity Monitoring System with Blynk

Wireless temperature and humidity monitoring system that streams sensor data from an Arduino-based edge node to a Blynk IoT cloud dashboard over a LoRa 433 MHz radio link.

**Type:** Master's case study (MSS-M, AI for Smart Sensors and Actuators) — Technische Hochschule Deggendorf
**Tech stack:** Arduino (C++) · LoRa 433 MHz · ESP8266 · DHT11 · Blynk IoT platform

## System architecture

```
[ DHT11 sensor ] → [ Arduino Uno + LoRa TX ]  ~~~ LoRa 433 MHz (wireless) ~~~  [ LoRa RX + ESP8266 ] → [ Wi-Fi ] → [ Blynk cloud dashboard ]
   (measures)         (transmits every 2s)                                          (receives, parses)      (mobile app, V1/V2)
```

- **Sender node:** Arduino Uno reads temperature and humidity from a DHT11 and transmits the values over LoRa every 2 seconds.
- **Receiver node:** ESP8266 receives the LoRa packets, parses the values, and pushes them to the Blynk IoT platform over Wi-Fi.
- **Dashboard:** temperature and humidity are displayed in real time on the Blynk mobile app (Virtual Pins V1 and V2).

## Hardware

| Component | Role |
|---|---|
| Arduino Uno | Sender microcontroller |
| DHT11 | Temperature and humidity sensor |
| LoRa 433 MHz (x2) | Wireless data link (one per side) |
| ESP8266 | Receiver microcontroller with Wi-Fi |
| Jumper wires + breadboard | Connections |

## Wiring

**Sender (Arduino Uno):**

```
DHT11  -> Arduino Uno
GND    -> GND
VCC    -> 5V
DATA   -> D2

LoRa   -> Arduino Uno
VCC    -> 3.3V   GND  -> GND
NSS    -> D10    RST  -> D9
DIO0   -> D2     SCK  -> D13
MISO   -> D12    MOSI -> D11
```

**Receiver (ESP8266):**

```
LoRa   -> ESP8266 GPIO
VCC    -> 3.3V   GND  -> GND
SCK    -> GPIO14 (D5)
MISO   -> GPIO12 (D6)
MOSI   -> GPIO13 (D7)
NSS    -> GPIO15 (D8)
RST    -> GPIO5  (D1)
DIO0   -> GPIO4  (D2)
```

## Setup

1. Install the Arduino IDE and the ESP8266 board package.
2. Install libraries: `LoRa` (sandeepmistry), `DHT sensor library` (Adafruit), `Blynk` v1.x.
3. Create a device template in the Blynk app: "Temperature and Humidity Monitor", with Virtual Pins **V1 = Temperature** and **V2 = Humidity**.
4. Copy your Template ID and Auth Token into `receiver/receiver.ino`, and set your Wi-Fi SSID/password.
5. Upload `sender/sender.ino` to the Arduino Uno.
6. Upload `receiver/receiver.ino` to the ESP8266.
7. Open the Blynk app and watch the live values.

> **Security note:** replace all placeholder credentials (`YOUR_BLYNK_AUTH_TOKEN`, `YOUR_WIFI_SSID`, etc.) with your own values. Never commit real credentials to a public repository.

## Project structure

```
IoT-Temperature-Humidity-Monitoring/
├── README.md
├── sender/
│   └── sender.ino          # Arduino Uno + DHT11 + LoRa transmitter
└── receiver/
    └── receiver.ino        # ESP8266 + LoRa receiver + Blynk integration
```

## License

Educational use only.
