// Copyright (C) 2017 BRK Brands, Inc. All Rights Reserved.
/// @file
/// This file simulates the process manager for sending all kinds of AWS messages
///
///
///
#include <stdlib.h>
#include <dirent.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>
#include "cia.h"
#include "http.h"
#include "fa_log.h"
#include "cJSON.h"

//#define LOCALHOST       "127.0.0.1"
#define LOCALHOST       "10.2.27.213"
#define LOCALPORT       8080

#define ONELINK_URI     "/onelinkCommand"


#define TEST_SSID       "FA-CC-TST"
#define TEST_PASS       "Tryme"

// Hard coded My Prime local key
// cc30c68de9b1496ba6e4f261fa61b512
const unsigned char key[16] = {
        0xcc,0x30,0xc6,0x8d,
        0xe9,0xb1,0x49,0x6b,
        0xa6,0xe4,0xf2,0x61,
        0xfa,0x61,0xb5,0x12
    };

bool makePost(const char *message)
{
    char *encrypted = encryptPayload(message, key);

    HttpStatus status;
    ResponseData responseData = (ResponseData){ .data = NULL, .size = 0 };
    bool success = false;

    if (encrypted) {
        status = httpPostRequest(LOCALHOST, LOCALPORT, ONELINK_URI,
                    NULL, &responseData, encrypted);
        if (status != HTTP_NO_CONTENT) {
            FA_ERROR("Post error");
        } else {

            success = true;
        }
    }
    free(encrypted);
    free(responseData.data);

    return success;
}

static bool setupWifi(void)
{
    cJSON *root = cJSON_CreateObject();
    cJSON *wifi = cJSON_CreateObject();
    cJSON_AddStringToObject(wifi, "ssid", TEST_SSID);
    cJSON_AddStringToObject(wifi, "passwd", TEST_PASS);
    cJSON_AddItemToObject(root, "wifi", wifi);
    char *message = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);

    bool status = makePost(message);
    free(message);
    return status;
}
static bool factoryReset(void)
{
    cJSON *root = cJSON_CreateObject();
    cJSON_AddBoolToObject(root, "reset", cJSON_True);
    char *message = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);

    bool status = makePost(message);
    free(message);
    return status;
}

static bool selfDiagose(void)
{
    cJSON *root = cJSON_CreateObject();
    cJSON_AddBoolToObject(root, "selfDiagosis", cJSON_True);
    char *message = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);

    bool status = makePost(message);
    free(message);
    return status;
}



int main(int argc, char *argv[])
{
    if (setupWifi()) {
        FA_NOTICE("setupWifi success");
    } else {
        FA_ERROR("setupWifi failed");
    }

    if (factoryReset()) {
        FA_NOTICE("factoryReset success");
    } else {
        FA_ERROR("factoryReset failed");
    }

    if (selfDiagose()) {
        FA_NOTICE("selfDiagose success");
    } else {
        FA_ERROR("selfDiagose failed");
    }

    return 0;
}
