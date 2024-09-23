// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/agent/DsfUpdateValidator.h"
#include "fboss/agent/DsfStateUpdaterUtil.h"
#include "fboss/agent/SwSwitch.h"
#include "fboss/agent/state/InterfaceMap.h"
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
    const std::shared_ptr<SwitchState>& /*oldState*/,
    const std::shared_ptr<SwitchState>& /*newState*/) {
  // TODO
}
} // namespace facebook::fboss
