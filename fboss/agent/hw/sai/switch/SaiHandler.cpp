/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/hw/sai/switch/SaiHandler.h"

#include "fboss/agent/ThriftHandlerUtils.h"
#include "fboss/agent/hw/sai/switch/SaiSwitch.h"

#include <folly/logging/xlog.h>
#include "fboss/lib/LogThriftCall.h"

namespace facebook::fboss {

SaiHandler::SaiHandler(SaiSwitch* hw)
    : hw_(hw), diagShell_(hw), diagCmdServer_(hw, &diagShell_) {}

SaiHandler::~SaiHandler() {}

apache::thrift::ResponseAndServerStream<std::string, std::string>
SaiHandler::startDiagShell() {
  XLOG(DBG2) << "New diag shell session connecting";
  if (!hw_->isFullyInitialized()) {
    throw FbossError("switch is still initializing or is exiting ");
  }
  diagShell_.tryConnect();
  auto streamAndPublisher =
      apache::thrift::ServerStream<std::string>::createPublisher([this]() {
        diagShell_.markResetPublisher();
        XLOG(DBG2) << "Diag shell session disconnected";
      });

  std::string firstPrompt =
      diagShell_.start(std::move(streamAndPublisher.second));
  return {firstPrompt, std::move(streamAndPublisher.first)};
}

void SaiHandler::produceDiagShellInput(
    std::unique_ptr<std::string> input,
    std::unique_ptr<ClientInformation> client) {
  diagShell_.consumeInput(std::move(input), std::move(client));
}

void SaiHandler::diagCmd(
    fbstring& result,
    std::unique_ptr<fbstring> cmd,
    std::unique_ptr<ClientInformation> client,
    int16_t /* unused */,
    bool /* unused */) {
  auto log = LOG_THRIFT_CALL(WARN, *cmd);
  hw_->ensureConfigured(__func__);
  {
    const std::lock_guard<std::mutex> lock(diagCmdLock_);
    result = diagCmdServer_.diagCmd(std::move(cmd), std::move(client));
  }
}

SwitchRunState SaiHandler::getHwSwitchRunState() {
  auto log = LOG_THRIFT_CALL(DBG1);
  return hw_->getRunState();
}

void SaiHandler::getHwFabricReachability(
    std::map<::std::int64_t, ::facebook::fboss::FabricEndpoint>& reachability) {
  auto log = LOG_THRIFT_CALL(DBG1);
  hw_->ensureVoqOrFabric(__func__);
  auto reachabilityInfo = hw_->getFabricConnectivity();
  for (auto&& entry : reachabilityInfo) {
    reachability.insert(std::move(entry));
  }
}

void SaiHandler::getHwFabricConnectivity(
    std::map<::std::string, ::facebook::fboss::FabricEndpoint>& connectivity) {
  auto log = LOG_THRIFT_CALL(DBG1);
  hw_->ensureVoqOrFabric(__func__);
  auto connectivityInfo = hw_->getFabricConnectivity();
  for (auto& entry : connectivityInfo) {
    auto port = hw_->getProgrammedState()->getPorts()->getNodeIf(entry.first);
    if (port) {
      connectivity.insert({port->getName(), entry.second});
    }
  }
}

void SaiHandler::getHwSwitchReachability(
    std::map<::std::string, std::vector<::std::string>>& reachability,
    std::unique_ptr<::std::vector<::std::string>> switchNames) {
  auto log = LOG_THRIFT_CALL(DBG1, *switchNames);
  hw_->ensureVoqOrFabric(__func__);
  if (switchNames->empty()) {
    throw FbossError("Empty switch name list input for getSwitchReachability.");
  }
  std::unordered_set<std::string> switchNameSet{
      switchNames->begin(), switchNames->end()};

  auto state = hw_->getProgrammedState();
  for (const auto& [_, dsfNodes] : std::as_const(*state->getDsfNodes())) {
    for (const auto& [_, node] : std::as_const(*dsfNodes)) {
      if (std::find(
              switchNameSet.begin(), switchNameSet.end(), node->getName()) !=
          switchNameSet.end()) {
        std::vector<std::string> reachablePorts;
        for (const auto& portId :
             hw_->getSwitchReachability(node->getSwitchId())) {
          auto port = state->getPorts()->getNodeIf(portId);
          if (port) {
            reachablePorts.push_back(port->getName());
          } else {
            XLOG(ERR) << "Port " << portId
                      << " in reachability matrix is not found in state";
          }
        }
        reachability.insert({node->getName(), std::move(reachablePorts)});
      }
    }
  }
  return;
}

void SaiHandler::clearHwPortStats(std::unique_ptr<std::vector<int32_t>> ports) {
  auto log = LOG_THRIFT_CALL(DBG1, *ports);
  hw_->ensureConfigured(__func__);
  utility::clearPortStats(hw_, std::move(ports), hw_->getProgrammedState());
}

void SaiHandler::clearAllHwPortStats() {
  auto log = LOG_THRIFT_CALL(DBG1);
  hw_->ensureConfigured(__func__);
  auto allPorts = std::make_unique<std::vector<int32_t>>();
  std::shared_ptr<SwitchState> swState = hw_->getProgrammedState();
  for (const auto& portMap : std::as_const(*(swState->getPorts()))) {
    for (const auto& port : std::as_const(*portMap.second)) {
      allPorts->push_back(port.second->getID());
    }
  }
  clearHwPortStats(std::move(allPorts));
}

void SaiHandler::getHwL2Table(std::vector<L2EntryThrift>& l2Table) {
  auto log = LOG_THRIFT_CALL(DBG1);
  hw_->ensureConfigured(__func__);
  hw_->fetchL2Table(&l2Table);
}

void SaiHandler::getVirtualDeviceToConnectionGroups(
    std::map<
        int64_t,
        std::map<int64_t, std::vector<facebook::fboss::RemoteEndpoint>>>&
        virtualDevice2ConnectionGroups) {
  auto log = LOG_THRIFT_CALL(DBG1);
  auto deviceToConnectionGroups =
      hw_->getVirtualDeviceToRemoteConnectionGroups();
  // Convert to format required by thrift handler
  for (const auto& [vid, connGroups] : deviceToConnectionGroups) {
    auto& outVidConnGroups = virtualDevice2ConnectionGroups[vid];
    for (const auto& [groupSize, group] : connGroups) {
      auto& outVidGroup = outVidConnGroups[groupSize];
      std::for_each(
          group.begin(),
          group.end(),
          [&outVidGroup](const auto& remoteEndpoint) {
            outVidGroup.push_back(remoteEndpoint);
          });
    }
  }
}

void SaiHandler::listHwObjects(
    std::string& out,
    std::unique_ptr<std::vector<HwObjectType>> hwObjects,
    bool cached) {
  auto log = LOG_THRIFT_CALL(DBG1);
  hw_->ensureConfigured(__func__);
  out = hw_->listObjects(*hwObjects, cached);
}

BootType SaiHandler::getBootType() {
  auto log = LOG_THRIFT_CALL(DBG1);
  hw_->ensureConfigured(__func__);
  return hw_->getBootType();
}

} // namespace facebook::fboss
