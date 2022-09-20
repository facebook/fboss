// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/lib/phy/gen-cpp2/phy_types.h"

#pragma once

namespace facebook::fboss {

std::vector<phy::PortComponent> prbsComponents(
    const std::vector<std::string>& components,
    bool returnAllIfEmpty);

} // namespace facebook::fboss
