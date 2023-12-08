// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once

#include <memory>
#include "fboss/agent/if/gen-cpp2/FbossCtrl.h"
#include "fboss/agent/state/DsfNode.h"
#include "fboss/agent/state/StateDelta.h"
#include "fboss/agent/types.h"

namespace facebook::fboss {
class SwitchState;
class PlatformMapping;

class FabricReachabilityManager {
 public:
  explicit FabricReachabilityManager() {}
  void stateUpdated(const StateDelta& stateDelta);
  std::map<PortID, FabricEndpoint> getReachabilityInfo();
  FabricEndpoint processReachabilityInfoForPort(
      const PortID& portId,
      const FabricEndpoint& hwReachability);
  bool isReachabilityInfoMissing(const PortID& portId);
  bool isReachabilityInfoMismatch(const PortID& portId);
};
} // namespace facebook::fboss
