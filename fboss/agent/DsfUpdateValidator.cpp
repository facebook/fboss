// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/agent/DsfUpdateValidator.h"
#include "fboss/agent/DsfStateUpdaterUtil.h"
#include "fboss/agent/state/DeltaFunctions.h"
#include "fboss/agent/state/InterfaceMap.h"
#include "fboss/agent/state/StateDelta.h"
#include "fboss/agent/state/SwitchState.h"
#include "fboss/agent/state/SystemPortMap.h"

namespace facebook::fboss {

void DsfUpdateValidator::validate(
    const std::shared_ptr<SwitchState>& oldState,
    const std::shared_ptr<SwitchState>& newState) {
  StateDelta delta(oldState, newState);

  auto checkInRange = [&newState](const SystemPort& sysPort) {
    const auto dsfNode = newState->getDsfNodes()->getNodeIf(
        static_cast<int64_t>(sysPort.getSwitchId()));
    if (!dsfNode) {
      throw FbossError(
          "Switch ID: ",
          sysPort.getSwitchId(),
          " not found for : ",
          sysPort.getName());
    }
    auto sysPortRange = dsfNode->getSystemPortRange();
    if (!sysPortRange.has_value()) {
      throw FbossError(
          "No system port range for node ",
          dsfNode->getName(),
          " corresponding to ",
          sysPort.getName());
    }
    if (sysPort.getID() < *sysPortRange->minimum() ||
        sysPort.getID() > *sysPortRange->maximum()) {
      throw FbossError(
          "Sys port : ",
          sysPort.getName(),
          " belonging to: ",
          sysPort.getSwitchId(),
          " out of range: [",
          *sysPortRange->minimum(),
          ", ",
          *sysPortRange->maximum(),
          "]");
    }
  };
  DeltaFunctions::forEachChanged(
      delta.getRemoteSystemPortsDelta(),
      [&](const auto& oldSysPort, const auto& newSysPort) {
        if (oldSysPort->getSwitchId() != newSysPort->getSwitchId()) {
          throw FbossError("SwitchID change not permitted for sys ports");
        }
        checkInRange(*newSysPort);
      },
      [&](const auto& addedSysPort) {
        if (localSwitchIds_.find(addedSysPort->getSwitchId()) !=
            localSwitchIds_.end()) {
          throw FbossError(
              "Got sys port: ",
              addedSysPort->getName(),
              " with local switchId: ",
              addedSysPort->getSwitchId());
        }
        checkInRange(*addedSysPort);
      },
      [&](const auto& /*removedSysPort*/) {});
  DeltaFunctions::forEachChanged(
      delta.getRemoteIntfsDelta(),
      [&](const auto& /*oldIntf*/, const auto& /*newIntf*/) {},
      [&](const auto& addedIntf) {
        const auto& remoteSysPorts = newState->getRemoteSystemPorts();
        if (!remoteSysPorts->getNodeIf(SystemPortID(addedIntf->getID()))) {
          throw FbossError(
              "Adding a interface, without a corresponding sys port:",
              addedIntf->getID());
        }
      },
      [&](const auto& /*removedSysPort*/) {});
}
} // namespace facebook::fboss
