#include <stdlib.h>
#include <dirent.h>
#include <unistd.h>
#include <fcntl.h>
#include <inttypes.h>
#include "prime_mq_cmds.h"
#include "message_queue.h"



enum {
    QUEUE_KEY_PAYLOAD = 16,
    QUEUE_STRING_PAYLOAD = 61
};

#pragma pack(push,1)
typedef struct {
    uint8_t     source;
    uint16_t    command;
    union {
        uint8_t     value_bool;
        uint8_t     value_u8;
        uint16_t    value_u16;
        uint8_t     key[QUEUE_KEY_PAYLOAD];
        char        str[QUEUE_STRING_PAYLOAD];
    } payload;
} IPC_MSG;

#pragma pack(pop)


static const char *getKeyString(IPC_COMMANDS cmd)
{
    switch (cmd) {
        case PRIME_MGR_CMD_PMESH_KEY:
            return "Pmesh_key";
        case PRIME_MGR_CMD_PMESH_ID:
            return "Pmesh_ID";
        case PRIME_MGR_CMD_ROOM:
            return "Room";
        case PRIME_MGR_CMD_NIGHTLIGHT:
            return "Night_light_switch";
        case PRIME_MGR_CMD_BRIGHTNESS:
            return "Brightness";
        case PRIME_MGR_CMD_SELF_DIAG_STATE:
            return "Self_diagnoise";
        case PRIME_MGR_CMD_FW_VERSION:
            return "Firmware_version";
        case PRIME_MGR_CMD_DOWNLOAD_DONE:
            return "Update_start";
        case PRIME_MGR_CMD_A2DP_ENABLE:
            return "A2DP_enabled";
        case PRIME_MGR_CMD_TEST_STATE:
            return "Test";
        case PRIME_MGR_CMD_SILENCE_STATE:
            return "Silence";
        case PRIME_MGR_CMD_SMOKE:
            return "Smoke_alarm";
        case PRIME_MGR_CMD_CO:
            return "CO_alarm";
        case PRIME_MGR_CMD_BATT_LEVEL:
            return "Battery_level";
        case PRIME_MGR_CMD_LOW_BATT:
            return "Low_battery";
        default:
            return "";
    }
}

typedef enum {
    VAL_U8,
    VAL_U16,
    VAL_BOOL,
    VAL_KEY,
    VAL_STR,
    VAL_BAD
} Payload_t;

static Payload_t getPayloadType(IPC_COMMANDS cmd)
{
    switch (cmd) {
        case PRIME_MGR_CMD_PMESH_KEY:
            return VAL_KEY;
        case PRIME_MGR_CMD_PMESH_ID:
            return VAL_U16;
        case PRIME_MGR_CMD_ROOM:
            return VAL_U8;
        case PRIME_MGR_CMD_NIGHTLIGHT:
            return VAL_BOOL;
        case PRIME_MGR_CMD_BRIGHTNESS:
            return VAL_U8;
        case PRIME_MGR_CMD_SELF_DIAG_STATE:
            return VAL_BOOL;
        case PRIME_MGR_CMD_FW_VERSION:
            return VAL_STR;
        case PRIME_MGR_CMD_DOWNLOAD_DONE:
            return VAL_BOOL;
        case PRIME_MGR_CMD_A2DP_ENABLE:
            return VAL_BOOL;
        case PRIME_MGR_CMD_TEST_STATE:
            return VAL_BOOL;
        case PRIME_MGR_CMD_SILENCE_STATE:
            return VAL_BOOL;
        case PRIME_MGR_CMD_SMOKE:
            return VAL_BOOL;
        case PRIME_MGR_CMD_CO:
            return VAL_BOOL;
        case PRIME_MGR_CMD_BATT_LEVEL:
            return VAL_U8;
        case PRIME_MGR_CMD_LOW_BATT:
            return VAL_BOOL;
        default:
            return VAL_BAD;
    }
}

char* bytesToString(uint8_t *bytes, size_t byteSize, char *str)
{
    char *ptr = str;
    for (size_t i = 0; i < byteSize; ++i)
    {
        ptr += sprintf(ptr, "%02x", (unsigned int)bytes[i]);
    }
    return str;
}
void display_message(Message *msg)
{
    IPC_MSG *ipc = (IPC_MSG *) msg->buffer;
    uint16_t cmd = ipc->command;

    printf("Event %s: ", getKeyString(cmd));
    Payload_t type = getPayloadType(cmd);

    switch (type) {
            case VAL_BOOL:
                printf("%s\n",ipc->payload.value_bool?"true":"false");
                break;
            case VAL_U8:
                printf("%" PRIu8 "\n", ipc->payload.value_u8);
                break;
            case VAL_U16:
                printf("%" PRIu16 "\n", ipc->payload.value_u16);
                break;
            case VAL_KEY: {
                char keystr[QUEUE_KEY_PAYLOAD*2+1] = {0};
                printf("%s\n",bytesToString(ipc->payload.key,16,keystr));
                break;
            }
            case VAL_STR:
                printf("%s\n",ipc->payload.str);
                break;
            case VAL_BAD:
            default:
                break;
        }

}
int main(int argc, char *argv[])
{
    int recv_qid = messageQueueGet(KEY_to_AWS);

    while (recv_qid < 0) {
        recv_qid = messageQueueGet(KEY_to_AWS);
        sleep(1);
    }
    printf("recv_qid is %d\n", recv_qid);



    while (1) {
        Message msg;
        memset(&msg, 0, sizeof(msg));
        int ret = messageQueueReceive_timeout(recv_qid, &msg, 3000000, 1);
        if (ret < 0){
            fprintf(stderr, "%s\n", "Error on receiving message");
            exit(EXIT_FAILURE);
        }
        if (ret == 0) {
            printf("%s\n", "Time out" );
        } else {
            printf("received %d bytes\n", ret);
            display_message(&msg);
        }
    }


    return 0;
}
