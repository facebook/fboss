/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/hw/bcm/SdkWrapSettings.h"

#include <folly/Singleton.h>

namespace facebook::fboss {

folly::Singleton<facebook::fboss::SdkWrapSettings> _sdkWrapSettings;
std::shared_ptr<SdkWrapSettings> SdkWrapSettings::getInstance() {
  return _sdkWrapSettings.try_get();
}

} // namespace facebook::fboss
