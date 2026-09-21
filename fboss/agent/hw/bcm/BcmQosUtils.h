// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once

#include <vector>

namespace facebook::fboss {

using BcmQosMapIdAndFlag = std::pair<int, int>;

std::vector<BcmQosMapIdAndFlag> getBcmQosMapIdsAndFlags(int unit);

// pfc default maps
const std::vector<int>& getDefaultTrafficClassToPg();
const std::vector<int>& getDefaultPfcPriorityToPg();
int getDefaultProfileId();
const std::vector<int>& getDefaultPfcPriorityToQueue();
const std::vector<int>& getDefaultPfcEnablePriorityToQueue();
const std::vector<int>& getDefaultPfcOptimizedPriorityToQueue();

} // namespace facebook::fboss
