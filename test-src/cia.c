// Copyright (C) 2017 BRK Brands, Inc. All Rights Reserved.
/// @file
/// These utilities handle Confidentiality, Intergrity and Authorization (CIA)
/// of device.

#include "util.h"
#include "base64.h"
#include "cia.h"
#include "fa_log.h"
#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <openssl/md5.h>
#include <openssl/aes.h>
/// DEFINES

#define RND_PADDING    4
#define MSG_LEN_BYTE   4

#define FA_USE_DEFAULT_KEY      "FA_USE_DEFAULT_KEY"
#define FA_LOCAL_KEY            "FA_LOCAL_KEY"
#define FA_CLOUD_KEY            "FA_CLOUD_KEY"

 

/// Space to hold an unsigned big-endian 32 bit integer.
typedef uint8_t BigEndianUInt32[4];

static void writeBEUInt32(BigEndianUInt32 beData, uint32_t value)
{
    beData[0] = 0xff & (value >> 24);
    beData[1] = 0xff & (value >> 16);
    beData[2] = 0xff & (value >> 8);
    beData[3] = 0xff & value;
}

// Read a 32 bit big-endian unsigned number from the array of bytes.
static uint32_t readBEUInt32(const BigEndianUInt32 beData)
{
    return (((uint32_t)beData[0] << 24) |
            ((uint32_t)beData[1] << 16) |
            ((uint32_t)beData[2] << 8) |
            (beData[3]));
}

/**
 * @brief      Calculate the MD5 digest of input string
 *
 * @param[in]  inStr    Input string
 * @param[out] outStr   The digest string value
 *
 * @return     Pointer to the hash string.
 */
char* md5String(char* inStr, char* outStr)
{
    MD5_CTX context;
    size_t strSize = strlen(inStr);
    MD5_Init(&context);
    MD5_Update(&context, inStr, strSize);
    uint8_t digest[MD5_BYTE_LEN] = {0};
    MD5_Final(digest, &context);

    bytesHexlify(outStr, digest, sizeof(digest));
    return outStr;
}

/**
 * @brief      Encrypt the input text with AES 128bit CBC algorithm using
 *             the user's private key and specified initializaation vector.
 *             The output of encryption has the IV as header and the data.
 *             The caller needs to free the output buffer after use.
 *
 * @param[in]  inputData  The input plain data (ASCII)
 * @param[in]  paddedSize   The size of input in bytes
 * @param[in]  userKey    The user key
 * @param[in]  iv         The initializaation vector always has the size of
 *                        AES_BLOCK_SIZE
 *
 * @return     Pointer to the buffer of encryption output
 */

static bool aes128_encrypt(uint8_t *cryptedText, const uint8_t *clearText,
                    const size_t paddedSize,const uint8_t* key, uint8_t *iv)
{
    // IV will be altered by AES and we want to keep the original
    uint8_t ivCopy[AES_BLOCK_SIZE];
    memcpy(ivCopy, iv, AES_BLOCK_SIZE);

    AES_KEY aes;
    AES_set_encrypt_key(key, 128, &aes);
    AES_cbc_encrypt(clearText, cryptedText, paddedSize, &aes, ivCopy, AES_ENCRYPT);
    return true;
}

/**
 * @brief      Decrypt the input text with private key and the IV embedded in text
 *
 * @param      clearText  The buffer to store the decrypted text
 * @param[in]  cryptText  pointer to the encrypted text without IV
 * @param[in]  len        The length of encrypted text
 * @param[in]  crytoKey   The user's cryto key
 * @param      iv         Initialization vector which is stripped by stripIvFromCryptText
 *
 * @return     true if success or otherwise
 */
static bool aes128_decrypt(uint8_t *clearText, const uint8_t *cryptText, size_t len,
                     const uint8_t *key, uint8_t *iv)
{
    AES_KEY aes;

    AES_set_decrypt_key(key, 128, &aes);
    AES_cbc_encrypt(cryptText,clearText, len, &aes, iv, AES_DECRYPT);

    return true;
}

/**
 * @brief      Strip the initialization vector from the encrypted text.
 *             In our AES CBC scheme the IV is added to the beginning of payload for
 *             the recipient to decrypt and its size should be 16 bytes.
 *
 * @param      cryptText  The encrypted text
 * @param[in]  len        The length of payload including the IV
 * @param      iv         the byte array stores the IV
 */
static void stripIvFromCryptText(uint8_t *cryptText, size_t len, uint8_t *iv)
{
    for (size_t i = 0; i < AES_BLOCK_SIZE; i++) {
        iv[i] = cryptText[i];
    }
    // move cryptText AES_BLOCK_SIZE bytes up
    for (size_t i = 0; i < (len - AES_BLOCK_SIZE); i++) {
        cryptText[i] = cryptText[i + AES_BLOCK_SIZE];
    }
}

