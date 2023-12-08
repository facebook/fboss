// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/agent/FabricConnectivityManager.h"
#include "fboss/agent/Utils.h"
#include "fboss/agent/state/DeltaFunctions.h"
#include "fboss/agent/state/DsfNode.h"
#include "fboss/agent/state/StateDelta.h"
#include "fboss/agent/state/SwitchState.h"

namespace facebook::fboss {

void FabricReachabilityManager::stateUpdated(const StateDelta& delta) {}

FabricEndpoint FabricReachabilityManager::processReachabilityInfoForPort(
    const PortID& portId,
    const FabricEndpoint& hwEndpoint) {
  return FabricEndpoint{};
}

bool FabricReachabilityManager::isReachabilityInfoMismatch(
    const PortID& portId) {
  return true;
}

bool FabricReachabilityManager::isReachabilityInfoMissing(
    const PortID& portId) {
  return false;
}

std::map<PortID, FabricEndpoint>
FabricReachabilityManager::getReachabilityInfo() {
  return {};
}

} // namespace facebook::fboss
