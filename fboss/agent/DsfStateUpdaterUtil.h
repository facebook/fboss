// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once

#include "fboss/agent/SwitchIdScopeResolver.h"
#include "fboss/agent/state/InterfaceMap.h"
#include "fboss/agent/state/SwitchState.h"
#include "fboss/agent/state/SystemPortMap.h"

namespace facebook::fboss {

class RoutingInformationBase;

class DsfStateUpdaterUtil {
 public:
  static std::shared_ptr<SwitchState> getUpdatedState(
      const std::shared_ptr<SwitchState>& in,
      const SwitchIdScopeResolver* scopeResolver,
      RoutingInformationBase* rib,
      const std::map<SwitchID, std::shared_ptr<SystemPortMap>>&
          switchId2SystemPorts,
      const std::map<SwitchID, std::shared_ptr<InterfaceMap>>& switchId2Intfs);
  template <typename TableT>
  static void updateNeighborEntry(
      const TableT& oldTable,
      const TableT& clonedTable);
};

} // namespace facebook::fboss
