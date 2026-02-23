# Introduction  

## Overview  

This project introduces an ESP-based LoRa node designed for both transmission (TX) and reception (RX) of long-range wireless data.  

The node supports bidirectional communication and can also operate in repeater mode, enabling it to extend network coverage by receiving and forwarding LoRa packets.  

The primary focus of this project is emergency communication with essential core functionalities. The system is intentionally designed to remain simple, reliable, and robust, ensuring communication capability even in infrastructure-independent or critical situations.  

The goal of this implementation is to provide a flexible and modular LoRa communication platform suitable for experimentation, prototyping, and low-power IoT deployments.

---

## What You Need to Do

You will need to add the following URL to your Arduino IDE:

https://github.com/Heltec-Aaron-Lee/WiFi_Kit_series/releases/download/0.0.7/package_heltec_esp32_index.json

1. Go to **File → Preferences**  
   Search for **"Additional Boards Manager URLs"** and paste the URL there.

2. Go to **Tools → Board → Boards Manager**  
   Search for **"Heltec"** and install the latest version.

3. In the IDE, go to **Select Board** and choose:  
   **WiFi LoRa 32 (V3) / Wireless Shell (V3) / Wireless Stick Lite (V3)**

> **Note:** Your ESP board must be plugged into your PC.