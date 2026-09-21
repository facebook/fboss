/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#pragma once

#include <cstdint>

namespace facebook::fboss {
class HwSwitch;
namespace utility {

int32_t getSwitchingModeFromHw(const facebook::fboss::HwSwitch* hw);

} // namespace utility
} // namespace facebook::fboss
