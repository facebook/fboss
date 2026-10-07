/*
 *  Copyright (c) 2004-present, Meta Platforms, Inc. and affiliates.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/platform/fixmyfboss/CheckRegistry.h"

#include "fboss/platform/platform_checks/checks/MacAddressCheck.h"
#include "fboss/platform/platform_checks/checks/PciDeviceCheck.h"
#include "fboss/platform/platform_checks/checks/PowerResetCheck.h"
#include "fboss/platform/platform_checks/checks/i801SmbusTimeoutCheck.h"

namespace facebook::fboss::platform::fixmyfboss {

using namespace platform_checks;

std::vector<std::unique_ptr<PlatformCheck>> createAllChecks(
    const CheckEnvironment& env) {
  const CheckTarget x86{.host = env.x86, .platformName = env.platformName};

  std::vector<std::unique_ptr<PlatformCheck>> checks;
  checks.push_back(std::make_unique<MacAddressCheck>(x86));
  checks.push_back(std::make_unique<PciDeviceCheck>(x86));
  checks.push_back(std::make_unique<RecentManualRebootCheck>(x86));
  checks.push_back(std::make_unique<RecentKernelPanicCheck>(x86));
  checks.push_back(std::make_unique<WatchdogDidNotStopCheck>(x86));
  checks.push_back(std::make_unique<i801SmbusTimeoutCheck>(x86));
  return checks;
}

} // namespace facebook::fboss::platform::fixmyfboss
