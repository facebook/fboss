/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/hw/bcm/BcmExactMatchUtils.h"
#include "fboss/agent/hw/bcm/BcmSwitch.h"
#include "fboss/agent/hw/bcm/BcmTeFlowTable.h"
#include "fboss/agent/hw/test/HwTestThriftHandler.h"

namespace facebook::fboss::utility {

int32_t HwTestThriftHandler::getNumTeFlowEntries() {
  const auto bcmSwitch = static_cast<const BcmSwitch*>(hwSwitch_);
  int gid = bcmSwitch->getPlatform()->getAsic()->getDefaultTeFlowGroupID();
  int size = 0;
  auto rv = bcm_field_entry_multi_get(
      bcmSwitch->getUnit(), getEMGroupID(gid), 0, nullptr, &size);
  if (rv < 0) {
    XLOG(ERR) << "Unable to get TE flow entries";
  }

  return size;
}
bool HwTestThriftHandler::checkSwHwTeFlowMatch(
    std::unique_ptr<::facebook::fboss::state::TeFlowEntryFields>
        flowEntryFields) {
  const auto bcmSwitch = static_cast<const BcmSwitch*>(hwSwitch_);

  const auto flowEntry = std::make_shared<TeFlowEntry>(*flowEntryFields);

  int gid = bcmSwitch->getPlatform()->getAsic()->getDefaultTeFlowGroupID();

  auto hwTeFlows = bcmSwitch->getTeFlowTable()->getTeFlowIf(flowEntry);
  if (hwTeFlows == nullptr) {
    return false;
  }

  return BcmTeFlowEntry::isStateSame(
      bcmSwitch, getEMGroupID(gid), hwTeFlows->getHandle(), flowEntry);
}

} // namespace facebook::fboss::utility
