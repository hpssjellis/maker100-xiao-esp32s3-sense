/*
  RadioLib SX126x Ping-Pong Example

  This example is intended to run on two wio_sx1262_with_xiao_esp32s3_kit_class,
  and send packets between the two.

  For default module settings, see the wiki page
  https://github.com/jgromes/RadioLib/wiki/Default-configuration#sx126x---lora-modem

  For full API reference, see the GitHub Pages
  https://jgromes.github.io/RadioLib/

  error codes 
  https://jgromes.github.io/RadioLib/group__status__codes.html


  Seeed getting started

  https://wiki.seeedstudio.com/wio_sx1262_with_xiao_esp32s3_kit_class/

// MUST BE SET IN RADIO CONFIGURATION, LORA --> OVERRIDE FREQUENCY 915.0
// Must reset the LoRa module
  
*/

// Sender Program
// Receiver Program

#include <Arduino.h>
#include <RadioLib.h>
#include "mbedtls/md.h"
#include "mbedtls/aes.h"

#define LORA_MISO 8
#define LORA_SCK 7
#define LORA_MOSI 9
#define LORA_CS 41
#define LORA_DIO2 38
#define LORA_DIO1 39
#define LORA_RESET 42
#define LORA_BUSY 40

#define AES_KEY_SIZE 32  // 256-bit key (Meshtastic standard)
#define MY_ITERATIONS 100000  // Meshtastic's default iteration count

#include <modules/SX126x/patches/SX126x_patch_scan.h>

SX1262 radio = new Module(LORA_CS, LORA_DIO1, LORA_RESET, LORA_BUSY);

volatile bool operationDone = false;
unsigned long lastSettingChange = 0;
const unsigned long settingDuration = 65000;

float bandwidthOptions[] = {250.0};   //500.0, 250.0, 125.0
uint8_t spreadingFactorOptions[] = {11};   // best 11   //10,11,12
uint8_t codingRateOptions[] = {5, 6, 7, 8};
uint8_t syncWordOptions[] = {0x2B};

int currentBW = 0;
int currentSF = 0;
int currentCR = 0;
int currentSW = 0;



struct myProtoMessage {
    int32_t myId;
    char myText[32];  // Ensure enough space
};

uint8_t myBuffer[36];  // 4 bytes for ID (max varint size) + 32 bytes for text

void myProtoStore(myProtoMessage &message) {
    uint8_t *ptr = myBuffer;

    // Encode ID as Protobuf Varint (handles small and large values)
    int32_t id = message.myId;
    while (id > 127) {
        *ptr++ = (id & 0x7F) | 0x80;  // Set MSB for multi-byte
        id >>= 7;
    }
    *ptr++ = id & 0x7F;  // Last byte (no MSB set)

    // Store length of the text (Protobuf-style length prefix)
    uint8_t textLength = strlen(message.myText);
    *ptr++ = textLength;

    // Copy text
    memcpy(ptr, message.myText, textLength);
}

void myProtoRecover(myProtoMessage &message) {
    uint8_t *ptr = myBuffer;

    // Decode Protobuf Varint for ID
    int32_t id = 0;
    int shift = 0;
    while (*ptr & 0x80) {  // While MSB is set
        id |= (*ptr++ & 0x7F) << shift;
        shift += 7;
    }
    id |= (*ptr++ & 0x7F) << shift;
    message.myId = id;

    // Read string length
    uint8_t textLength = *ptr++;

    // Copy text
    memcpy(message.myText, ptr, textLength);
    message.myText[textLength] = '\0';  // Ensure null termination
}


void setFlag() {
    operationDone = true;
}

void cycleParameters() {
    currentCR++;
    if (currentCR >= sizeof(codingRateOptions) / sizeof(codingRateOptions[0])) {
        currentCR = 0;
        currentSF++;
        if (currentSF >= sizeof(spreadingFactorOptions) / sizeof(spreadingFactorOptions[0])) {
            currentSF = 0;
            currentBW++;
            if (currentBW >= sizeof(bandwidthOptions) / sizeof(bandwidthOptions[0])) {
                currentBW = 0;
            }
            currentSW++;
            if (currentSW >= sizeof(syncWordOptions) / sizeof(syncWordOptions[0])) {
                currentSW = 0;
            }
        }
    }
    lastSettingChange = millis();
}

