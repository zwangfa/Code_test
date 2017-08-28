// Copyright (C) 2017 BRK Brands, Inc. All Rights Reserved.

///@file
// First Alert Logging Facility

/// The goal is to provide a common way to log events in code and to provide a
/// flexible control mechanism. The API defined here provides a target
/// independent means of generating log messages. The initialization and
/// configuration functions provide flexibility to control what messages are
/// generated and where they are sent.
///
/// The severity level of a message allows us to filter messages. In production,
/// we may want to log only the most severe messages to keep the overhead
/// low. During development of a feature, all log message may be desired. The
/// name of the file may be used to control the minimum severity level. This
/// allows developers to get more log messages from a particular file compared
/// to the default settings that apply to all other files.

#ifndef SRC_LOG_H
#define SRC_LOG_H

#include <string.h>
#include <stdint.h>

/// Identify the severity of the message.  The values range from the system is
/// about to crash to the low-level details of a particular implementation.
typedef enum FaLogLevel {
    FA_LOG_LEVEL_CRITICAL, ///< The system is about to die
    FA_LOG_LEVEL_ERROR,    ///< Some functionality may be lost.
    FA_LOG_LEVEL_WARNING,  ///< A potentially recoverable problem
    FA_LOG_LEVEL_NOTICE,   ///< Normal but significant condition
    FA_LOG_LEVEL_INFO,     ///< Informational message
    FA_LOG_LEVEL_DEBUG,    ///< the nitty gritty details.

    FA_LOG_NUM_LEVELS      ///< The number of FaLogLevel values
} FaLogLevel;


//extern FaLogLevel g_defaultLogLevel;
/// Select where to send the log output. Not every destination will be available
/// on all targets.
typedef enum FaLogDestination {
    FA_LOG_DEST_DEFAULT = 0,
    FA_LOG_DEST_NONE    = 1<<0, ///< lost for all time.
    FA_LOG_DEST_CONSOLE = 1<<1, ///< console(linux) or debug UART(bare metal)
    FA_LOG_DEST_SYSLOG  = 1<<2, ///< SYSLOG
    FA_LOG_DEST_CBUFFER = 1<<3, ///< Circular buffer
} FaLogDestination;

/// We want to be able to send the log messages to multiple destinations. The
/// \ref FaLogDestination values are non-intersecting. To create a set, just
/// bitwise-or the values together.
typedef int FaLogDestinationSet;

/// Log a critical error - one that the system cannot recover from without
/// rebooting.
/// @param [in] ... printf style format string and additional parameters as
/// needed by the format string.
#define FA_CRITICAL(...) faLog(CURRENT_FILENAME, __LINE__, FA_LOG_LEVEL_CRITICAL, __VA_ARGS__)
/// Log an error - one that some functionality may be lost.
/// @param [in] ... printf style format string and additional parameters as
/// needed by the format string.
#define FA_ERROR(...)    faLog(CURRENT_FILENAME, __LINE__, FA_LOG_LEVEL_ERROR, __VA_ARGS__)
/// Log a warning - a problem that we may be able to work around.
/// @param [in] ... printf style format string and additional parameters as
/// needed by the format string.
#define FA_WARNING(...)  faLog(CURRENT_FILENAME, __LINE__, FA_LOG_LEVEL_WARNING, __VA_ARGS__)
/// Log a normal, but significant condition.
/// @param [in] ... printf style format string and additional parameters as
/// needed by the format string.
#define FA_NOTICE(...)   faLog(CURRENT_FILENAME, __LINE__, FA_LOG_LEVEL_NOTICE, __VA_ARGS__)
/// Log less important information.
/// @param [in] ... printf style format string and additional parameters as
/// needed by the format string.
#define FA_INFO(...)     faLog(CURRENT_FILENAME, __LINE__, FA_LOG_LEVEL_INFO, __VA_ARGS__)
/// Log the low level details that may cause the logging mechanisms to consume
/// significant resources.
/// @param [in] ... printf style format string and additional parameters as
/// needed by the format string.
#define FA_DEBUG(...)    faLog(CURRENT_FILENAME, __LINE__, FA_LOG_LEVEL_DEBUG, __VA_ARGS__)

