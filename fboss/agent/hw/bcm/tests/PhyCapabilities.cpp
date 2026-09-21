// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/agent/hw/test/PhyCapabilities.h"

namespace facebook::fboss {

bool rxSignalDetectSupportedInSdk() {
#if defined(BCM_SDK_VERSION_GTE_6_5_26)
  return true;
#endif
  return false;
}

bool rxLockStatusSupportedInSdk() {
  return true;
}

bool pcsRxLinkStatusSupportedInSdk() {
  return false;
}

bool fecAlignmentLockSupportedInSdk() {
  return false;
}

} // namespace facebook::fboss
