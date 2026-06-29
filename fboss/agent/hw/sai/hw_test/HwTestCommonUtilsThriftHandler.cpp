// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/agent/hw/sai/switch/SaiSwitch.h"
#include "fboss/agent/hw/sai/switch/SaiVlanManager.h"
#include "fboss/agent/hw/test/HwTestThriftHandler.h"

namespace facebook {
namespace fboss {
namespace utility {

void HwTestThriftHandler::printDiagCmd(std::unique_ptr<::std::string> cmd) {
  auto saiSwitch = static_cast<const SaiSwitch*>(hwSwitch_);
  saiSwitch->printDiagCmd(*cmd);
}
void HwTestThriftHandler::getVlanToNumPorts(
    std::map<int32_t, int32_t>& vlanToNumPorts) {
  const auto& vlanMgr =
      static_cast<const SaiSwitch*>(hwSwitch_)->managerTable()->vlanManager();
  for (const auto& [vlanId, handle] : vlanMgr.getVlanHandles()) {
    vlanToNumPorts[static_cast<int32_t>(vlanId)] = handle->vlanMembers.size();
  }
}

} // namespace utility
} // namespace fboss
} // namespace facebook
