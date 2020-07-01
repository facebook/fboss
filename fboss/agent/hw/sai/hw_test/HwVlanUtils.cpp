/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/hw/test/HwVlanUtils.h"
#include "fboss/agent/hw/sai/switch/SaiManagerTable.h"
#include "fboss/agent/hw/sai/switch/SaiSwitch.h"
#include "fboss/agent/hw/sai/switch/SaiVlanManager.h"

#include <folly/gen/Base.h>

namespace facebook::fboss {

std::vector<VlanID> getConfiguredVlans(const HwSwitch* hwSwitch) {
  const auto& vlanMgr =
      static_cast<const SaiSwitch*>(hwSwitch)->managerTable()->vlanManager();

  return folly::gen::from(vlanMgr.getVlanHandles()) |
      folly::gen::map([](const auto& vlanIdAndHandle) {
           return vlanIdAndHandle.first;
         }) |
      folly::gen::as<std::vector<VlanID>>();
}

std::map<VlanID, uint32_t> getVlanToNumPorts(const HwSwitch* hwSwitch) {
  std::map<VlanID, uint32_t> vlan2NumPorts;
  const auto& vlanMgr =
      static_cast<const SaiSwitch*>(hwSwitch)->managerTable()->vlanManager();
  for (const auto& [vlanId, handle] : vlanMgr.getVlanHandles()) {
    vlan2NumPorts[vlanId] = handle->vlanMembers.size();
  }
  return vlan2NumPorts;
}

} // namespace facebook::fboss
