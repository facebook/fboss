/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/platforms/test_platforms/BcmTestYampPlatform.h"
#include "fboss/agent/platforms/test_platforms/BcmTestYampPort.h"

namespace {
static const std::array<int, 128> kMasterLogicalPortIds = {
    // TODO(joseph5wu) If in the future, we need to support both 16q and 4dd
    // pims, we need to implement a function to decide what's the correct
    // logical ports for each pims. For now, we can just use 16q pim.
    // pim1
    20,
    21,
    22,
    23,
    1,
    2,
    3,
    4,
    40,
    41,
    42,
    43,
    60,
    61,
    62,
    63,
    // pim2
    44,
    45,
    46,
    47,
    5,
    6,
    7,
    8,
    24,
    25,
    26,
    27,
    64,
    65,
    66,
    67,
    // pim3
    28,
    29,
    30,
    31,
    9,
    10,
    11,
    12,
    48,
    49,
    50,
    51,
    68,
    69,
    70,
    71,
    // pim4
    52,
    53,
    54,
    55,
    32,
    33,
    34,
    35,
    13,
    14,
    15,
    16,
    72,
    73,
    74,
    75,
    // pim5
    100,
    101,
    102,
    103,
    120,
    121,
    122,
    123,
    140,
    141,
    142,
    143,
    80,
    81,
    82,
    83,
    // pim6
    124,
    125,
    126,
    127,
    144,
    145,
    146,
    147,
    104,
    105,
    106,
    107,
    84,
    85,
    86,
    87,
    // pim7
    108,
    109,
    110,
    111,
    148,
    149,
    150,
    151,
    128,
    129,
    130,
    131,
    88,
    89,
    90,
    91,
    // pim8
    132,
    133,
    134,
    135,
    152,
    153,
    154,
    155,
    112,
    113,
    114,
    115,
    92,
    93,
    94,
    95};

constexpr uint8_t kNumPortsPerTransceiver = 1;
} // unnamed namespace

namespace facebook {
namespace fboss {
BcmTestYampPlatform::BcmTestYampPlatform()
    : BcmTestWedgeTomahawk3Platform(
          std::vector<PortID>(
              kMasterLogicalPortIds.begin(),
              kMasterLogicalPortIds.end()),
          kNumPortsPerTransceiver) {}

std::unique_ptr<BcmTestPort> BcmTestYampPlatform::createTestPort(PortID id) {
  return std::make_unique<BcmTestYampPort>(id, this);
}
} // namespace fboss
} // namespace facebook
