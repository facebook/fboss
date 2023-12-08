// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/agent/FabricConnectivityManager.h"
#include "fboss/agent/Utils.h"
#include "fboss/agent/state/DeltaFunctions.h"
#include "fboss/agent/state/DsfNode.h"
#include "fboss/agent/state/StateDelta.h"
#include "fboss/agent/state/SwitchState.h"

namespace facebook::fboss {

void FabricConnectivityManager::stateUpdated(const StateDelta& delta) {}

FabricEndpoint FabricConnectivityManager::processConnectivityInfoForPort(
    const PortID& portId,
    const FabricEndpoint& hwEndpoint) {
  return FabricEndpoint{};
}

bool FabricConnectivityManager::isConnectivityInfoMismatch(
    const PortID& portId) {
  return true;
}

bool FabricConnectivityManager::isConnectivityInfoMissing(
    const PortID& portId) {
  return false;
}

std::map<PortID, FabricEndpoint>
FabricConnectivityManager::getConnectivityInfo() {
  return {};
}

} // namespace facebook::fboss
