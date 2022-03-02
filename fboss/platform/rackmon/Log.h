// Copyright 2021-present Facebook. All Rights Reserved.
#pragma once
#include <glog/logging.h>

#if defined(__TEST__)
#define logError LOG(ERROR)
#define logWarn LOG(WARNING)
#define logInfo LOG(INFO)
#elif defined(RACKMON_SYSLOG)
#define logError SYSLOG(ERROR)
#define logWarn SYSLOG(WARNING)
#define logInfo SYSLOG(INFO)
#else
#define logError LOG(ERROR)
#define logWarn LOG(WARNING)
#define logInfo LOG(INFO)
#endif

#ifdef PROFILING
#include <openbmc/profile.hpp>
#define RACKMON_PROFILE_SCOPE(name, desc, os) openbmc::Profile name(desc, os)
#else
#define RACKMON_PROFILE_SCOPE(name, desc, os)
#endif
