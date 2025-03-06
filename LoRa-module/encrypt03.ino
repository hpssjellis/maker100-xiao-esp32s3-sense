

Can you fix this code. I don't think the AAES has been initialized.  /*
  ESP32 LoRa with Meshtastic AES Encryption Compatibility
 
  This code extends the RadioLib SX126x Ping-Pong example to work with
  Meshtastic AES-encrypted messages. Meshtastic uses AES-256-CTR for encryption.
 
  For default module settings, see the wiki page
  https://github.com/jgromes/RadioLib/wiki/Default-configuration#sx126x---lora-modem
*/

#include <RadioLib.h>
#include <mbedtls/aes.h>  // ESP32's built-in AES library

#define INITIATING_NODE   // so it sends the first transmission

// LoRa pin definitions
#define LORA_MISO 8
#define LORA_SCK 7
#define LORA_MOSI 9
#define LORA_CS 41    // NSS
#define LORA_DIO2 38
#define LORA_DIO1 39  // IRQ
#define LORA_RESET 42
#define LORA_BUSY 40


// Encryption settings
#define AES_BLOCK_SIZE 16
#define AES_KEY_SIZE 32  // AES-256 uses 32-byte key

// Default channel settings for Meshtastic compatibility
#define MESH_FREQUENCY 915.0     // Default for US region, adjust per your region
#define MESH_BANDWIDTH 125.0     // Meshtastic default
#define MESH_SPREADING_FACTOR 7  // Meshtastic's LongFast default
#define MESH_CODING_RATE 5       // Meshtastic default (4/5)
#define MESH_SYNC_WORD 0x12      // Meshtastic uses 0x12 (or sometimes 0x34 on LoRaWAN)
#define MESH_OUTPUT_POWER 14     // Typical Meshtastic power
#define MESH_PREAMBLE_LENGTH 16   // Meshtastic default

// this file contains binary patch for the SX1262
#include <modules/SX126x/patches/SX126x_patch_scan.h>

SX1262 radio = new Module(LORA_CS, LORA_DIO1, LORA_RESET, LORA_BUSY);

volatile bool operationDone = false;
bool transmitFlag = false;
int transmissionState = RADIOLIB_ERR_NONE;

// Default encryption key (REPLACE WITH YOUR ACTUAL CHANNEL KEY)
// In Meshtastic, the default key is derived from the channel name if not explicitly set
// this has been set for meshtastic FastLong channel name
uint8_t encryptionKey[AES_KEY_SIZE] = {
0xcd, 0x94, 0x94, 0x08, 0x3a, 0x91, 0xc5, 0x99,
0xba, 0x66, 0xca, 0x6a, 0x77, 0x48, 0x39, 0xdd,
0xa2, 0x0b, 0xf6, 0x91, 0xdc, 0x6b, 0x98, 0x8d,
0x62, 0xa7, 0x2a, 0x80, 0x9d, 0x46, 0x2b, 0x42

};





// 0xcd, 0x94, 0x94, 0x08, 0x3a, 0x91, 0xc5, 0x99, 0xba, 0x66, 0xca, 0x6a, 0x77, 0x48, 0x39, 0xdd, 0xa2, 0x0b, 0xf6, 0x91, 0xdc, 0x6b, 0x98, 0x8d, 0x62, 0xa7, 0x2a, 0x80, 0x9d, 0x46, 0x2b, 0x42,


// AES context for encryption/decryption
mbedtls_aes_context aes;

#if defined(ESP8266) || defined(ESP32)
ICACHE_RAM_ATTR
#endif
void setFlag(void) {
  operationDone = true;
}

// Function to validate parameters
bool validateParameters(float frequency, float bandwidth, uint8_t spreadingFactor, uint8_t codingRate, uint8_t syncWord, float outputPower, uint16_t preambleLength) {
  if (frequency < 150.0 || frequency > 960.0) {
    Serial.println(F("Error: Frequency must be between 150.0 MHz and 960.0 MHz."));
    return false;
  }
  if (bandwidth != 7.8 && bandwidth != 10.4 && bandwidth != 15.6 && bandwidth != 20.8 && bandwidth != 31.25 &&
      bandwidth != 41.7 && bandwidth != 62.5 && bandwidth != 125.0 && bandwidth != 250.0 && bandwidth != 500.0) {
    Serial.println(F("Error: Invalid bandwidth value."));
    return false;
  }
  if (spreadingFactor < 6 || spreadingFactor > 12) {
    Serial.println(F("Error: Spreading factor must be between 6 and 12."));
    return false;
  }
  if (codingRate < 5 || codingRate > 8) {
    Serial.println(F("Error: Coding rate must be between 5 and 8."));
    return false;
  }
  if (outputPower < -17.0 || outputPower > 22.0) {
    Serial.println(F("Error: Output power must be between -17 dBm and 22 dBm."));
    return false;
  }
  if (preambleLength < 6 || preambleLength > 65535) {
    Serial.println(F("Error: Preamble length must be between 6 and 65535 symbols."));
    return false;
  }
  if (syncWord > 0xFF) {
    Serial.println(F("Error: Sync word must be a valid 1-byte value."));
    return false;
  }
  return true;
}

