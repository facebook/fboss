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

#include "fboss/agent/gen-cpp2/platform_config_types.h"
#include "fboss/lib/phy/gen-cpp2/phy_types.h"

namespace facebook {
namespace fboss {
namespace utility {

std::vector<phy::PinID> getTransceiverLanes(
    const cfg::PlatformPortEntry& port,
    const std::vector<phy::DataPlanePhyChip>& chips,
    std::optional<cfg::PortProfileID> profileID = std::nullopt);

} // namespace utility
} // namespace fboss
} // namespace facebook
