// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/agent/DsfUpdateValidator.h"
#include "fboss/agent/DsfStateUpdaterUtil.h"
#include "fboss/agent/SwSwitch.h"
#include "fboss/agent/state/DeltaFunctions.h"
#include "fboss/agent/state/InterfaceMap.h"
#include "fboss/agent/state/StateDelta.h"
#include "fboss/agent/state/SwitchState.h"
#include "fboss/agent/state/SystemPortMap.h"

namespace facebook::fboss {

DsfUpdateValidator::~DsfUpdateValidator() {}

std::shared_ptr<SwitchState> DsfUpdateValidator::validateAndGetUpdate(
    const std::shared_ptr<SwitchState>& in,
    const std::map<SwitchID, std::shared_ptr<SystemPortMap>>&
        switchId2SystemPorts,
    const std::map<SwitchID, std::shared_ptr<InterfaceMap>>& switchId2Intfs) {
  auto out = DsfStateUpdaterUtil::getUpdatedState(
      in,
      sw_->getScopeResolver(),
      sw_->getRib(),
      switchId2SystemPorts,
      switchId2Intfs);
  validate(in, out);
  return out;
}

void DsfUpdateValidator::validate(
    const std::shared_ptr<SwitchState>& oldState,
    const std::shared_ptr<SwitchState>& newState) {
  StateDelta delta(oldState, newState);

  DeltaFunctions::forEachChanged(
      delta.getRemoteSystemPortsDelta(),
      [&](const auto& oldSysPort, const auto& newSysPort) {
        if (oldSysPort->getSwitchId() != newSysPort->getSwitchId()) {
          throw FbossError("SwitchID change not permitted for sys ports");
        }
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
