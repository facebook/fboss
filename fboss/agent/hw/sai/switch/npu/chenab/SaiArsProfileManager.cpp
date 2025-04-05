/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/hw/sai/switch/SaiArsProfileManager.h"

namespace facebook::fboss {

#if SAI_API_VERSION >= SAI_VERSION(1, 14, 0)

SaiArsProfileTraits::CreateAttributes SaiArsProfileManager::createAttributes(
    const std::shared_ptr<FlowletSwitchingConfig>& /* unused */) {
  // Chenab will use the default when 0 is passed in. We will replace these
  // with config values in the future
  SaiArsProfileTraits::CreateAttributes attributes{0, 0, 0};

  return attributes;
}
#endif

} // namespace facebook::fboss
