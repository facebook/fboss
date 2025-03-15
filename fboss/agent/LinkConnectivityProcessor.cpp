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

  // Returns whether the LED state was modified
  auto setPortLedState =
      [&out](PortID portId, PortLedExternalState desiredLedState) {
        auto curPort = out->getPorts()->getNode(portId);
        if (curPort->getLedPortExternalState() == desiredLedState) {
          return false;
        }
        auto newPort = curPort->modify(&out);
        newPort->setLedPortExternalState(desiredLedState);
        return true;
      };

  // Returns whether the port errors were modified
  auto setPortErrors =
      [&out](PortID portId, const std::set<PortError> updatedErrors) {
        auto port = out->getPorts()->getNodeIf(portId);
        auto activeErrorsVec = port->getActiveErrors();
        auto activeErrors =
            std::set(activeErrorsVec.begin(), activeErrorsVec.end());
        if (updatedErrors == activeErrors) {
          return false;
        }
        port->modify(&out)->setActiveErrors(updatedErrors);
        return true;
      };

  bool changed = false;
  for (const auto& [portId, connectivityDelta] : port2ConnectivityDelta) {
    auto port = in->getPorts()->getNodeIf(portId);
    if (!port) {
      XLOG(ERR) << "Got connectivity delta for unknown port: " << portId;
      continue;
    }
    XLOG(DBG2) << "Connectivity changed for port " << port->getName()
               << ". Delta: " << connectivityDelta;

    auto portErrors = out->getPorts()->getNodeIf(portId)->getActiveErrors();
    auto updatedPortErrors = std::set(portErrors.begin(), portErrors.end());
    auto newConnectivity = connectivityDelta.newConnectivity();
    if (newConnectivity.has_value() && *newConnectivity->isAttached()) {
      auto localBaseSwitchId = scopeResolver.scope(portId).switchId();
      auto localPortAsic = asicTable.getHwAsic(localBaseSwitchId);
      auto connectedSwitchId = SwitchID(*newConnectivity->switchId());

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
        changed |= setPortLedState(
            portId, PortLedExternalState::CABLING_ERROR_LOOP_DETECTED);
      } else if (FabricConnectivityManager::isConnectivityInfoMissing(
                     *newConnectivity)) {
        changed |= setPortLedState(portId, PortLedExternalState::CABLING_ERROR);
        updatedPortErrors.emplace(PortError::MISSING_EXPECTED_NEIGHBOR);
      } else if (FabricConnectivityManager::isConnectivityInfoMismatch(
                     *newConnectivity)) {
        changed |= setPortLedState(portId, PortLedExternalState::CABLING_ERROR);
        updatedPortErrors.emplace(PortError::MISMATCHED_NEIGHBOR);
      } else {
        updatedPortErrors.erase(PortError::MISMATCHED_NEIGHBOR);
        updatedPortErrors.erase(PortError::MISSING_EXPECTED_NEIGHBOR);
        changed |= setPortLedState(portId, PortLedExternalState::NONE);
      }
    } else {
      updatedPortErrors.erase(PortError::MISMATCHED_NEIGHBOR);
      updatedPortErrors.erase(PortError::MISSING_EXPECTED_NEIGHBOR);
      changed |= setPortLedState(portId, PortLedExternalState::NONE);
    }

    changed |= setPortErrors(portId, updatedPortErrors);
  }

  return changed ? out : std::shared_ptr<SwitchState>();
}
} // namespace facebook::fboss
