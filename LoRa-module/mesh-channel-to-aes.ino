
#include "mbedtls/md.h"

#define AES_KEY_SIZE 32  // 256-bit key (Meshtastic standard)
#define MY_ITERATIONS 100000  // Meshtastic's default iteration count

// PBKDF2-HMAC-SHA256 implementation
void myPBKDF2_HMAC_SHA256(const uint8_t *myPassword, size_t myPasswordLen,
                          const uint8_t *mySalt, size_t mySaltLen,
                          int myIterations, uint8_t *myOutput, size_t myOutputLen) {
    mbedtls_md_context_t myCtx;
    const mbedtls_md_info_t *myMdInfo = mbedtls_md_info_from_type(MBEDTLS_MD_SHA256);

    if (myMdInfo == NULL) {
        Serial.println("Error: SHA256 not available");
        return;
    }

    mbedtls_md_init(&myCtx);
    mbedtls_md_setup(&myCtx, myMdInfo, 1);

    uint8_t myU[AES_KEY_SIZE];
    uint8_t myT[AES_KEY_SIZE];

    for (size_t i = 0; i < myOutputLen / AES_KEY_SIZE; i++) {
        memset(myT, 0, AES_KEY_SIZE);
        memcpy(myU, mySalt, mySaltLen);
        myU[mySaltLen] = (i + 1) >> 24;
        myU[mySaltLen + 1] = (i + 1) >> 16;
        myU[mySaltLen + 2] = (i + 1) >> 8;
        myU[mySaltLen + 3] = (i + 1);

        for (int j = 0; j < myIterations; j++) {
            mbedtls_md_hmac_starts(&myCtx, myPassword, myPasswordLen);
            mbedtls_md_hmac_update(&myCtx, myU, mySaltLen + 4);
            mbedtls_md_hmac_finish(&myCtx, myU);

            for (int k = 0; k < AES_KEY_SIZE; k++) {
                myT[k] ^= myU[k];
            }
        }
        memcpy(myOutput + i * AES_KEY_SIZE, myT, AES_KEY_SIZE);
    }

    mbedtls_md_free(&myCtx);
}

void setup() {
    Serial.begin(115200);
    delay(6000); // Allow Serial monitor time to start

    const char *myChannelName = "LongFast";  // Replace with your channel name
    const char *mySalt = "Meshtastic";  // Meshtastic's default salt
    uint8_t myDerivedKey[AES_KEY_SIZE];

    myPBKDF2_HMAC_SHA256((const uint8_t *)myChannelName, strlen(myChannelName),
                         (const uint8_t *)mySalt, strlen(mySalt),
                         MY_ITERATIONS, myDerivedKey, AES_KEY_SIZE);

    Serial.print("Derived Encryption Key: ");
    for (int i = 0; i < AES_KEY_SIZE; i++) {
        Serial.printf("0x%02x, ", myDerivedKey[i]);
    }
    Serial.println();
}

void loop() {
    // No need for anything in loop
}
