// Copyright 2021-present Facebook. All Rights Reserved.

// Implementation of FanServiceHandler class. Refer to .h file
// for functional description
#include "FanServiceHandler.h"

#include <folly/logging/xlog.h>

namespace facebook::fboss::platform {
facebook::fb303::cpp2::fb_status FanServiceHandler::getStatus() {
  return facebook::fb303::cpp2::fb_status::ALIVE;
}

FanServiceHandler::FanServiceHandler(std::unique_ptr<FanService> fanService)
    : ::facebook::fb303::FacebookBase2DeprecationMigration("FanService"),
      service_(std::move(fanService)) {
  XLOG(INFO) << "FanServiceHandler Started";
}
} // namespace facebook::fboss::platform
