// Copyright (C) 2017 BRK Brands, Inc. All Rights Reserved.

///@file
/// For our log system, I would like to do a few things:
/// \li Work correctly in a multi-threaded environment.
/// \li send the log to multiple possible destinations (stdout, syslog, circular buffer for app access?)
/// \li allow custom control of log output (by file, severity) that is controlled by a config file.

#include <syslog.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <pthread.h>

#include "config.h"
#include "fa_log.h"

// Here are some defines you may wish to set in the project's config.h
//#define FEATURE_LOG_TO_STDOUT 1 ///< The target supports printing to stdout.
//#define FEATURE_LOG_TO_SYSLOG 1 ///< The target supports logging to syslog.
//#define FEATURE_LOG_TO_CBUF   1 ///< The target supports logging to a circular buffer.

#if defined(FEATURE_LOG_TO_SYSLOG)
#if !defined(FEATURE_LOG_TO_SYSLOG_NAME)
/// Set the name to use when logging via syslog.
#define FEATURE_LOG_TO_SYSLOG_NAME "onelink"
#endif
#endif


static const char *logLevelString(FaLogLevel level)
{
    switch (level) {
        case FA_LOG_LEVEL_CRITICAL:
            return "CRITIC";
        case FA_LOG_LEVEL_ERROR:
            return "ERROR";
        case FA_LOG_LEVEL_WARNING:
            return "WARN";
        case FA_LOG_LEVEL_NOTICE:
            return "NOTIC";
        case FA_LOG_LEVEL_INFO:
            return "INFO";
        case FA_LOG_LEVEL_DEBUG:
        case FA_LOG_NUM_LEVELS:
            return "DEBUG";
    }
    return "";
}
// get the destination set for a given source file and severity level.
static FaLogDestinationSet getDestinations(const char *filename, FaLogLevel severity);

#if defined(FEATURE_LOG_TO_SYSLOG)
/// Initialize syslog so our messages to it look pretty. The main values set
/// here are the name to use in syslog and the default facility to use.
static void faSyslogInitialize(void)
{
    openlog(FEATURE_LOG_TO_SYSLOG_NAME, LOG_CONS, LOG_USER);
}

/// Send a message to syslog for logging.
static void faSyslogLog(FaLogLevel severity, const char *msg)
{
    syslog(severity, "%s", msg);
}
#endif

/// Calls \ref faLogInitialize with some reasonable default values. This is only
/// used if a logging function other than \ref faLogInitialize is called first.
static void faLogDoDefaultInitialization(void)
{
    faLogInitialize(FA_LOG_LEVEL_DEBUG, 0xff);
}

/// Set to true when \ref faLogInitialize is called. \ref faLogInitialize must
/// be called before the rest of the log system will work. This flag allows us
/// to make sure the system is initialized before continuing.
static bool g_LogIsInitialized = false;

// This is our main logging entrypoint.
void faLog(const char *fname, uint32_t line, FaLogLevel severity, const char *format, ...)
{
    if (!g_LogIsInitialized) {
#if defined(OS_LINUX)
        static pthread_once_t fa_log_initialized = PTHREAD_ONCE_INIT;
        pthread_once(&fa_log_initialized, faLogDoDefaultInitialization);
#else
        faLogDoDefaultInitialization();
#endif
    }


    char buffer[1024];
    int pre_len = snprintf(buffer, sizeof(buffer), "%s:%d: [%s] ", fname, line, logLevelString(severity));
    if (pre_len < 0) {
        return; // Some random problem. We are in a world of hurt if this fails.
    }
    va_list args;
    va_start(args, format);
    int main_len = vsnprintf(&buffer[pre_len], sizeof(buffer) - (unsigned)pre_len, format, args);
    va_end(args);
    if (main_len < 0) {
        // Something bad and very unexpected happened, use what we previously put into the buffer.
        buffer[pre_len] = '\0';
    }

    FaLogDestinationSet destinations = getDestinations(fname, severity);

    if (destinations & FA_LOG_DEST_CONSOLE) {
#if defined(FEATURE_LOG_TO_STDOUT)
        puts(buffer);
        fflush(stdout);
//#else some test for a debug UART
//   and code to send the buffer to the debug UART...
#endif
    }

#if defined(FEATURE_LOG_TO_SYSLOG)
    if (destinations & FA_LOG_DEST_SYSLOG) {
        faSyslogLog(severity, buffer);
    }
#endif
}

