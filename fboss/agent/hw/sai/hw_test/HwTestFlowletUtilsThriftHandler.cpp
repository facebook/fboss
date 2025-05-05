// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/agent/FbossError.h"
#include "fboss/agent/hw/test/HwTestThriftHandler.h"

namespace facebook::fboss::utility {

void HwTestThriftHandler::updateFlowletStats() {
  throw FbossError("Flowlet stats are not supported in SaiSwitch.");
}

} // namespace facebook::fboss::utility
