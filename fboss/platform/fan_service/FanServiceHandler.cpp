// Copyright 2021-present Facebook. All Rights Reserved.

// Implementation of FanServiceHandler class. Refer to .h file
// for functional description
#include "FanServiceHandler.h"

facebook::fb303::cpp2::fb_status
facebook::fboss::FanServiceHandler::getStatus() {
  return facebook::fb303::cpp2::fb_status::ALIVE;
}

facebook::fboss::FanServiceHandler::FanServiceHandler(
    std::unique_ptr<FanService> fanService)
    : FacebookBase2("FanService"), service_(std::move(fanService)) {
  XLOG(INFO) << "FanServiceHandler Started";
}
