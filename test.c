#include <stdlib.h>
#include <dirent.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdint.h>

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

int main(int argc, char *argv[])
{
    int send_qid = messageQueueCreate(KEY_to_AWS);

    if (send_qid < 0) {
        printf("Can't create queue!\n");
        exit(EXIT_FAILURE);
    }

    for (int i = PRIME_MGR_CMD_SMOKE; i <= PRIME_MGR_CMD_SILENCE_STATE; i++)
    {
        Message msg;
        IPC_MSG ipc_msg;
        ipc_msg.source = PRIME_MQ_FROM_PM;
        ipc_msg.command = i;
        Payload_t type = getPayloadType(i);
        switch (type) {
            case VAL_BOOL:
                ipc_msg.payload.value_bool = 1;
                break;
            case VAL_U8:
                ipc_msg.payload.value_u8 = 0x64;
                break;
            case VAL_U16:
                ipc_msg.payload.value_u16 = 512;
                break;
            case VAL_KEY: {
                uint8_t key[QUEUE_KEY_PAYLOAD] =
                        {0,0x1,0x2,0x3,0x4,0x5,0x6,0x7,0x8,0x9,0xa,0xb,0xc,0xd,0xe,0xf};
                memcpy(ipc_msg.payload.key, key, QUEUE_KEY_PAYLOAD);
            }
                break;
            case VAL_STR:
                strncpy(ipc_msg.payload.str, "This is a long string", QUEUE_STRING_PAYLOAD);
                break;
            case VAL_BAD:
            default:
                break;
        }
        setMessage(&msg, (char *)&ipc_msg, sizeof(IPC_MSG), 1);
        messageQueueSend(send_qid, &msg);
        printf("sent %s\n", getKeyString(i));
        sleep(1);
    }


    messageQueueDelete(send_qid);
    return 0;
}
