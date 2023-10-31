// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/agent/DsfStateUpdaterUtil.h"

#include "fboss/agent/state/StateDelta.h"

namespace facebook::fboss {

std::shared_ptr<SwitchState> DsfStateUpdaterUtil::getUpdatedState(
    const std::shared_ptr<SwitchState>& in,
    const SwitchIdScopeResolver* scopeResolver,
    const std::shared_ptr<SystemPortMap>& newSysPorts,
    const std::shared_ptr<InterfaceMap>& newRifs,
    const std::string& nodeName,
    const SwitchID& nodeSwitchId) {
  // TODO
  return std::shared_ptr<SwitchState>{};
}

} // namespace facebook::fboss
