// Copyright 2021-present Facebook. All Rights Reserved.
#pragma once
#include <glog/logging.h>
#ifdef PROFILING
#include <openbmc/profile.hpp>
#endif

#if defined(RACKMON_SYSLOG)

#define logError SYSLOG(ERROR)
#define logWarn SYSLOG(WARNING)
#define logInfo SYSLOG(INFO)
#ifdef PROFILING
#define RACKMON_PROFILE_SCOPE(name, desc)       \
  google::LogMessage name##stream(              \
      __FILE__,                                 \
      __LINE__,                                 \
      google::GLOG_INFO,                        \
      0,                                        \
      &google::LogMessage::SendToSyslogAndLog); \
  openbmc::Profile name(desc, name##stream.stream())
#else // !defined(PROFILING)
#define RACKMON_PROFILE_SCOPE(name, desc)
#endif // defined(PROFILING)

#else // !defined(RACKMON_SYSLOG)
// If we are not sending over to syslog use regular
// logging
#define logError LOG(ERROR)
#define logWarn LOG(WARNING)
#define logInfo LOG(INFO)

#ifdef PROFILING
#define RACKMON_PROFILE_SCOPE(name, desc)                                 \
  google::LogMessage name##stream(__FILE__, __LINE__, google::GLOG_INFO); \
  openbmc::Profile name(desc, name##stream.stream())
#else // ! defined(PROFILING)
#define RACKMON_PROFILE_SCOPE(name, desc)
#endif // defined(PROFILING)

#endif // defined(RACKMON_SYSLOG)
