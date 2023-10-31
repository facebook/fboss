// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once

#include "fboss/agent/SwitchIdScopeResolver.h"
#include "fboss/agent/state/InterfaceMap.h"
#include "fboss/agent/state/SwitchState.h"
#include "fboss/agent/state/SystemPortMap.h"

namespace facebook::fboss {

class DsfStateUpdaterUtil {
 public:
  static std::shared_ptr<SwitchState> getUpdatedState(
      const std::shared_ptr<SwitchState>& in,
      const SwitchIdScopeResolver* scopeResolver,
      const std::shared_ptr<SystemPortMap>& newSysPorts,
      const std::shared_ptr<InterfaceMap>& newRifs,
      const std::string& nodeName,
      const SwitchID& nodeSwitchId);
};

} // namespace facebook::fboss
