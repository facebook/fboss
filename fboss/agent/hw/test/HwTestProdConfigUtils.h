/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#pragma once
#include "fboss/agent/hw/switch_asics/HwAsic.h"
#include "fboss/agent/platforms/common/PlatformMapping.h"
#include "fboss/agent/types.h"

namespace facebook::fboss {
class HwSwitch;
namespace cfg {
class SwitchConfig;
}

namespace utility {
/*
 * Add a representative set of prod features to config
 */
void addProdFeaturesToConfig(
    cfg::SwitchConfig& config,
    const HwAsic* hwAsic,
    bool isSai);
} // namespace utility
} // namespace facebook::fboss
