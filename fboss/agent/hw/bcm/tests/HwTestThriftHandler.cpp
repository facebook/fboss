// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/agent/hw/test/HwTestThriftHandler.h"

namespace facebook::fboss::utility {
std::shared_ptr<HwTestThriftHandler> createHwTestThriftHandler(HwSwitch* hw) {
  return std::make_shared<HwTestThriftHandler>(hw);
}
} // namespace facebook::fboss::utility
