// Copyright (C) 2017 BRK Brands, Inc. All Rights Reserved.

///\file
/// Common defines and other configuration items.

#define FEATURE_LOG_TO_STDOUT 1 ///< The target supports printing to stdout.
#define FEATURE_LOG_TO_SYSLOG 1 ///< The target supports logging to syslog.
/// Set the name to use when logging via syslog.
#define FEATURE_LOG_TO_SYSLOG_NAME "FirstAlert"
//#define FEATURE_LOG_TO_CBUF   1 ///< The target supports logging to a circular buffer.

#ifndef STATIC
/// For unit testing we need a way to directly call the private functions in a
/// module. By using this define, builds for unit testing can make the private
/// functions public.
#define STATIC static
#endif