// Initialize LoRa module with parameter validation
void initRadio() {
  float frequency = MESH_FREQUENCY;
  float bandwidth = MESH_BANDWIDTH;
  uint8_t spreadingFactor = MESH_SPREADING_FACTOR;
  uint8_t codingRate = MESH_CODING_RATE;
  uint8_t syncWord = MESH_SYNC_WORD;
  float outputPower = MESH_OUTPUT_POWER;
  uint16_t preambleLength = MESH_PREAMBLE_LENGTH;

  // Validate parameters
  if (!validateParameters(frequency, bandwidth, spreadingFactor, codingRate, syncWord, outputPower, preambleLength)) {
    Serial.println(F("Error: Invalid parameters. Stopping."));
    while (true) { delay(10); }
  }

  Serial.print(F("[SX1262] Initializing ... "));
  int state = radio.begin(frequency, bandwidth, spreadingFactor, codingRate, syncWord, outputPower, preambleLength, 1.6, false);

  if (state == RADIOLIB_ERR_NONE) {
    Serial.println(F("success!"));
    radio.setDio1Action(setFlag);
  } else {
    Serial.print(F("failed, code "));
    Serial.println(state);
    while (true) { delay(10); }
  }
}

// Initialize AES encryption
void initAES() {
  mbedtls_aes_init(&aes);
  mbedtls_aes_setkey_enc(&aes, encryptionKey, 256);  // 256 bits for AES-256
  Serial.println(F("AES encryption initialized"));
}

// Function to encrypt data using AES-256-CTR mode
// Meshtastic uses a nonce (IV) composed of packet ID and sender ID
void encryptData(uint8_t* input, uint8_t* output, size_t length, uint8_t* nonce) {
  uint8_t counter[AES_BLOCK_SIZE] = {0};
  uint8_t stream_block[AES_BLOCK_SIZE] = {0};
  size_t nc_offset = 0;
 
  // Copy the nonce to the counter block
  memcpy(counter, nonce, 8);  // Meshtastic uses 8-byte nonce
 
  // Perform CTR mode encryption
  mbedtls_aes_crypt_ctr(&aes, length, &nc_offset, counter, stream_block, input, output);
}

// Function to decrypt data using AES-256-CTR mode
// For CTR mode, encryption and decryption are identical operations
void decryptData(uint8_t* input, uint8_t* output, size_t length, uint8_t* nonce) {
  encryptData(input, output, length, nonce);  // CTR mode is symmetric
}

// Meshtastic packet structure (simplified)
typedef struct {
  uint32_t packetId;     // Packet identifier
  uint32_t from;         // Sender node ID
  uint8_t encrypted;     // Flag indicating if payload is encrypted
  uint8_t data[240];     // Payload data (max size)
  size_t dataLength;     // Actual data length
} MeshtasticPacket;

// Send packet with Meshtastic compatibility
void sendPacket(const char* message) {
  Serial.print(F("[SX1262] Sending packet ... "));
  Serial.print(message);
  Serial.print(F(" "));
 
  // Create a Meshtastic packet
  MeshtasticPacket packet;
  packet.packetId = millis();  // Use current time as packet ID
  packet.from = 0x12345678;    // Example node ID
  packet.encrypted = 1;        // Enable encryption
 
  // Copy message to data
  size_t messageLen = strlen(message);
  if (messageLen > 240) messageLen = 240;  // Ensure we don't overflow
 
  // Create nonce from packetId and from fields
  uint8_t nonce[8];
  memcpy(nonce, &packet.packetId, 4);
  memcpy(nonce + 4, &packet.from, 4);
 
  // Encrypt the message
  uint8_t encryptedData[240];
  encryptData((uint8_t*)message, encryptedData, messageLen, nonce);
 
  // Copy encrypted data to packet
  memcpy(packet.data, encryptedData, messageLen);
  packet.dataLength = messageLen;
 
  // Serialize the packet (simplified - actual Meshtastic uses Protobuf)
  uint8_t serializedPacket[256];
  size_t packetSize = 0;
 
  // Simple serialization format (for demonstration - not actual Meshtastic format)
  memcpy(serializedPacket, &packet.packetId, 4);
  packetSize += 4;
  memcpy(serializedPacket + packetSize, &packet.from, 4);
  packetSize += 4;
  serializedPacket[packetSize++] = packet.encrypted;
  serializedPacket[packetSize++] = packet.dataLength;
  memcpy(serializedPacket + packetSize, packet.data, packet.dataLength);
  packetSize += packet.dataLength;
 
  // Send the packet
  transmissionState = radio.startTransmit(serializedPacket, packetSize);
  transmitFlag = true;
}

