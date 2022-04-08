// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include "fboss/lib/phy/gen-cpp2/phy_types.h"

namespace facebook::fboss::utility {

bool isReedSolomonFec(phy::FecMode fec);

} // namespace facebook::fboss::utility