/**
 * @brief      Remove the padding from the decrypted message and store the payload
 *             to a new buffer with nul terminated
 *
 * @param[in]  paddedData  The padded data after AES decryption
 * @param      msg         The buffer to store the payload
 *
 * @return     The size embedded in the data for the payload message
 */
static char *unpadData(const uint8_t *paddedData, size_t *msg_len)
{
    // Verify random bytes are filled as expected
    if ((paddedData[0] != paddedData[2]) || (paddedData[1] != paddedData[3])) {
        return NULL;
    }

    *msg_len = readBEUInt32(&paddedData[4]);
    // Save a C string wiht nul terminated
    char *msg = calloc(*msg_len+1, sizeof(char));
    if (msg) {
        memcpy(msg, &paddedData[8], *msg_len);
    }
    return msg;
}

/**
 * @brief      Encrypt the input data with AES128 after proper padding and generate
 *             the base64-encoded string
 *
 * @param[in]  in  The string form of JSON contents
 * @param[in]  key      The crypto key
 *
 * @return     Pointer to the buffer that stores the base64-encoded string or NULL if failed
 *             It is the caller's responsibility to free this buffer.
 */
char* encryptPayload(const char *in, const uint8_t *key)
{
    if (in == NULL || strlen(in) == 0) {
        return NULL;
    }

    size_t inSize = strlen(in);
    size_t paddedSize = (inSize + RND_PADDING + MSG_LEN_BYTE + AES_BLOCK_SIZE - 1) & \
                 ~(size_t)(AES_BLOCK_SIZE-1);

    uint8_t iv[AES_BLOCK_SIZE];
    getRandomBytes(iv, AES_BLOCK_SIZE);

    char *b64Cypher = NULL;

    uint8_t *paddedBuffer = calloc(paddedSize, sizeof(uint8_t));
    uint8_t *cryptOut     = calloc(paddedSize, sizeof(uint8_t));
    uint8_t *out          = calloc(AES_BLOCK_SIZE + paddedSize, sizeof(uint8_t));

    if (paddedBuffer && cryptOut && out) {
        getRandomBytes(paddedBuffer, paddedSize);
        paddedBuffer[0] = paddedBuffer[2];    // make a pattern for validation
        paddedBuffer[1] = paddedBuffer[3];
        writeBEUInt32(&paddedBuffer[4], (uint32_t)inSize);
        memcpy(&paddedBuffer[8], in, inSize);

        if (aes128_encrypt(cryptOut, paddedBuffer, paddedSize, key, iv)) {
            memcpy(out, iv, AES_BLOCK_SIZE);
            memcpy(out + AES_BLOCK_SIZE, cryptOut, paddedSize);
            b64Cypher = (char*)base64_encode(out, AES_BLOCK_SIZE + paddedSize, NULL);
        }
        else {
            FA_ERROR("AWS: Failed to encrypt message!!!");
        }

    }
    else {
        FA_ERROR("AWS: Failed to allocate memory");
    }
    free(paddedBuffer);
    free(cryptOut);
    free(out);

    return b64Cypher;
}

/**
 * @brief      Decode the base64 encoded string and decrypt the data and remove the padding
 *
 * @param[in]  payload  The input payload
 * @param[in]  key      The crypto key
 *
 * @return     Pointer to the buffer that stores the plain string of JSON content
 *             It is the caller's responsibility to free this buffer.
 */
char* decryptPayload(const char *payload, const uint8_t *key)
{
    size_t b64OutSize;
    unsigned char *b64decoded = base64_decode((const unsigned char*)payload,
                                        strlen(payload), &b64OutSize);

    if (b64decoded == NULL) {
        return NULL;
    }

    uint8_t iv[AES_BLOCK_SIZE];
    stripIvFromCryptText(b64decoded, b64OutSize, iv);

    uint8_t *clearText = calloc(b64OutSize-AES_BLOCK_SIZE, sizeof(uint8_t));
    if (clearText == NULL) {
        free(b64decoded);
        return NULL;
    }

    char *msg = NULL;
    if (!aes128_decrypt(clearText, b64decoded, b64OutSize-AES_BLOCK_SIZE, key, iv)) {
        FA_ERROR("AWS: AES decryption failed");
    } else {
        size_t msg_len;
        msg = unpadData(clearText, &msg_len);
        if (msg) {
            FA_NOTICE("AWS: Unpadded msg[%zu bytes]: %s", msg_len, msg);
        }
        else {
            FA_ERROR("AWS: Failed to unpad data");
        }
    }
    free(clearText);
    free(b64decoded);
    return msg;
}

 