#include <SX126x.h>

SX126x LoRa;

/*

#define LORA_MISO 8   // confirmed
#define LORA_SCK 7    // confirmed
#define LORA_MOSI 9  // confirmed
#define LORA_CS 41    //NSS
#define LORA_DIO2 38
#define LORA_DIO1 39     // irq
#define LORA_RESET 42
#define LORA_BUSY 40

 
// this file contains binary patch for the SX1262
#include <modules/SX126x/patches/SX126x_patch_scan.h>

SX1262 radio = new Module(LORA_CS, LORA_DIO1, LORA_RESET, LORA_BUSY);
  // int state = radio.begin(frequency, bandwidth, spreadingFactor, codingRate, syncWord, outputPower, preambleLength, 1.6, false);
  // int state = radio.begin(915.0, 500.0, 6, 5, 0x34, 2, 20);  // LoRaWan
  // int state = radio.begin(915.0, 125.0, 7, 5, 0x12, 14, 8);  // LoRa like portenta
*/


void setup() {

  // Begin serial communication
  Serial.begin(115200);
  delay(6000);

  // Begin LoRa radio and set NSS, reset, busy, txen, and rxen pin with connected arduino pins
  Serial.println("Begin LoRa radio");
  int8_t nssPin = 41, resetPin = 42, busyPin = 40, irqPin = 39, txenPin = 43, rxenPin = 44;
  if (!LoRa.begin(nssPin, resetPin, busyPin, irqPin, txenPin, rxenPin)){
    Serial.println("Something wrong, can't begin LoRa radio");
    while(1);
  }

  // Configure TCXO or XTAL used in RF module
  Serial.println("Set RF module to use TCXO as clock reference");
  uint8_t dio3Voltage = SX126X_DIO3_OUTPUT_1_8;
  uint32_t tcxoDelay = SX126X_TCXO_DELAY_10;
  LoRa.setDio3TcxoCtrl(dio3Voltage, tcxoDelay);
  
  // Set frequency to 915 Mhz
  Serial.println("Set frequency to 915 Mhz");
  LoRa.setFrequency(915000000);

  // Set RX gain to boosted gain
  Serial.println("Set RX gain to boosted gain");
  LoRa.setRxGain(SX126X_RX_GAIN_BOOSTED);

  // Configure modulation parameter including spreading factor (SF), bandwidth (BW), and coding rate (CR)
  Serial.println("Set modulation parameters:\n\tSpreading factor = 7\n\tBandwidth = 125 kHz\n\tCoding rate = 4/5");
  uint8_t sf = 7;
  uint32_t bw = 125000;
  uint8_t cr = 5;
  LoRa.setLoRaModulation(sf, bw, cr);

  // Configure packet parameter including header type, preamble length, payload length, and CRC type
  Serial.println("Set packet parameters:\n\tExplicit header type\n\tPreamble length = 12\n\tPayload Length = 15\n\tCRC on");
  uint8_t headerType = SX126X_HEADER_EXPLICIT;
  uint16_t preambleLength = 12;
  uint8_t payloadLength = 15;
  bool crcType = true;
  LoRa.setLoRaPacket(headerType, preambleLength, payloadLength, crcType);

  // Set syncronize word for public network (0x3444)
  //Serial.println("Set syncronize word to 0x3444");
  //LoRa.setSyncWord(0x3444);
  Serial.println("Set syncronize word to 0x12");
  LoRa.setSyncWord(0x12);

  Serial.println("\n-- LORA RECEIVER CONTINUOUS --\n");
  
  // Request for receiving new LoRa packet in RX continuous mode
  LoRa.request(SX126X_RX_CONTINUOUS);
  
}

void loop() {

  // Check for incoming LoRa packet
  const uint8_t msgLen = LoRa.available();
  if (msgLen) {

    // Put received packet to message and counter variable
    char message[msgLen-1];
    uint8_t counter;
    uint8_t i=0;
    while (LoRa.available() > 1){
      message[i++] = LoRa.read();
    }
    counter = LoRa.read();
    
    // Print received message and counter in serial
    Serial.print(message);
    Serial.print("  ");
    Serial.println(counter);

    // Print packet/signal status including package RSSI and SNR
    Serial.print("Packet status: RSSI = ");
    Serial.print(LoRa.packetRssi());
    Serial.print(" dBm | SNR = ");
    Serial.print(LoRa.snr());
    Serial.println(" dB");
    
    // Show received status in case CRC or header error occur
    uint8_t status = LoRa.status();
    if (status == SX126X_STATUS_CRC_ERR) Serial.println("CRC error");
    else if (status == SX126X_STATUS_HEADER_ERR) Serial.println("Packet header error");
    Serial.println();

  }

}
