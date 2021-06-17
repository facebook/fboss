/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/hw/bcm/BcmIngressFieldProcessorFlexCounter.h"

#include "fboss/agent/hw/bcm/BcmError.h"
#include "fboss/agent/hw/bcm/BcmSdkVer.h"

extern "C" {
#include <bcm/field.h>
#if defined(IS_OPENNSA) || defined(BCM_SDK_VERSION_GTE_6_5_20)
#include <bcm/flexctr.h>
#endif
}

namespace {
struct BcmGetNumAclStatsCountUserData {
  int count{0};
  int groupID{0};
  std::set<uint32> statCounterIDs;
};

#if defined(IS_OPENNSA) || defined(BCM_SDK_VERSION_GTE_6_5_20)
// Broadcom provided some reference code in CS00011186214 to implement the
// details of IFP. Most of the constants are following their examples.
constexpr auto kIFPActionIndexNum = 2;
constexpr auto kIFPActionIndexObj0Mask = 8;
constexpr auto kIFPActionIndexObj1Mask = 1;
constexpr auto kIFPActionPacketOperationObj0Mask = 16;
constexpr auto kIFPActionPacketOperationObj1Mask = 1;
constexpr auto kNumDefaultIFPFlexCounterPerAcl = 1;

void setIFPActionConfig(bcm_flexctr_action_t* action, int groupID) {
  action->flags = 0;
  /* Group ID passed as hint and IFP as source */
  action->hint = groupID;
  action->hint_type = bcmFlexctrHintFieldGroupId;
  action->source = bcmFlexctrSourceIfp;
  action->mode = bcmFlexctrCounterModeNormal;
  action->index_num = kIFPActionIndexNum;
  action->drop_count_mode = bcmFlexctrDropCountModeAll;
}

void setIFPActionIndex(bcm_flexctr_action_t* action) {
  bcm_flexctr_index_operation_t* index = &(action->index_operation);
  /* Counter index is PKT_ATTR_OBJ0. */
  index->object[0] = bcmFlexctrObjectStaticIngFieldStageIngress;
  index->object_id[0] = 0;
  index->mask_size[0] = kIFPActionIndexObj0Mask;
  index->shift[0] = 0;
  index->object[1] = bcmFlexctrObjectConstZero;
  index->mask_size[1] = kIFPActionIndexObj1Mask;
  index->shift[1] = 0;
}

using CounterType = facebook::fboss::cfg::CounterType;
void setActionPerPacketValueOperation(
    bcm_flexctr_value_operation_t* operation) {
  operation->select = bcmFlexctrValueSelectCounterValue;
  operation->object[0] = bcmFlexctrObjectConstOne;
  operation->mask_size[0] = kIFPActionPacketOperationObj0Mask;
  operation->shift[0] = 0;
  operation->object[1] = bcmFlexctrObjectConstZero;
  operation->mask_size[1] = kIFPActionPacketOperationObj1Mask;
  operation->shift[1] = 0;
  operation->type = bcmFlexctrValueOperationTypeInc;
}

void setActionPerBytesValueOperation(bcm_flexctr_value_operation_t* operation) {
  operation->select = bcmFlexctrValueSelectPacketLength;
  operation->type = bcmFlexctrValueOperationTypeInc;
}

void setActionValue(bcm_flexctr_action_t* action) {
  // PACKET type always use operation_a
  setActionPerPacketValueOperation(&(action->operation_a));
  setActionPerBytesValueOperation(&(action->operation_b));
}

int getNumAclStatsCounterCallback(
    int unit,
    uint32 stat_counter_id,
    bcm_flexctr_action_t* action,
    void* user_data) {
  auto* cbUserData = static_cast<BcmGetNumAclStatsCountUserData*>(user_data);
  // Check action config has the same group id
  if (action->source != bcmFlexctrSourceIfp ||
      action->hint_type != bcmFlexctrHintFieldGroupId ||
      action->hint != cbUserData->groupID) {
    return 0;
  }
  cbUserData->count++;
  cbUserData->statCounterIDs.insert(stat_counter_id);
  return 0;
}

#endif
} // namespace

