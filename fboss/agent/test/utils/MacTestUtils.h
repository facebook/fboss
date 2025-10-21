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

#include "fboss/agent/if/gen-cpp2/ctrl_types.h"

#include <folly/MacAddress.h>

namespace facebook::fboss {

class TestEnsembleIf;
class SwSwitch;

namespace utility {

void setMacAgeTimerSeconds(
    facebook::fboss::TestEnsembleIf* ensemble,
    uint32_t seconds);
std::vector<L2EntryThrift> getL2Table(SwSwitch* sw_, bool sdk = false);

} // namespace utility
} // namespace facebook::fboss
