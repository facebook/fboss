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
#include "fboss/agent/HwAsicTable.h"
#include "fboss/agent/SwitchIdScopeResolver.h"
#include "fboss/agent/state/SwitchState.h"

namespace facebook::fboss {

std::shared_ptr<SwitchState> LinkConnectivityProcessor::process(
    const SwitchIdScopeResolver& scopeResolver,
    const HwAsicTable& asicTable,
    const std::shared_ptr<SwitchState>& in,
    const std::map<PortID, multiswitch::FabricConnectivityDelta>&
        port2ConnectivityDelta) {
  auto out = in->clone();
  bool changed = false;
  auto setPortLedState =
      [&out, &changed](PortID portId, PortLedExternalState desiredLedState) {
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
    XLOG(DBG2) << " Connectivity changed for  port : " << port->getName()
               << " Delta: " << connectivityDelta;
    auto newConnectivity = connectivityDelta.newConnectivity();
    if (newConnectivity.has_value()) {
      if (!*newConnectivity->isAttached()) {
        // Not connected (port maybe down). Retain whatever cabling
        // error state we had from before. When we get connectivity info
        // again, we will determine cabling states based on that.
        continue;
      }
      auto connectedSwitchId = SwitchID(*newConnectivity->switchId());
      auto localBaseSwitchId = scopeResolver.scope(portId).switchId();
      auto localPortAsic = asicTable.getHwAsic(localBaseSwitchId);
      if (connectedSwitchId >= localBaseSwitchId &&
          connectedSwitchId < SwitchID(
                                  static_cast<int>(localBaseSwitchId) +
                                  localPortAsic->getVirtualDevices())) {
        // Loop detected - we do this regardless of expected connectivity.
        // On DSF its pretty risky to have this condition set on fabric
        // switches. For undrained ports it can hose a % of traffic
        // going through that R3. With cell spraying this impact
        // can be quite wide. So can't risk config always getting this
        // righte
        setPortLedState(
            portId, PortLedExternalState::CABLING_ERROR_LOOP_DETECTED);
      } else if (FabricConnectivityManager::isConnectivityInfoMismatch(
                     *newConnectivity)) {
        // Fabric connectivity mismatched.
        setPortLedState(portId, PortLedExternalState::CABLING_ERROR);
      } else {
        setPortLedState(portId, PortLedExternalState::NONE);
      }
    } else {
      setPortLedState(portId, PortLedExternalState::NONE);
    }
  }
  return changed ? out : std::shared_ptr<SwitchState>();
}
} // namespace facebook::fboss
