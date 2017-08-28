// Copyright (C) 2017 BRK Brands, Inc. All Rights Reserved.
/// @file
/// These utilities handle Confidentiality, Intergrity and Authorization (CIA)
/// of device and its communication protocol with Onelink Application.

#ifndef SRC_CRYPTO_H
#define SRC_CRYPTO_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

/**
 * @brief      Calculate the MD5 digest of input string
 *
 * @param[in]  inStr    Input string
 * @param[out] outStr   The digest string value
 *
 * @return     Pointer to the hash string
 */
char* md5String(char* inStr, char* outStr);


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
char* encryptPayload(const char *in, const uint8_t *key);

/**
 * @brief      Decode the base64 encoded string and decrypt the data and remove the padding
 *
 * @param[in]  payload  The input payload
 * @param[in]  key      The crypto key
 *
 * @return     Pointer to the buffer that stores the plain string of JSON content
 *             It is the caller's responsibility to free this buffer.
 */
char* decryptPayload(const char *payload, const uint8_t *key);
 

#endif // SRC_CRYPTO_H

