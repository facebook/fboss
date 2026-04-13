// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/agent/hw/bcm/BcmError.h"
#include "fboss/agent/hw/bcm/BcmSwitch.h"
#include "fboss/agent/hw/test/HwTestThriftHandler.h"

namespace facebook {
namespace fboss {
namespace utility {
void HwTestThriftHandler::printDiagCmd(std::unique_ptr<::std::string> cmd) {
  auto bcmSwitch = static_cast<const BcmSwitch*>(hwSwitch_);
  bcmSwitch->printDiagCmd(*cmd);
}
void HwTestThriftHandler::getVlanToNumPorts(
    std::map<int32_t, int32_t>& vlanToNumPorts) {
  auto bcmSwitch = static_cast<const BcmSwitch*>(hwSwitch_);
  auto unit = bcmSwitch->getUnit();
  bcm_vlan_data_t* vlanList = nullptr;
  int vlanCount = 0;
  SCOPE_EXIT {
    bcm_vlan_list_destroy(unit, vlanList, vlanCount);
  };
  auto rv = bcm_vlan_list(unit, &vlanList, &vlanCount);
  bcmCheckError(rv, "failed to list all VLANs");
  for (int i = 0; i < vlanCount; i++) {
    int portCount;
    BCM_PBMP_COUNT(vlanList[i].port_bitmap, portCount);
    vlanToNumPorts[static_cast<int32_t>(vlanList[i].vlan_tag)] = portCount;
  }
}

} // namespace utility
} // namespace fboss
} // namespace facebook
