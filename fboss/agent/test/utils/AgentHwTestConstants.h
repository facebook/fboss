// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include "fboss/agent/if/gen-cpp2/mpls_types.h"

#include <vector>

namespace facebook::fboss::utility {

std::vector<facebook::fboss::MplsLabelStack> kHwTestLabelStacks();

std::vector<uint64_t> kHwTestEcmpWeights();

std::vector<uint64_t> kHwTestUcmpWeights();

std::vector<uint64_t> kHwTestWideUcmpWeights();

uint32_t constexpr kHwTestMplsLabel = 1001;

} // namespace facebook::fboss::utility