// Receive and process Meshtastic packet
void receivePacket() {
  uint8_t data[256];
  size_t dataLen = 0;
 
  int state = radio.readData(data, dataLen);
 
  if (state == RADIOLIB_ERR_NONE && dataLen > 0) {
    Serial.println(F("[SX1262] Received packet!"));
   
    // Extract Meshtastic packet fields (simplified - actual parsing would be more complex)
    MeshtasticPacket receivedPacket;
    size_t offset = 0;
   
    // Parse header
    memcpy(&receivedPacket.packetId, data, 4);
    offset += 4;
    memcpy(&receivedPacket.from, data + offset, 4);
    offset += 4;
    receivedPacket.encrypted = data[offset++];
    size_t msgLen = data[offset++];
   
    // Copy encrypted data
    memcpy(receivedPacket.data, data + offset, msgLen);
    receivedPacket.dataLength = msgLen;
   
    Serial.print(F("[SX1262] Packet ID: "));
    Serial.println(receivedPacket.packetId);
    Serial.print(F("[SX1262] From: "));
    Serial.println(receivedPacket.from, HEX);
   
    // If encrypted, decrypt the payload
    if (receivedPacket.encrypted) {
      uint8_t decryptedData[240];
      uint8_t nonce[8];
     
      // Create nonce from packetId and from fields
      memcpy(nonce, &receivedPacket.packetId, 4);
      memcpy(nonce + 4, &receivedPacket.from, 4);
     
      // Decrypt the data
      decryptData(receivedPacket.data, decryptedData, receivedPacket.dataLength, nonce);
     
      // Null-terminate the string for printing
      decryptedData[receivedPacket.dataLength] = '\0';
     
      Serial.print(F("[SX1262] Decrypted Data: "));
      Serial.println((char*)decryptedData);
    } else {
      // Data is not encrypted, just display it
      receivedPacket.data[receivedPacket.dataLength] = '\0';
      Serial.print(F("[SX1262] Data: "));
      Serial.println((char*)receivedPacket.data);
    }
   
    Serial.print(F("[SX1262] RSSI: "));
    Serial.print(radio.getRSSI());
    Serial.println(F(" dBm"));
    Serial.print(F("[SX1262] SNR: "));
    Serial.print(radio.getSNR());
    Serial.println(F(" dB"));
  } else {
    Serial.print(F("readData failed, code "));
    Serial.println(state);
  }
}



// Setup function
void setup() {
  Serial.begin(115200);
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, 1);
  randomSeed(analogRead(0)); // Seed random with noise from an analog pin
  delay(5000);
  digitalWrite(LED_BUILTIN, 0);
 
  // Initialize radio
  initRadio();
 
  // Initialize AES encryption
  initAES();
 

 
  #if defined(INITIATING_NODE)
    sendPacket("ESP32 Meshtastic Test");
  #else
    radio.startReceive();
  #endif
}

// Main loop
void loop() {
  if (operationDone) {
    operationDone = false;
    if (transmitFlag) {
      if (transmissionState == RADIOLIB_ERR_NONE) {
        Serial.println(F("transmission finished!"));
      } else {
        Serial.print(F("failed, code "));
        Serial.println(transmissionState);
      }
      radio.startReceive();
      transmitFlag = false;
    } else {
      receivePacket();
      digitalWrite(LED_BUILTIN, 1);
      delay(5000);
      digitalWrite(LED_BUILTIN, 0);

      char myMessage[40]; // Buffer to store the formatted string
      int myRandomNumber = random(100, 1000);
      sprintf(myMessage, "ESP32Mesh:%d", myRandomNumber); // Format string
      sendPacket(myMessage);
    }
  }
}

