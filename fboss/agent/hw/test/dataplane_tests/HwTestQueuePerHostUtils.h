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
#include "fboss/agent/gen-cpp2/switch_config_types.h"
#include "fboss/agent/types.h"

/*
 * This utility is to provide utils for bcm queue-per host tests.
 */

namespace facebook::fboss::utility {

constexpr int kQueuePerHostWeight = 1;

constexpr int kQueuePerHostQueue0 = 0;
constexpr int kQueuePerHostQueue1 = 1;
constexpr int kQueuePerHostQueue2 = 2;
constexpr int kQueuePerHostQueue3 = 3;
constexpr int kQueuePerHostQueue4 = 4;

const std::vector<int>& kQueuePerhostQueueIds();
const std::vector<cfg::AclLookupClass>& kLookupClasses();

void addQueuePerHostQueueConfig(cfg::SwitchConfig* config, PortID portID);
void addQueuePerHostAcls(cfg::SwitchConfig* config);

} // namespace facebook::fboss::utility
