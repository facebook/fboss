// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once

#include <vector>

namespace facebook::fboss {

using BcmQosMapIdAndFlag = std::pair<int, int>;

std::vector<BcmQosMapIdAndFlag> getBcmQosMapIdsAndFlags(int unit);

} // namespace facebook::fboss