/// List of destination sets by severity level.
typedef FaLogDestinationSet DestsByLevel[FA_LOG_NUM_LEVELS];

/// The default destination(s) for each log severity level.
static DestsByLevel defaultLogDestinations;

/// Mapping from a filename to the log destinations. This allows us to override
/// the default settings on a file by file basis.
typedef struct FileLogInfo {
    const char *filename;     ///< Base name of source file. E.g. "onelink.c"
    DestsByLevel destinations;///< Destination info to apply for the source file.
} FileLogInfo;

/// Home many different files can override the default settings.
#define OVERRIDE_MAX  20
/// Array of information for files that use non-default logging values.
static FileLogInfo overrides[OVERRIDE_MAX];
/// How many overrides have been recorded.
static int overrideCount;
/// Define the size of the space used to store filenames for the override info.
#define nameBufferSize ((size_t)512)
/// Reserve space to store the override file names.
static char nameBuffer[nameBufferSize];
/// How much of \ref nameBuffer has been used so far.
static size_t nameBufferUsed;

/// Return the logging destinations given a source file and severity level. The
/// destination set will either be the default for the given severity, or a
/// value from the \ref overrides array.
/// @return Set of logging destinations to use.
static FaLogDestinationSet getDestinations(const char *filename, FaLogLevel severity)
{
    assert(severity < FA_LOG_NUM_LEVELS);
    FaLogDestinationSet destinations = defaultLogDestinations[severity];
    if (filename) {
        for (int i=0; i<overrideCount; ++i) {
            if (strcmp(filename, overrides[i].filename) == 0) {
                FaLogDestinationSet tmp = overrides[i].destinations[severity];
                if (tmp != FA_LOG_DEST_DEFAULT) {
                    destinations = tmp;
                }
                break;
            }
        }
    }
    return destinations;
}

/// Make sure the override information is cleared and ready for overrides to be
/// recorded.
static void initializeFileOverride(void)
{
    overrideCount = 0;
    memset(overrides, 0, sizeof(overrides));
    nameBufferUsed = 0;
    memset(nameBuffer, 0, sizeof(nameBuffer));
}

// Initialize the logging system and set the default destination info.
void faLogInitialize(FaLogLevel minimumSeverity, FaLogDestinationSet destinations)
{

    initializeFileOverride();
    for (int i=0; i<FA_LOG_NUM_LEVELS; ++i) {
        defaultLogDestinations[i] = FA_LOG_DEST_NONE;
    }
    // Remember, the severity values have lower numbers as the most severe.
    for (FaLogLevel i=0; i<=minimumSeverity; ++i) {
        defaultLogDestinations[i] = destinations;
    }

#if defined(FEATURE_LOG_TO_SYSLOG)
    faSyslogInitialize();
#endif
    g_LogIsInitialized = true;
}

// Create an override for the given source file.
void faLogConfigureFile(const char *filename, FaLogLevel minimumSeverity, FaLogDestinationSet destinations)
{
    if (overrideCount >= OVERRIDE_MAX) {
        // Too many files.
        return;
    }
    size_t nameLength = strlen(filename) + 1;// allow room for \0
    if (nameBufferUsed + nameLength >= nameBufferSize) {
        // Not enough room to save the name.
        return;
    }
    strncpy(&nameBuffer[nameBufferUsed], filename, nameBufferSize - nameBufferUsed);
    overrides[overrideCount].filename = &nameBuffer[nameBufferUsed];
    for (FaLogLevel i=0; i<=minimumSeverity; ++i) {
        overrides[overrideCount].destinations[i] = destinations;
    }
    ++overrideCount;
    nameBufferUsed += nameLength;
}

// log the assertion failure and exit the program.
void faLogAssertionFail(const char *filename, uint32_t line, const char *expression)
{
    faLog(filename, line, FA_LOG_LEVEL_CRITICAL, "ERROR: assert(%s) failed", expression);
    exit(1);
}
