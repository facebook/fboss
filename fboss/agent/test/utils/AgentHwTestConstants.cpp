// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/agent/test/utils/AgentHwTestConstants.h"

namespace facebook::fboss::utility {
std::vector<facebook::fboss::MplsLabelStack> kHwTestLabelStacks() {
  return {
      {101, 102, 103},
      {201, 202, 203},
      {301, 302, 303},
      {401, 402, 403},
      {501, 502, 503},
      {601, 602, 603},
      {701, 702, 703},
      {801, 802, 803}};
}

std::vector<uint64_t> kHwTestEcmpWeights() {
  return {};
}

// Total normalized weight 36. Normal ecmp used in hw
std::vector<uint64_t> kHwTestUcmpWeights() {
  return {10, 20, 30, 40, 50, 60, 70, 80};
}

// Total weight 361 leads to wide ecmp in HW
std::vector<uint64_t> kHwTestWideUcmpWeights() {
  return {11, 20, 30, 40, 50, 60, 70, 80};
}

} // namespace facebook::fboss::utility
