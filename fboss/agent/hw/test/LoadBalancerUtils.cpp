/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/hw/test/LoadBalancerUtils.h"

#include <folly/IPAddress.h>

#include "fboss/agent/HwAsicTable.h"
#include "fboss/agent/HwSwitch.h"
#include "fboss/agent/LoadBalancerConfigApplier.h"
#include "fboss/agent/LoadBalancerUtils.h"
#include "fboss/agent/Platform.h"
#include "fboss/agent/SwSwitch.h"
#include "fboss/agent/TxPacket.h"
#include "fboss/agent/hw/test/ConfigFactory.h"
#include "fboss/agent/hw/test/HwTestAclUtils.h"
#include "fboss/agent/hw/test/HwTestPacketUtils.h"
#include "fboss/agent/packet/EthFrame.h"
#include "fboss/agent/state/LoadBalancer.h"
#include "fboss/agent/state/SwitchState.h"
#include "fboss/agent/test/ResourceLibUtil.h"

#include "folly/MacAddress.h"

#include <folly/gen/Base.h>
#include <gtest/gtest.h>
#include <sstream>

namespace facebook::fboss::utility {

bool isHwDeterministicSeed(
    HwSwitch* hwSwitch,
    const std::shared_ptr<SwitchState>& state,
    LoadBalancerID id) {
  if (!hwSwitch->getPlatform()->getAsic()->isSupported(
          HwAsic::Feature::HASH_FIELDS_CUSTOMIZATION)) {
    // hash seed is not programmed or configured
    return true;
  }
  auto lb = state->getLoadBalancers()->getNode(id);
  return lb->getSeed() == hwSwitch->generateDeterministicSeed(id);
}

} // namespace facebook::fboss::utility
