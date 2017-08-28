#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <signal.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <fcntl.h>
#include "base64.h"
#include <openssl/aes.h>
#include "fa_log.h"

static bool runnable = true;



int main(int argc, char *argv[])
{
    pid_t self = syscall(SYS_gettid);
    FA_NOTICE("PID %d",self);

    faLogInitialize(FA_LOG_LEVEL_DEBUG, FA_LOG_DEST_CONSOLE|FA_LOG_DEST_SYSLOG);
    faLogConfigureFile("test.c", FA_LOG_LEVEL_DEBUG, FA_LOG_DEST_CONSOLE|FA_LOG_DEST_SYSLOG);

    while (runnable) {
        FA_NOTICE("Notice message");
        FA_DEBUG("Debug message");
        sleep(1);
    }



    return 0;
}
