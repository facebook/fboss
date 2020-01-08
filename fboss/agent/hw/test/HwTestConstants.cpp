// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/agent/hw/test/HwTestConstants.h"

namespace facebook::fboss::utility {
std::vector<facebook::fboss::MplsLabelStack> kHwTestLabelStacks() {
  return {{101, 102, 103},
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

std::vector<uint64_t> kHwTestUcmpWeights() {
  return {10, 20, 30, 40, 50, 60, 70, 80};
}

} // namespace facebook::fboss::utility
