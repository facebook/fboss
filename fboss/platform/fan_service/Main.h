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
#include <folly/experimental/FunctionScheduler.h>
#include "fboss/platform/fan_service/FanService.h"
#include "fboss/platform/fan_service/FanServiceHandler.h"

// Required for using gFlags
#include <gflags/gflags.h>
