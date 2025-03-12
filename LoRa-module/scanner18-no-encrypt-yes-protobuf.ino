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
const unsigned long settingDuration = 61000;   // 4000;      //  to cycle through but try a large number just to stay on one set of settings such as  100000;  //

// LoRa parameter sets for testing
float bandwidthOptions[] = {250.0};   //500.0, 250.0, 125.0    // bests 250.0
uint8_t spreadingFactorOptions[] = {11};   //7, 8, 9, 10, 11, 12  // best 11   // very important
uint8_t codingRateOptions[] = {5, 6, 7, 8};    //5, 6, 7, 8    // best 7  // best 5   // not as important
uint8_t syncWordOptions[] = {0x2B};    //0x00, 0x12, 0x34, 0x2B, 0x2B84    // best 0x2B

int currentBW = 0;
int currentSF = 0;
int currentCR = 0;
int currentSW = 0;


// set a buffer
uint8_t myBuffer[104];  // Sufficient for ID and text   +4 bigger than the text
size_t bufferSize = 0;



struct myProtoMessage {
    int32_t myId;
    char myText[100];  // Ensure enough space
};


myProtoMessage myMessage;



// Function to encode a message into a buffer
void myProtoStore(myProtoMessage &message, uint8_t *buffer, size_t &bufferSize) {
    uint8_t *ptr = buffer;

    // Encode ID as Protobuf Varint
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

    // Calculate total buffer size used
    bufferSize = ptr - buffer + textLength;
}

// Function to decode a message from a buffer
void myProtoRecover(myProtoMessage &message, uint8_t *buffer) {
    uint8_t *ptr = buffer;

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
                currentSW++;
                if (currentSW >= sizeof(syncWordOptions) / sizeof(syncWordOptions[0])) {
                  currentSW = 0;
                }
          }
        }
    }
    lastSettingChange = millis();
}

void initRadio() {
  // Note: This was set in the Mestastic LoRa setting as fixed.   
    float frequency =   906.875;  //  906.875;  //best  906.875;    //902.125;  //   906.875;  // 915;   // MUST BE SET IN RADIO CONFIGURATION, LORA --> OVERRIDE FREQUENCY 915.0
    float bandwidth = bandwidthOptions[currentBW];
    uint8_t spreadingFactor = spreadingFactorOptions[currentSF];
    uint8_t codingRate = codingRateOptions[currentCR];
    uint8_t syncWord = syncWordOptions[currentSW];   
    float outputPower = 0;
    uint16_t preambleLength = 8;  //  //8;  //16;  //32;    // best 8

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
        Serial.print(F("[SX1262] Raw Data: "));
        Serial.println(str);


        // Clear structure before recovery
        memset(&myMessage, 0, sizeof(myMessage));


        memcpy(myBuffer, str.c_str(), strlen(str.c_str()) + 1);
        // Recover
        myProtoRecover(myMessage, myBuffer);

        Serial.print("Recovered ID: i");
        Serial.println(myMessage.myId, HEX);
        Serial.print("Recovered Text: ");
        Serial.println(myMessage.myText);




    } else {
        Serial.print(F("[SX1262] ReadData failed, code "));
        Serial.println(state);
    }
}

void setup() {
    Serial.begin(115200);
    initRadio();
    lastSettingChange = millis();
    

    myMessage.myId = 0xAA11BB22;  // Example ID
    strcpy(myMessage.myText, "Hello ProtoBuf");

    Serial.println("Original Message:");
    Serial.printf("ID: %X\n", myMessage.myId);
    Serial.printf("Text: %s\n", myMessage.myText);


    // Store
    myProtoStore(myMessage, myBuffer, bufferSize);

    // Clear structure before recovery
    memset(&myMessage, 0, sizeof(myMessage));

    // Recover
    myProtoRecover(myMessage, myBuffer);

    Serial.println("\nRecovered Message:");
    Serial.printf("ID: %X\n", myMessage.myId);
    Serial.printf("Text: %s\n", myMessage.myText);
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
