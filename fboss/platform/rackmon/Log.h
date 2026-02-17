// Copyright 2021-present Facebook. All Rights Reserved.
#pragma once
#include <glog/logging.h>

#if defined(RACKMON_SYSLOG)

#define logError SYSLOG(ERROR)
#define logWarn SYSLOG(WARNING)
#define logInfo SYSLOG(INFO)

#else // !defined(RACKMON_SYSLOG)

#define logError LOG(ERROR)
#define logWarn LOG(WARNING)
#define logInfo LOG(INFO)

#endif // defined(RACKMON_SYSLOG)

#define RACKMON_PROFILE_SCOPE(name, desc)
