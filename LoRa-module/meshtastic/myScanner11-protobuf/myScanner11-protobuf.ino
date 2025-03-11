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
#include "encrypted_message.pb.h"  // Include the generated protobuf header
#include "pb_decode.h"

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

SX1262 radio = SX1262(new Module(LORA_CS, LORA_DIO1, LORA_RESET, LORA_BUSY));

volatile bool operationDone = false;
unsigned long lastSettingChange = 0;
const unsigned long settingDuration = 61000;

float bandwidthOptions[] = {250.0};   
uint8_t spreadingFactorOptions[] = {11};   
uint8_t codingRateOptions[] = {5, 6, 7, 8};
uint8_t syncWordOptions[] = {0x2B};

int currentBW = 0;
int currentSF = 0;
int currentCR = 0;
int currentSW = 0;

void setFlag() {
    operationDone = true;
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
    memcpy(iv, nonce, 12);
    memcpy(iv + 12, &counter, sizeof(counter));

    size_t nc_off = 0;
    uint8_t stream_block[16] = {0};

    mbedtls_aes_setkey_enc(&aes, key, 256);
    mbedtls_aes_crypt_ctr(&aes, inputLen, &nc_off, iv, stream_block, input, output);
    mbedtls_aes_free(&aes);
}

void printProtobufMessage(const uint8_t *data, size_t length) {
    EncryptedMessage message = EncryptedMessage_init_zero;
    pb_istream_t stream = pb_istream_from_buffer(data, length);

    if (!pb_decode(&stream, EncryptedMessage_fields, &message)) {
        Serial.println("Failed to decode protobuf message.");
        return;
    }

    Serial.print("Decoded Message ID: ");
    Serial.println(message.id);
    Serial.print("Decoded Text: ");
    Serial.println(message.text);
}

void receivePacket() {
    String str;
    int state = radio.readData(str);
    if (state == RADIOLIB_ERR_NONE) {
        Serial.print(F("[SX1262] Received Encrypted Data: "));
        Serial.println(str);

        uint64_t nodeNumber = 0x123456789ABC;
        uint64_t packetNumber = 0x000001;

        uint8_t myDerivedKey[AES_KEY_SIZE];
        deriveKey("LONG_FAST", "Meshtastic", myDerivedKey);

        uint8_t nonce[12];
        memcpy(nonce, &nodeNumber, 6);
        memcpy(nonce + 6, &packetNumber, 6);

        uint32_t counter = 0;
        uint8_t decryptedData[str.length()];
        myAES256_CTR_Decrypt((const uint8_t *)str.c_str(), str.length(), myDerivedKey, decryptedData, nonce, counter);

        Serial.print("Decrypted Data: ");
        for (size_t i = 0; i < str.length(); i++) {
            Serial.printf("%c", decryptedData[i]);
        }
        Serial.println();

        // Decode and print protobuf message
        printProtobufMessage(decryptedData, str.length());
    } else {
        Serial.print(F("[SX1262] ReadData failed, code "));
        Serial.println(state);
    }
}

void initRadio() {
    // Initialize the radio settings here
}

void cycleParameters() {
    // Cycle through the parameter options here
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
