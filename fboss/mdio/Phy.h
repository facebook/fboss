/*
 *  Copyright (c) 2018-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#pragma once

#include <cstdint>

namespace facebook {
namespace fboss {
namespace phy {

using PhyAddress = uint8_t; // 5b, 0-31
using Cl22RegisterAddress = uint8_t; // 5b, 0-31
using Cl22Data = uint16_t; // 16b
using Cl45DeviceAddress = uint8_t; // 5b, 0-31
using Cl45RegisterAddress = uint16_t; // 16b
using Cl45Data = uint16_t; // 16b

} // namespace phy
} // namespace fboss
} // namespace facebook