namespace facebook::fboss {
BcmIngressFieldProcessorFlexCounter::BcmIngressFieldProcessorFlexCounter(
    int unit,
    std::optional<int> groupID,
    std::optional<BcmAclStatHandle> statHandle)
    : BcmFlexCounter(unit) {
  if (!groupID.has_value() && !statHandle.has_value()) {
    throw FbossError(
        "Failed to create BcmIngressFieldProcessorFlexCounter ",
        "with empty groupID and statHandle");
  }
  if (statHandle.has_value()) {
    // as long as statHandle has value, usually means the counter has been
    // created in HW, like during warmboot, we don't need to create such counter
    // again.
    counterID_ = *statHandle;
  } else {
#if defined(IS_OPENNSA) || defined(BCM_SDK_VERSION_GTE_6_5_20)
    // Ideally if bcm_flexctr_action_t is opensource, we can just have most of
    // the common logics below implement in the parent class BcmFlexCounter, so
    // that any of the child class FlexCounter won't need to re-implement this
    // creating bcm_flexctr_action_t logic in their constructors.
    // Right now, we only use flex counter for IFP ACLs, hence keep the logic
    // in the unnamed namespace for now.
    bcm_flexctr_action_t action;
    bcm_flexctr_action_t_init(&action);

    // set counter action config
    setIFPActionConfig(&action, *groupID);

    // set counter action index
    setIFPActionIndex(&action);

    // set counter action value
    setActionValue(&action);

    auto rv = bcm_flexctr_action_create(
        unit, 0 /* 0 options */, &action, &counterID_);
    bcmCheckError(
        rv,
        "Failed to create Ingress Field Processor Flex Counter for group:",
        *groupID);
    XLOG(DBG1) << "Successfully created Ingress Field Processor Flex Counter:"
               << counterID_ << " for group:" << *groupID;
#else
    throw FbossError(
        "Current SDK version doesn't support create IFP flex counter");
#endif
  }
}

void BcmIngressFieldProcessorFlexCounter::attach(BcmAclEntryHandle acl) {
#if defined(IS_OPENNSA) || defined(BCM_SDK_VERSION_GTE_6_5_20)
  bcm_field_flexctr_config_t flexCounter;
  flexCounter.flexctr_action_id = counterID_;
  flexCounter.counter_index = 1;
  flexCounter.color = BCM_FIELD_COUNT_ALL;
  auto rv = bcm_field_entry_flexctr_attach(unit_, acl, &flexCounter);
  bcmCheckError(
      rv, "Failed to attach flex counter:", counterID_, " to acl:", acl);
#else
  throw FbossError(
      "Current SDK version doesn't support attach IFP flex counter to acl");
#endif
}

void BcmIngressFieldProcessorFlexCounter::detach(
    int unit,
    BcmAclEntryHandle acl,
    BcmAclStatHandle aclStatHandle) {
#if defined(IS_OPENNSA) || defined(BCM_SDK_VERSION_GTE_6_5_20)
  bcm_field_flexctr_config_t flexCounter;
  flexCounter.flexctr_action_id = aclStatHandle;
  flexCounter.counter_index = 1;
  flexCounter.color = BCM_FIELD_COUNT_ALL;
  auto rv = bcm_field_entry_flexctr_detach(unit, acl, &flexCounter);
  bcmCheckError(
      rv, "Failed to detach flex counter:", aclStatHandle, " from acl:", acl);
#else
  throw FbossError(
      "Current SDK version doesn't support detach IFP flex counter from acl");
#endif
}

std::set<cfg::CounterType>
BcmIngressFieldProcessorFlexCounter::getCounterTypeList(
    int unit,
    uint32_t counterID) {
  std::set<cfg::CounterType> types;
#if defined(IS_OPENNSA) || defined(BCM_SDK_VERSION_GTE_6_5_20)
  bcm_flexctr_action_t action;
  bcm_flexctr_action_t_init(&action);
  auto rv = bcm_flexctr_action_get(unit, counterID, &action);
  bcmCheckError(rv, "Failed to get FlexCounter action for id:", counterID);
  // Check the index object needs to be IFP
  if (action.index_operation.object[0] !=
      bcmFlexctrObjectStaticIngFieldStageIngress) {
    throw FbossError("FlexCounter id:", counterID, " is not a IFP counter");
  }
  // Flex counter always have two operations
  for (auto* operation : {&action.operation_a, &action.operation_b}) {
    switch (operation->select) {
      case bcmFlexctrValueSelectCounterValue:
        types.insert(cfg::CounterType::PACKETS);
        break;
      case bcmFlexctrValueSelectPacketLength:
        types.insert(cfg::CounterType::BYTES);
        break;
      default:
        throw FbossError(
            "FlexCounter id:",
            counterID,
            " has unrecognized counter type:",
            operation->select);
    }
  }
#endif
  return types;
}

int BcmIngressFieldProcessorFlexCounter::getNumAclStatsInFpGroup(
    int unit,
    int gid) {
  BcmGetNumAclStatsCountUserData userData;
  userData.groupID = gid;
#if defined(IS_OPENNSA) || defined(BCM_SDK_VERSION_GTE_6_5_20)
  auto rv = bcm_flexctr_action_traverse(
      unit, &getNumAclStatsCounterCallback, static_cast<void*>(&userData));
  bcmCheckError(rv, "Failed to traverse flex counters for IFP id=", gid);
#endif
  return userData.count;
}

void BcmIngressFieldProcessorFlexCounter::removeAllCountersInFpGroup(
    [[maybe_unused]] int unit,
    [[maybe_unused]] int gid) {
#if defined(IS_OPENNSA) || defined(BCM_SDK_VERSION_GTE_6_5_20)
  int entryCount = 0;
  auto rv = bcm_field_entry_multi_get(unit, gid, 0, nullptr, &entryCount);
  // First find all the acl entry has flex counter attached
  if (rv == BCM_E_NONE && entryCount > 0) {
    std::vector<bcm_field_entry_t> bcmEntries(entryCount);
    rv = bcm_field_entry_multi_get(
        unit, gid, entryCount, bcmEntries.data(), &entryCount);
    bcmCheckError(rv, "Unable to get field entry information for group=", gid);
    for (auto bcmEntry : bcmEntries) {
      if (auto counterID = BcmIngressFieldProcessorFlexCounter::
              getFlexCounterIDFromAttachedAcl(unit, gid, bcmEntry)) {
        // detach the counter from acl
        BcmIngressFieldProcessorFlexCounter::detach(
            unit, bcmEntry, BcmAclStatHandle(*counterID));
      }
    }
  } else if (rv == BCM_E_NOT_FOUND || entryCount == 0) {
    // no-op
  } else {
    bcmCheckError(rv, "Unable to get count of field entry for group: ", gid);
  }

  // Make sure to remove detached IFP FlexCounter
  BcmGetNumAclStatsCountUserData userData;
  userData.groupID = gid;
  rv = bcm_flexctr_action_traverse(
      unit, &getNumAclStatsCounterCallback, static_cast<void*>(&userData));
  bcmCheckError(rv, "Failed to traverse flex counters for IFP id=", gid);
  for (auto counterID : userData.statCounterIDs) {
    BcmFlexCounter::destroy(unit, counterID);
  }
#endif
}

std::optional<uint32_t>
BcmIngressFieldProcessorFlexCounter::getFlexCounterIDFromAttachedAcl(
    int unit,
    int groupID,
    BcmAclEntryHandle acl) {
#if defined(IS_OPENNSA) || defined(BCM_SDK_VERSION_GTE_6_5_20)
  bcm_field_entry_config_t aclEntry;
  bcm_field_entry_config_t_init(&aclEntry);
  aclEntry.entry_id = acl;
  aclEntry.group_id = groupID;
  auto rv = bcm_field_entry_config_get(unit, &aclEntry);
  bcmCheckError(
      rv,
      "Failed to get acl entry config for group id=",
      groupID,
      ", acl id=",
      acl);

  return aclEntry.flex_counter_action_id <= 0
      ? std::nullopt
      : std::optional<uint32_t>{aclEntry.flex_counter_action_id};
#else
  throw FbossError(
      "Current SDK version doesn't support bcm_field_entry_config_get");
#endif
}

BcmTrafficCounterStats
BcmIngressFieldProcessorFlexCounter::getAclTrafficFlexCounterStats(
    [[maybe_unused]] int unit,
    [[maybe_unused]] BcmAclStatHandle handle,
    [[maybe_unused]] const std::vector<cfg::CounterType>& counters) {
  BcmTrafficCounterStats stats;
#if defined(IS_OPENNSA) || defined(BCM_SDK_VERSION_GTE_6_5_20)
  bcm_flexctr_counter_value_t values;
  memset(&values, 0, sizeof(values));
  // bcm_flexctr_stat_get needs to pass in a pointer for index
  uint32_t index = 1;
  // Each acl has one flex counter entry and index is 1
  auto rv = bcm_flexctr_stat_get(
      unit, handle, kNumDefaultIFPFlexCounterPerAcl, &index, &values);
  bcmCheckError(rv, "Failed to get IFP FlexCounter stats for id:", handle);
  for (auto counterType : counters) {
    // CounterType::PACKETS is always operation_a
    switch (counterType) {
      case cfg::CounterType::PACKETS:
        stats.emplace(cfg::CounterType::PACKETS, values.value[0]);
        break;
      case cfg::CounterType::BYTES:
        stats.emplace(cfg::CounterType::BYTES, values.value[1]);
        break;
    }
  }
#endif
  return stats;
}
} // namespace facebook::fboss
