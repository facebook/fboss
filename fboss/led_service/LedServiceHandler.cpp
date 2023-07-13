// Copyright 2021-present Facebook. All Rights Reserved.

#include "fboss/led_service/LedServiceHandler.h"

namespace facebook::fboss {

/*
 * LedServiceHandler ctor()
 *
 * Create service handler with service object
 */
LedServiceHandler::LedServiceHandler(std::unique_ptr<LedService> ledService)
    : service_(std::move(ledService)) {
  XLOG(INFO) << "LedServiceHandler Started";
}

} // namespace facebook::fboss