void initRadio() {
    float frequency = 906.875;
    float bandwidth = bandwidthOptions[currentBW];
    uint8_t spreadingFactor = spreadingFactorOptions[currentSF];
    uint8_t codingRate = codingRateOptions[currentCR];
    uint8_t syncWord = syncWordOptions[currentSW];   
    float outputPower = 0;
    uint16_t preambleLength = 8;

    Serial.print(F("freq:")); Serial.print(frequency);
    Serial.print(F(", BW:")); Serial.print(bandwidth);
    Serial.print(F(", SF:")); Serial.print(spreadingFactor);
    Serial.print(F(", CR:")); Serial.print(codingRate);
    Serial.print(F(", SW:0x")); Serial.print(syncWord, HEX);
    Serial.print(F(", preamble:")); Serial.println(preambleLength);

    int state = radio.begin(frequency, bandwidth, spreadingFactor, codingRate, syncWord, outputPower, preambleLength, 1.6, false);
    if (state == RADIOLIB_ERR_NONE) {
        radio.setDio1Action(setFlag);
        radio.setCRC(false);
        radio.startReceive();
    } else {
        Serial.print(F("[SX1262] Initialization failed, code "));
        Serial.println(state);
    }
}

void deriveKey(const char *channelName, const char *salt, uint8_t *outputKey) {
    mbedtls_md_context_t ctx;
    const mbedtls_md_info_t *mdInfo = mbedtls_md_info_from_type(MBEDTLS_MD_SHA256);
    mbedtls_md_init(&ctx);
    mbedtls_md_setup(&ctx, mdInfo, 1);
    mbedtls_md_hmac_starts(&ctx, (const uint8_t *)channelName, strlen(channelName));
    mbedtls_md_hmac_update(&ctx, (const uint8_t *)salt, strlen(salt));
    mbedtls_md_hmac_finish(&ctx, outputKey);
    mbedtls_md_free(&ctx);
}

void myAES256_CTR_Decrypt(const uint8_t *input, size_t inputLen,
                          const uint8_t *key, uint8_t *output,
                          uint8_t *nonce, uint32_t counter) {
    mbedtls_aes_context aes;
    mbedtls_aes_init(&aes);
    uint8_t iv[16] = {0};

    // Copy NONCE to the upper 96 bits of the IV
    memcpy(iv, nonce, 12);

    // Copy COUNTER to the lower 32 bits of the IV
    memcpy(iv + 12, &counter, sizeof(counter));

    size_t nc_off = 0;
    uint8_t stream_block[16] = {0};

    mbedtls_aes_setkey_enc(&aes, key, 256);
    mbedtls_aes_crypt_ctr(&aes, inputLen, &nc_off, iv, stream_block, input, output);
    mbedtls_aes_free(&aes);
}

void receivePacket() {
    String str;
    int state = radio.readData(str);
    if (state == RADIOLIB_ERR_NONE) {
        Serial.print(F("[SX1262] Data: "));
        Serial.println(str);




        uint64_t nodeNumber = 0x123456789ABC;      //   33 36 36 35 30 34 30 37 30 30    is 3665040700
        uint64_t packetNumber = 0x000001;

        uint8_t myDerivedKey[AES_KEY_SIZE];
        const char *myChannelName = "LONG_FAST";
        const char *mySalt = "Meshtastic";
        deriveKey(myChannelName, mySalt, myDerivedKey);

        uint8_t nonce[12];
        memcpy(nonce, &nodeNumber, 6);
        memcpy(nonce + 6, &packetNumber, 6);

        uint32_t counter = 0;

        uint8_t *decryptedData = (uint8_t *)malloc(str.length());
        if (decryptedData == NULL) {
            Serial.println(F("Memory allocation failed"));
            return;
        }
        
        myAES256_CTR_Decrypt((const uint8_t *)str.c_str(), str.length(), myDerivedKey, decryptedData, nonce, counter);
        
        Serial.print("Decrypted Data: ");
        for (size_t i = 0; i < str.length(); i++) {
            Serial.printf("%c", decryptedData[i]);
        }
        Serial.println();

        size_t myDataLength = min(sizeof(myBuffer), str.length());
        memcpy(myBuffer, decryptedData, myDataLength);

        if (myDataLength < sizeof(myBuffer)) {
            memset(myBuffer + myDataLength, 0, sizeof(myBuffer) - myDataLength);
        }

        free(decryptedData);

        myProtoMessage myRecoveredMessage;
        myProtoRecover(myRecoveredMessage);

        Serial.print("Recovered ID: ");
        Serial.println(myRecoveredMessage.myId, HEX);
        Serial.print("Recovered Text: ");
        Serial.println(myRecoveredMessage.myText);
    } else {
        Serial.print(F("[SX1262] ReadData failed, code "));
        Serial.println(state);
    }
}

void setup() {
    Serial.begin(115200);
    initRadio();
    lastSettingChange = millis();
}

void loop() {
    if (millis() - lastSettingChange >= settingDuration) {
        Serial.println();
        cycleParameters();
        initRadio();
    }
    if (operationDone) {
        operationDone = false;
        receivePacket();
    }
}
