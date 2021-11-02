// Copyright 2021- Facebook. All rights reserved.

#pragma once

// Standard C++ include files
#include <memory>
#include <mutex>
#include <string>

// Files needed for XLOG
#include <folly/logging/Init.h>
#include <folly/logging/xlog.h>

// Include files for Fan Service class and handler class
#include "fboss/platform/fan_service/FanService.h"
#include "fboss/platform/fan_service/FanServiceHandler.h"

// Other Facebook specific service headerfiles
#include "common/init/Init.h"
#include "common/services/cpp/AclCheckerModule.h"
#include "common/services/cpp/BuildModule.h"
#include "common/services/cpp/Fb303Module.h"
#include "common/services/cpp/ServiceFrameworkLight.h"
#include "common/services/cpp/ThriftStatsModule.h"

// Required for using gFlags
#include <gflags/gflags.h>

// Need this in order to use Thrift server
namespace apache {
namespace thrift {
class ThriftServer;
}
} // namespace apache
