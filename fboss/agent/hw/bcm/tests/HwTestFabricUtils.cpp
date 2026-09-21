// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/agent/hw/test/HwTestFabricUtils.h"
#include "fboss/agent/FbossError.h"

namespace facebook::fboss {

void setForceTrafficOverFabric(const HwSwitch* /*hw*/, bool /*force*/) {
  throw FbossError("Force over fabric not supported");
}

} // namespace facebook::fboss
