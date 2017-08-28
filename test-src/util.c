// Copyright (C) 2017 BRK Brands, Inc. All Rights Reserved.
/// @file

#include "fa_log.h"
#include "util.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>



/**
 * @brief      Gets the random bytes from high quality random number generator
 *
 * @param      key      The byte array to save the numbers
 * @param[in]  keysize  The array size
 *
 * @return     True if succes or otherwise
 */
bool getRandomBytes(uint8_t *key, size_t keysize)
{
    bool success = false;
    int fd = open("/dev/urandom", O_RDONLY);
    if (fd > 0)
    {
        ssize_t n = read(fd, key, keysize);
        if (n > 0)
        {
            success = true;
        }
        else
        {
            //FA_ERROR("Failed to read /dev/urandom");
        }
        close(fd);
    }
    return success;
}

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
char* bytesHexlify(char *str, uint8_t *bytes, size_t byteSize)
{
    char *ptr = str;
    for (size_t i = 0; i < byteSize; ++i)
    {
        ptr += sprintf(ptr, "%02x", (unsigned int)bytes[i]);
    }
    return str;
}

/**
 * @brief      Convert a hexdecimal string to its byte form
 *
 * @param[in]  hexStr   The hexadecimal string
 * @param[out] hexByte  The hexadecimal byte
 * @param[in]  byteLen  The byte array length
 *
 * @return     0 if success and -1 if failed.
 */
int bytesUnHexlify(uint8_t *hexByte, const char *hexStr, size_t byteLen)
{
    int status = 0;

    if (hexStr && hexByte)
    {
        const char *pos = hexStr;

        for(size_t count = 0; count < byteLen; count++) {
            int n = sscanf(pos, "%2hhx", &hexByte[count]);
            if (n == 1) {
                pos += 2 * sizeof(char);
            } else {
                status = -1;
                break; // No matching characters
            }
        }
    }
    else
    {
        status = -1;
    }
    return status;
}

/**
 * @brief      Convert bytes array to its ASCII string presentation.
 *
 * @param      bytes     The bytes
 * @param[in]  byteSize  The byte size
 * @param      str       The output string
 *
 * @return     true if all bytes are printable characters, false if otherwise
 */
bool bytesToChar(char *str, uint8_t *bytes, size_t byteSize)
{
    char *ptr = str;
    for (size_t i = 0; i < byteSize; ++i) {
        if (bytes[i] < 32 || bytes[i] > 126) {
            return false;
        }
        ptr += sprintf(ptr, "%c", bytes[i]);
    }
    return true;
}


/**
 * @brief      Generate a version 4 UUID string
 *
 * @param      str the char array for storing UUID string
 * @param      key the byte array of UUID
 *
 * @return     Pointer to the UUID string
 */
char* uuid4String(char *str, uint8_t *key)
{
    bytesHexlify(str, key, KEY_BYTE_LEN);
    return str;
}

/**
 * @brief      Fill teh input byte array with UUID4 format
 *
 * @param      key the byte array for storing UUID
 *
 * @return     pointer to the uuid array
 */
uint8_t *uuid4Key(uint8_t *key)
{
    if (getRandomBytes(key, UUID_BYTE_LEN))
    {
        // Set UUID version to 4 --- truly random generation
        key[6] = (key[6] & 0x0F) | 0x40;
        // Set the UUID variant to DCE
        key[8] = (key[8] & 0x3F) | 0x80;

        return key;
    }
    return NULL;
}

/**
 * @brief      Check input is in a valid UUID4 string format as
 *             xxxxxxxx-xxxx-4xxx-Nxxx-xxxxxxxxxxxx
 *
 * @param[in]  id    The input string
 *
 * @return     true if the string has valid UUID4 format or false if otherwise
 */
bool validUUID4(const char *id)
{
    if (strlen(id) != (UUID_STRING_LEN - 1)) {
        return false;
    }
    if (id[12] != '4') {
        return false;
    }
    if (id[16]<'8' || (id[16]>'9' && id[16]<'a') || id[16]>'b') {
        return false;
    }
    return true;
}

 