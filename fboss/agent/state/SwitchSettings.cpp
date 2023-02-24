/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/state/SwitchSettings.h"
#include <fboss/agent/gen-cpp2/switch_config_types.h>
#include "common/network/if/gen-cpp2/Address_types.h"
#include "fboss/agent/AddressUtil.h"
#include "fboss/agent/state/SwitchState.h"

#include "fboss/agent/state/NodeBase-defs.h"
#include "folly/dynamic.h"
#include "folly/json.h"

namespace {
constexpr auto kL2LearningMode = "l2LearningMode";
constexpr auto kQcmEnable = "qcmEnable";
constexpr auto kPtpTcEnable = "ptpTcEnable";
constexpr auto kL2AgeTimerSeconds = "l2AgeTimerSeconds";
constexpr auto kMaxRouteCounterIDs = "maxRouteCounterIDs";
constexpr auto kBlockNeighbors = "blockNeighbors";
constexpr auto kBlockNeighborVlanID = "blockNeighborVlanID";
constexpr auto kBlockNeighborIP = "blockNeighborIP";
constexpr auto kMacAddrsToBlock = "macAddrsToBlock";
constexpr auto kMacAddrToBlockVlanID = "macAddrToBlockVlanID";
constexpr auto kMacAddrToBlockAddr = "macAddrToBlockAddr";
constexpr auto kSwitchType = "switchType";
constexpr auto kSwitchId = "switchId";
constexpr auto kExactMatchTableConfigs = "exactMatchTableConfigs";
constexpr auto kExactMatchTableName = "name";
constexpr auto kExactMatchTableDstPrefixLength = "dstPrefixLength";

} // namespace

namespace facebook::fboss {

SwitchSettings* SwitchSettings::modify(std::shared_ptr<SwitchState>* state) {
  if (!isPublished()) {
    CHECK(!(*state)->isPublished());
    return this;
  }

  SwitchState::modify(state);
  auto newSwitchSettings = clone();
  auto* ptr = newSwitchSettings.get();
  (*state)->resetSwitchSettings(std::move(newSwitchSettings));
  return ptr;
}

template class ThriftStructNode<SwitchSettings, state::SwitchSettingsFields>;

} // namespace facebook::fboss