/// Our own assert function. If the expression does not evaluate to a true
/// value, the equivalent of FA_CRITICAL is called to log a message explaining
/// the assertion failed, and the program exits with a non-zero status.
#define assert(expression) do { if ((expression)==0) { \
            faLogAssertionFail(CURRENT_FILENAME, __LINE__, #expression); \
        }} while(0)

/// We need a fairly portable way to say a function will not return.
#if __STDC_VERSION__ >= 201112L
#define NORETURN _Noreturn
#else
#define NORETURN __attribute__((__noreturn__))
#endif

/// Generate a CRITICAL severity log message explaining the assertion failed,
/// and call exit with a non-zero status.
NORETURN void faLogAssertionFail(const char *filename, uint32_t line, const char *expression);

/// Get the name of the current file without any path information.
#define CURRENT_FILENAME      (strrchr("/" __FILE__, '/')+1)

#ifdef __GNUC__
#define FA_PRINTF_ARGS(FMT,ARGS) __attribute__ ((format (printf, FMT, ARGS)))
#endif

/// Send the given log message to the pre-configured destination(s).
/// Rather than invoking this function directly, use the logging macros instead.
/// @param [in] filename name of the current file. This should not include any
///             path components.
/// @param [in] line line number where this function is being called from.
/// @param [in] severity represents the importance level of the log message.
///             This value can be used to filter messages, causing low severity
///             messages to be dropped.
/// @param [in] format printf format string used to format the contents of the
///             log message. Please keep it simple. This function may use a
///             simple formatter, that does not handle the less common options,
///             to reduce the code size.
/// @param [in] ... Zero or more parameters as required by the format string.
void faLog(const char *filename, uint32_t line, FaLogLevel severity,
           const char *format, ...) FA_PRINTF_ARGS(4,5);

/// Initialize the First Alert Logging system. Set the default logging settings
/// for all files. Settings for individual files can be changed by calling
/// \ref faLogConfigureFile.
/// @param [in] minimumSeverity the minimum severity level that a message must
///             be for it to be sent to a logging destination.
/// @param [in] destinations set of \ref FaLogDestination values or'd together.
///    If the target does not support a destination, it will be silently
///    ignored. For example, if <tt>(FA_LOG_DEST_CONSOLE | FA_LOG_DEST_SYSLOG |
///    FA_LOG_DEST_CBUFFER)</tt> is passed to \p destinations and the target
///    does not support syslog, then the log message would only be sent to the
///    console/debug UART and to the circular buffer.
void faLogInitialize(FaLogLevel minimumSeverity, FaLogDestinationSet destinations);

/// Override the default logging parameters for a given source file. Any log
/// messages generated from \p filename with \p minimumSeverity or more severe,
/// will be sent to \p destinations. If the severity is less than
/// \p minimumSeverity, the default settings apply.
///
/// The general use case is to enable DEBUG messages from a file under
/// development while the default might be set to only display NOTICE and above
/// messages. The developer gets more info from the code being worked on without
/// being flooded with debug messages from the rest of the system.
///
/// At this time, only a limited number of files can be customized.
///
/// \param [in] filename base name of the source file. Do not include the path.
/// \param [in] minimumSeverity messages with a severity level between this and
///                 FA_LOG_LEVEL_CRITICAL will be affected.
/// \param [in] destinations the set of \ref FaLogDestination values where the
///                 log messages are to be sent. The set is formed by or'ing
///                 together the desired \ref FaLogDestination values.
void faLogConfigureFile(const char *filename, FaLogLevel minimumSeverity, FaLogDestinationSet destinations);

#endif
