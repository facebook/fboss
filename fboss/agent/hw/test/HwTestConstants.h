// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once

#include <vector>
#include "fboss/agent/HwSwitch.h"

namespace facebook::fboss::utility {
std::vector<facebook::fboss::MplsLabelStack> kHwTestLabelStacks();

std::vector<uint64_t> kHwTestEcmpWeights();

std::vector<uint64_t> kHwTestUcmpWeights();

uint32_t constexpr kHwTestMplsLabel = 1001;

} // namespace facebook::fboss::utility
