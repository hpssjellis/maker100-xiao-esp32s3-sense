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
#include <RadioLib.h>

#define LORA_MISO 8   // confirmed
#define LORA_SCK 7    // confirmed
#define LORA_MOSI 9  // confirmed
#define LORA_CS 41    // NSS
#define LORA_DIO2 38
#define LORA_DIO1 39     // irq
#define LORA_RESET 42
#define LORA_BUSY 40

#include <modules/SX126x/patches/SX126x_patch_scan.h>

SX1262 radio = new Module(LORA_CS, LORA_DIO1, LORA_RESET, LORA_BUSY);

volatile bool operationDone = false;
unsigned long lastSettingChange = 0;
const unsigned long settingDuration = 4000;

// LoRa parameter sets for testing
float bandwidthOptions[] = {125.0};   //500.0, 250.0, 125.0
uint8_t spreadingFactorOptions[] = {9};   //7, 8, 9, 10, 11, 12
uint8_t codingRateOptions[] = {6, 7};    //5, 6, 7, 8
uint8_t syncWordOptions[] = {0x12};    //0x00, 0x12, 0x34, 0x2B

int currentBW = 0;
int currentSF = 0;
int currentCR = 0;
int currentSW = 0;

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
  // Note: This was set in the Mestastic LoRa setting as fixed.   
    float frequency =  915;   //   906.875;  // 915;   // MUST BE SET IN RADIO CONFIGURATION, LORA --> OVERRIDE FREQUENCY 915.0
    float bandwidth = bandwidthOptions[currentBW];
    uint8_t spreadingFactor = spreadingFactorOptions[currentSF];
    uint8_t codingRate = codingRateOptions[currentCR];
    uint8_t syncWord = syncWordOptions[currentSW];   
    float outputPower = 14;
    uint16_t preambleLength = 16;  //8;  //8;  //16;  //32;

    Serial.print(F("freq:")); Serial.print(frequency);
    Serial.print(F(", BW:")); Serial.print(bandwidth);
    Serial.print(F(", SF:")); Serial.print(spreadingFactor);
    Serial.print(F(", CR:")); Serial.print(codingRate);
    Serial.print(F(", SW:0x")); Serial.print(syncWord, HEX);  //Serial.println(syncWord);
    Serial.print(F(", preamble:")); Serial.println(preambleLength);

    int state = radio.begin(frequency, bandwidth, spreadingFactor, codingRate, syncWord, outputPower, preambleLength, 1.6, false);
    if (state == RADIOLIB_ERR_NONE) {
       // Serial.println(F("[SX1262] Initialization successful!"));
        radio.setDio1Action(setFlag);

        radio.setCRC(false);    // maybe remove this later
       // radio.setCRC(true);    
        radio.startReceive();
    } else {
        Serial.print(F("[SX1262] Initialization failed, code "));
        Serial.println(state);
    }
}

void receivePacket() {
    String str;
    int state = radio.readData(str);
    if (state == RADIOLIB_ERR_NONE) {
       // Serial.println(F("[SX1262] Received packet!"));
        Serial.print(F("[SX1262] Data: "));
        Serial.println(str);
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
