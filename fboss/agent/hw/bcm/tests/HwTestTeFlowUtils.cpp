/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/hw/test/HwTestTeFlowUtils.h"

#include <fboss/agent/gen-cpp2/switch_config_types.h>
#include "fboss/agent/hw/bcm/BcmExactMatchUtils.h"
#include "fboss/agent/hw/bcm/BcmFieldProcessorUtils.h"
#include "fboss/agent/hw/bcm/BcmIngressFieldProcessorFlexCounter.h"
#include "fboss/agent/hw/bcm/BcmSwitch.h"
#include "fboss/agent/hw/bcm/BcmTeFlowTable.h"
#include "fboss/agent/state/SwitchState.h"

#include <gtest/gtest.h>

namespace facebook::fboss::utility {

bool validateTeFlowGroupEnabled(
    const facebook::fboss::HwSwitch* hw,
    int prefixLength) {
  const auto bcmSwitch = static_cast<const BcmSwitch*>(hw);

  if (!prefixLength) {
    return bcmSwitch->getTeFlowTable()->getDstIpPrefixLength() == prefixLength;
  }

  int gid = bcmSwitch->getPlatform()->getAsic()->getDefaultTeFlowGroupID();
  return fpGroupExists(bcmSwitch->getUnit(), getEMGroupID(gid)) &&
      validateDstIpHint(
             bcmSwitch->getUnit(),
             bcmSwitch->getTeFlowTable()->getHintId(),
             getEmDstIpHintStartBit(prefixLength),
             getEmDstIpHintEndBit());
}

int getNumTeFlowEntries(const facebook::fboss::HwSwitch* hw) {
  const auto bcmSwitch = static_cast<const BcmSwitch*>(hw);

  int gid = bcmSwitch->getPlatform()->getAsic()->getDefaultTeFlowGroupID();
  int size = 0;

  auto rv = bcm_field_entry_multi_get(
      bcmSwitch->getUnit(), getEMGroupID(gid), 0, nullptr, &size);
  bcmCheckError(
      rv,
      "failed to get field group entry count, gid=",
      folly::to<std::string>(gid));
  return size;
}

void checkSwHwTeFlowMatch(
    const HwSwitch* hw,
    std::shared_ptr<SwitchState> state,
    TeFlow flow) {
  const auto bcmSwitch = static_cast<const BcmSwitch*>(hw);
  auto flowEntry =
      state->getMultiSwitchTeFlowTable()->getNodeIf(getTeFlowStr(flow));
  int gid = bcmSwitch->getPlatform()->getAsic()->getDefaultTeFlowGroupID();

  auto hwTeFlows = bcmSwitch->getTeFlowTable()->getTeFlowIf(flowEntry);
  ASSERT_NE(nullptr, hwTeFlows);
  ASSERT_TRUE(BcmTeFlowEntry::isStateSame(
      bcmSwitch, getEMGroupID(gid), hwTeFlows->getHandle(), flowEntry));
}

uint64_t getTeFlowOutBytes(const HwSwitch* hw, std::string statName) {
  const auto bcmSwitch = static_cast<const BcmSwitch*>(hw);
  auto statHandle =
      bcmSwitch->getTeFlowTable()->getTeFlowStat(statName)->getHandle();
  uint64_t value;
  if (hw->getPlatform()->getAsic()->isSupported(
          HwAsic::Feature::INGRESS_FIELD_PROCESSOR_FLEX_COUNTER)) {
    std::vector<cfg::CounterType> counterTypes = {cfg::CounterType::BYTES};
    auto actionIndex =
        bcmSwitch->getTeFlowTable()->getTeFlowStat(statName)->getActionIndex();
    const auto& stats =
        BcmIngressFieldProcessorFlexCounter::getAclTrafficFlexCounterStats(
            bcmSwitch->getUnit(), statHandle, actionIndex, counterTypes);
    value = stats.at(cfg::CounterType::BYTES);
  } else {
    auto rv = bcm_field_stat_sync_get(
        bcmSwitch->getUnit(), statHandle, bcmFieldStatBytes, &value);
    bcmCheckError(rv, "Failed to update stat=", statHandle);
  }
  return value;
}

} // namespace facebook::fboss::utility
