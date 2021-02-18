// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once

#include <vector>

namespace facebook::fboss {

using BcmQosMapIdAndFlag = std::pair<int, int>;

std::vector<BcmQosMapIdAndFlag> getBcmQosMapIdsAndFlags(int unit);

// pfc default maps
const std::vector<int>& getBcmDefaultTrafficClassToPgArr();
const std::vector<int>& getBcmDefaultPfcPriorityToPgArr();
int getBcmDefaultPfcPriorityToPgSize();
int getDefaultProfileId();
int getBcmDefaultTrafficClassToPgSize();
const std::vector<int>& getBcmDefaultPfcPriorityToQueueArr();

} // namespace facebook::fboss
