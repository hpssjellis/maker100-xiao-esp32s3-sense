# maker100-xiao-esp32s3-sense

Started Feb 26th, 2025

A combination of maker100-eco with a LoRa capable Seeedstudio module for the IoT part.

Also check the curriculum at https://github.com/hpssjellis/maker100-curriculum


Based on the working Maker100-eco course at https://github.com/hpssjellis/maker100-eco but with the Seeedstudio LoRa module  


$9.90 USD LoRa Module https://www.seeedstudio.com/Wio-SX1262-with-XIAO-ESP32S3-p-5982.html

and the amazing $13.99 Xiao-Esp32S3Sense   https://www.seeedstudio.com/XIAO-ESP32S3-Sense-p-5639.html

![image](https://github.com/user-attachments/assets/5bb9f204-d206-43c6-a4c4-84933e369dcf)


More comming.







ChatGPT update of the maker100 curriculum with the new xiao-LoRa module. I will change these just need a place to put them.


<br><br><h2 name="iot"> IoT Internet of Things Connectivity</h2><br>

1. **IoT01-WiFi-Webserver:** Make your microcontroller into a LOCAL WIFI webserver. Note: Unless your IT department approves, this webserver will not be connected to the internet. [my example](https://github.com/hpssjellis/maker100-eco/blob/main/README.md#a47)  
1. **IoT02-camera-streaming-webserver:** Stream your camera feed to a local webpage. This is a default ESP32-S3 program; just uncomment the necessary sections. Find it under Examples → ESP32 → Camera → cameraWebServer.  
1. **IoT03-sound-streaming:** Stream live sound over WiFi. This is challenging, but students can experiment with ESP32-S3's I2S features.  
1. **IoT04-BLE:** Get the microcontroller to connect to an app like NRF Connect by Nordic. [Apple](https://apps.apple.com/us/app/nrf-connect-for-mobile/id1054362403) ...  [Android](https://play.google.com/store/apps/details?id=no.nordicsemi.android.mcp&hl=en&pli=1)  [my example](https://github.com/hpssjellis/maker100-eco/blob/main/README.md#a52) BLE coding is tricky; use AI tools for assistance.
1. **IoT05-LoRa (Meshtastic Basic):** With two Seeed Studio Wio SX1262 modules, create a simple text-messaging system over LoRa. Follow Meshtastic documentation for setup and explore creative use cases like off-grid communication.  
1. **IoT06-LoRaWan (Advanced):** Connect one LoRa module to a LoRaWAN network like TTN or Helium. You’ll need cloud integration and credentials for a gateway. Experiment with sending environmental sensor data.  
1. **IoT07-Meshtastic Network:** Create a decentralized communication network using multiple Wio SX1262 LoRa modules. Test messaging across different distances and obstacles, exploring real-world applications like emergency communication.  
1. **IoT08-ESPNOW:** ESP32 modules can communicate without a router using ESPNOW. Use the default example to send and receive messages between microcontrollers.  
1. **IoT09-ethernet-poe:** If you have an Ethernet module, create a webserver using Ethernet. Advantages: 1. No passwords needed, 2. POE (Power over Ethernet). [my example](https://github.com/hpssjellis/maker100#21) (PortentaH7 with Ethernet Vision Shield).  
1. **IoT10-multiplexer:** Some microcontrollers lack sufficient pins for projects. Use a multiplexer to expand I/O capacity. This is useful for controlling many sensors or displays.  
1. **IoT11-UART:** Connect two microcontrollers using the UART serial protocol (RX/TX crossed). [my example](https://github.com/hpssjellis/maker100-eco/blob/main/README.md#a38b)   
1. **IoT12-I2C:** Use the I2C protocol to connect and exchange information between two microcontrollers. Note: Pull up SDA and SCL lines to 3.3V using 4.7 kOhm resistors. [my example](https://github.com/hpssjellis/maker100-eco/blob/main/README.md#a38a)   
1. **IoT13-SPI:** Use SPI (MOSI, MISO, SCK, SS) to exchange information between microcontrollers. This can be difficult, as most MCUs act as controllers while sensors function as peripherals.

### New Additions for Seeed Wio SX1262:

1. **IoT14-Meshtastic GPS Tracking:** Use the Wio SX1262’s Meshtastic capabilities to build a basic GPS tracking system for hiking or cycling.
1. **IoT15-LoRa Remote Sensor Network:** Deploy multiple LoRa nodes to collect and transmit environmental data (temperature, humidity, air quality) to a central station.
1. **IoT16-LoRa Secure Messaging:** Implement encryption to send secure messages between LoRa modules.
1. **IoT17-LoRa to WiFi Bridge:** Use an ESP32-S3 as a bridge to receive LoRa messages and send them to a web dashboard via WiFi.


