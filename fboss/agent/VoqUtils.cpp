/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/VoqUtils.h"
#include "fboss/agent/AgentFeatures.h"
#include "fboss/agent/state/SwitchState.h"

namespace facebook::fboss {

int getLocalPortNumVoqs(cfg::PortType portType, cfg::Scope portScope) {
  if (!isDualStage3Q2QMode()) {
    return 8;
  }
  if (FLAGS_dual_stage_edsw_3q_2q) {
    return 3;
  }
  CHECK(FLAGS_dual_stage_rdsw_3q_2q);
  if (portType == cfg::PortType::MANAGEMENT_PORT ||
      (portType == cfg::PortType::RECYCLE_PORT &&
       portScope == cfg::Scope::GLOBAL)) {
    return 2;
  }
  return 3;
}

int getRemotePortNumVoqs(
    HwAsic::InterfaceNodeRole intfRole,
    cfg::PortType portType) {
  if (intfRole == HwAsic::InterfaceNodeRole::DUAL_STAGE_EDGE_NODE) {
    CHECK(isDualStage3Q2QMode());
    return 3;
  }
  if (isDualStage3Q2QMode()) {
    if (portType == cfg::PortType::MANAGEMENT_PORT ||
        portType == cfg::PortType::RECYCLE_PORT) {
      return 2;
    }
    return 3;
  }
  return 8;
}

void processRemoteInterfaceRoutes(
    const std::shared_ptr<Interface>& remoteIntf,
    const std::shared_ptr<SwitchState>& state,
    bool add,
    IntfRouteTable& remoteIntfRoutesToAdd,
    RouterIDToPrefixes& remoteIntfRoutesToDel) {
  // On the same box, interfaces of both npu0 and npu1 will be added
  // as local interface. They will be used for route resolution as directly
  // connected routes. Hence no need to process as remote interfaces.
  if (state->getInterfaces()->getNodeIf(remoteIntf->getID())) {
    return;
  }

  for (const auto& [addr, mask] : std::as_const(*remoteIntf->getAddresses())) {
    const auto ipAddr = folly::IPAddress(addr);
    // Skip link-local addresses in directly-connected routes
    if (ipAddr.isV6() && ipAddr.isLinkLocal()) {
      continue;
    }
    auto prefix = folly::IPAddress::createNetwork(
        folly::to<std::string>(addr, "/", static_cast<int>(mask->cref())));
    IntfAddress intfAddr = std::make_pair(remoteIntf->getID(), ipAddr);
    if (add) {
      // Check if route already in remoteIntfRoutesToDel
      auto& toDel = remoteIntfRoutesToDel[remoteIntf->getRouterID()];
      auto iter = std::find(toDel.begin(), toDel.end(), prefix);
      if (iter != toDel.end()) {
        toDel.erase(iter);
      } else {
        remoteIntfRoutesToAdd[remoteIntf->getRouterID()].emplace(
            prefix, intfAddr);
      }
    } else {
      // Check if route already in remoteIntfRoutesToAdd
      auto& toAdd = remoteIntfRoutesToAdd[remoteIntf->getRouterID()];
      auto iter = toAdd.find(prefix);
      if (iter != toAdd.end()) {
        toAdd.erase(iter);
      } else {
        remoteIntfRoutesToDel[remoteIntf->getRouterID()].push_back(prefix);
      }
    }
  }
}
} // namespace facebook::fboss
