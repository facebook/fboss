// Copyright 2021-present Facebook. All Rights Reserved.

// Implementation of FanServiceHandler class. Refer to .h file
// for functional description
#include "FanServiceHandler.h"

#include <folly/logging/xlog.h>

namespace facebook::fboss::platform::fan_service {

FanServiceHandler::FanServiceHandler(
    std::unique_ptr<FanServiceImpl> fanServiceImpl)
    : fanServiceImpl_(std::move(fanServiceImpl)) {
  XLOG(INFO) << "FanServiceHandler Started";
}
} // namespace facebook::fboss::platform::fan_service
