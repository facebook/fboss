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

#include "folly/MacAddress.h"

/*
 * This utility is to provide utils for bcm queue-per host tests.
 */

// Forward declarations
namespace facebook::fboss {
class HwSwitch;
class HwSwitchEnsemble;
} // namespace facebook::fboss

namespace folly {
class IPAddress;
}

namespace facebook::fboss::utility {

constexpr int kQueuePerHostWeight = 1;

constexpr int kQueuePerHostQueue0 = 0;
constexpr int kQueuePerHostQueue1 = 1;
constexpr int kQueuePerHostQueue2 = 2;
constexpr int kQueuePerHostQueue3 = 3;
constexpr int kQueuePerHostQueue4 = 4;

// For verifyQueuePerHostMapping()
constexpr int kQueueId = 2;

const std::vector<int>& kQueuePerhostQueueIds();
const std::vector<cfg::AclLookupClass>& kLookupClasses();

void addQueuePerHostQueueConfig(cfg::SwitchConfig* config);
void addQueuePerHostAcls(cfg::SwitchConfig* config);

std::string getAclTableGroupName();
std::string getQueuePerHostAclTableName();
std::string getTtlAclTableName();

std::string getQueuePerHostTtlAclName();
std::string getQueuePerHostTtlCounterName();

void verifyQueuePerHostMapping(
    const HwSwitch* hwSwitch,
    HwSwitchEnsemble* ensemble,
    VlanID vlanId,
    folly::MacAddress srcMac,
    folly::MacAddress dstMac,
    const folly::IPAddress& srcIp,
    const folly::IPAddress& dstIp,
    bool useFrontPanel,
    bool blockNeighbor);

void addQueuePerHostAclTables(cfg::SwitchConfig* config);

} // namespace facebook::fboss::utility
