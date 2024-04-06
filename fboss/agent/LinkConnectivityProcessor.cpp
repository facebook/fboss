/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/LinkConnectivityProcessor.h"
#include <memory>
#include "fboss/agent/FabricConnectivityManager.h"
#include "fboss/agent/SwitchIdScopeResolver.h"
#include "fboss/agent/state/SwitchState.h"

namespace facebook::fboss {

std::shared_ptr<SwitchState> LinkConnectivityProcessor::process(
    const SwitchIdScopeResolver& scopeResolver,
    const std::shared_ptr<SwitchState>& in,
    const std::map<PortID, multiswitch::FabricConnectivityDelta>&
        port2ConnectivityDelta) {
  auto out = in->clone();
  bool changed = false;
  auto setPortState = [&out, &changed](
                          PortID portId, PortLedExternalState desiredLedState) {
    auto curPort = out->getPorts()->getNode(portId);
    if (curPort->getLedPortExternalState() == desiredLedState) {
      return;
    }
    auto newPort = curPort->modify(&out);
    newPort->setLedPortExternalState(desiredLedState);
    changed = true;
  };
  for (const auto& [portId, connectivityDelta] : port2ConnectivityDelta) {
    auto port = in->getPorts()->getNodeIf(portId);
    if (!port) {
      XLOG(ERR) << " Got connectivity delta for unknown port: " << portId;
      continue;
    }
    XLOG(DBG2) << " Connectivity changed for  port : " << port->getName();
    auto newConnectivity = connectivityDelta.newConnectivity();
    if (newConnectivity.has_value()) {
      auto localSwitchId = scopeResolver.scope(portId).switchId();
      if (localSwitchId == SwitchID(*newConnectivity->switchId())) {
        // Loop detected - we do this regardless of expected connectivity.
        // On DSF its pretty risky to have this condition set on fabric
        // switches. For undrained ports it can hose a % of traffic
        // going through that R3. With cell spraying this impact
        // can be quite wide. So can't risk config always getting this
        // righte
        setPortState(portId, PortLedExternalState::CABLING_ERROR_LOOP_DETECTED);
      } else if (FabricConnectivityManager::isConnectivityInfoMismatch(
                     *newConnectivity)) {
        // Fabric connectivity mismatched.
        setPortState(portId, PortLedExternalState::CABLING_ERROR);
      } else {
        setPortState(portId, PortLedExternalState::NONE);
      }
    } else {
      setPortState(portId, PortLedExternalState::NONE);
    }
  }
  return changed ? out : std::shared_ptr<SwitchState>();
}
} // namespace facebook::fboss