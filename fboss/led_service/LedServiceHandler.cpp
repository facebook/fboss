// Copyright 2021-present Facebook. All Rights Reserved.

#include "fboss/led_service/LedServiceHandler.h"

namespace facebook::fboss {

/*
 * getStatus
 *
 * If the call is successfully made then the service is alive so return that
 */
facebook::fb303::cpp2::fb_status LedServiceHandler::getStatus() {
  return facebook::fb303::cpp2::fb_status::ALIVE;
}

/*
 * LedServiceHandler ctor()
 *
 * Create service handler with service object
 */
LedServiceHandler::LedServiceHandler(std::unique_ptr<LedService> ledService)
    : ::facebook::fb303::FacebookBase2DeprecationMigration("LedService"),
      service_(std::move(ledService)) {
  XLOG(INFO) << "LedServiceHandler Started";
}

} // namespace facebook::fboss
