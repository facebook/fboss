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
    const HwSwitch* hwSwitch,
    const bool mmuLossless = false,
    const std::vector<PortID>& ports = {});
} // namespace utility
} // namespace facebook::fboss
