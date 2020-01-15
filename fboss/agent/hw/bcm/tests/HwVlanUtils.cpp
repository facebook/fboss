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

#include "fboss/agent/hw/bcm/BcmError.h"
#include "fboss/agent/hw/bcm/BcmSwitch.h"
#include "fboss/agent/types.h"

#include <vector>

extern "C" {
#include <bcm/port.h>
}
namespace {
std::vector<bcm_vlan_data_t> getVlans(
    const facebook::fboss::HwSwitch* hwSwitch) {
  auto unit =
      static_cast<const facebook::fboss::BcmSwitch*>(hwSwitch)->getUnit();
  bcm_vlan_data_t* vlanList = nullptr;
  int vlanCount = 0;
  SCOPE_EXIT {
    bcm_vlan_list_destroy(unit, vlanList, vlanCount);
  };
  auto rv = bcm_vlan_list(unit, &vlanList, &vlanCount);
  facebook::fboss::bcmCheckError(rv, "failed to list all VLANs");
  return {vlanList, vlanList + vlanCount};
}
} // namespace
namespace facebook::fboss {

std::vector<VlanID> getConfiguredVlans(const HwSwitch* hwSwitch) {
  auto hwVlans = getVlans(hwSwitch);
  std::vector<VlanID> vlans;
  std::for_each(hwVlans.begin(), hwVlans.end(), [&vlans](const auto& vlanData) {
    vlans.emplace_back(vlanData.vlan_tag);
  });
  return vlans;
}

std::map<VlanID, uint32_t> getVlanToNumPorts(const HwSwitch* hwSwitch) {
  std::map<VlanID, uint32_t> vlan2NumPorts;
  for (const auto& vlanData : getVlans(hwSwitch)) {
    int port_count;
    BCM_PBMP_COUNT(vlanData.port_bitmap, port_count);
    vlan2NumPorts.emplace(vlanData.vlan_tag, port_count);
  }
  return vlan2NumPorts;
}

} // namespace facebook::fboss
