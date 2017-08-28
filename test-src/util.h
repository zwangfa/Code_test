// Copyright (C) 2017 BRK Brands, Inc. All Rights Reserved.
/// @file


#ifndef PRIME_UTIL_H
#define PRIME_UTIL_H

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

#define KEY_BYTE_LEN            16
#define KEY_STRING_LEN          (KEY_BYTE_LEN*2+1)
#define UUID_BYTE_LEN           KEY_BYTE_LEN
#define UUID_STRING_LEN         (UUID_BYTE_LEN*2+1)
#define MD5_BYTE_LEN            KEY_BYTE_LEN
#define MD5_STRING_LEN          (MD5_BYTE_LEN*2+1)



/**
 * @brief      Gets the random bytes from high quality random number generator
 *
 * @param      key      The byte array to save the numbers
 * @param[in]  keysize  The array size
 *
 * @return     True if succes or otherwise
 */
bool getRandomBytes(uint8_t *key, size_t keysize);

/**
 * @brief      Convert bytes array to a hexdecimal string. Caller needs to make
 *             sure the string size is 2*bytesSize+1.
 *
 * @param      bytes     The bytes
 * @param[in]  byteSize  The byte size
 * @param      str       The output string
 *
 * @return     the pointer to the output string for chain use
 */
char* bytesHexlify(char *str, uint8_t *bytes, size_t byteSize);

/**
 * @brief      Convert a hexdecimal string to its byte form
 *
 * @param[in]  hexStr   The hexadecimal string
 * @param[out] hexByte  The hexadecimal byte
 * @param[in]  byteLen  The byte array length
 *
 * @return     0 if success and -1 if failed.
 */
int bytesUnHexlify(uint8_t *hexByte, const char *hexStr, size_t byteLen);

/**
 * @brief      Convert bytes array to its ASCII string presentation.
 *
 * @param      bytes     The bytes
 * @param[in]  byteSize  The byte size
 * @param      str       The output string
 *
 * @return     true if all bytes are printable characters, false if otherwise
 */
bool bytesToChar(char *str, uint8_t *bytes, size_t byteSize);

/**
 * @brief      Generate a version 4 UUID string
 *
 * @param      str the char array for storing UUID string
 * @param      key the byte array of UUID
 *
 * @return     Pointer to the UUID string
 */
char* uuid4String(char *str, uint8_t *key);

/**
 * @brief      Fill teh input byte array with UUID4 format
 *
 * @param      key the byte array for storing UUID
 *
 * @return     pointer to the uuid array
 */
uint8_t *uuid4Key(uint8_t *key);


/**
 * @brief      Check input is in a valid UUID4 string format as
 *             xxxxxxxx-xxxx-4xxx-Nxxx-xxxxxxxxxxxx
 *
 * @param[in]  id    The input string
 *
 * @return     true if the string has valid UUID4 format or false if otherwise
 */
bool validUUID4(const char *id);


 


#endif
