// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include "fboss/led_service/LedManager.h"

namespace facebook::fboss {

std::unique_ptr<LedManager> createLedManager();

} // namespace facebook::fboss
