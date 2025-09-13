/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/lib/multinode/MultiNodeUtil.h"

#include <thrift/lib/cpp2/async/RocketClientChannel.h>
#include "common/network/NetworkUtil.h"

#include "fboss/agent/if/gen-cpp2/FbossHwCtrl.h"
#include "fboss/agent/if/gen-cpp2/MultiNodeTestCtrlAsyncClient.h"

#include "fboss/agent/Utils.h"
#include "fboss/lib/CommonUtils.h"

namespace {
using facebook::fboss::FbossHwCtrl;
using facebook::fboss::MultiNodeTestCtrl;
using facebook::fboss::MultiSwitchRunState;
using RunForHwAgentFn = std::function<void(
    apache::thrift::Client<facebook::fboss::FbossHwCtrl>& client)>;

std::unique_ptr<apache::thrift::Client<MultiNodeTestCtrl>>
getSwAgentThriftClient(const std::string& switchName) {
  folly::EventBase* eb = folly::EventBaseManager::get()->getEventBase();
  auto remoteSwitchIp =
      facebook::network::NetworkUtil::getHostByName(switchName);
  folly::SocketAddress agent(remoteSwitchIp, 5909);
  auto socket = folly::AsyncSocket::newSocket(eb, agent);
  auto channel =
      apache::thrift::RocketClientChannel::newChannel(std::move(socket));
  return std::make_unique<apache::thrift::Client<MultiNodeTestCtrl>>(
      std::move(channel));
}

std::unique_ptr<apache::thrift::Client<FbossHwCtrl>> getHwAgentThriftClient(
    const std::string& switchName,
    int port) {
  folly::EventBase* eb = folly::EventBaseManager::get()->getEventBase();
  auto remoteSwitchIp =
      facebook::network::NetworkUtil::getHostByName(switchName);
  folly::SocketAddress agent(remoteSwitchIp, port);
  auto socket = folly::AsyncSocket::newSocket(eb, agent);
  auto channel =
      apache::thrift::RocketClientChannel::newChannel(std::move(socket));
  return std::make_unique<apache::thrift::Client<FbossHwCtrl>>(
      std::move(channel));
}

MultiSwitchRunState getMultiSwitchRunState(const std::string& switchName) {
  auto swAgentClient = getSwAgentThriftClient(switchName);
  MultiSwitchRunState runState;
  swAgentClient->sync_getMultiSwitchRunState(runState);
  return runState;
}

int getNumHwSwitches(const std::string& switchName) {
  auto runState = getMultiSwitchRunState(switchName);
  return runState.hwIndexToRunState()->size();
}

void runOnAllHwAgents(
    const std::string& switchName,
    const RunForHwAgentFn& fn) {
  static const std::vector kHwAgentPorts = {5931, 5932};
  auto numHwSwitches = getNumHwSwitches(switchName);
  for (int i = 0; i < numHwSwitches; i++) {
    auto hwAgentClient = getHwAgentThriftClient(switchName, kHwAgentPorts[i]);
    fn(*hwAgentClient);
  }
}

void adminDisablePort(const std::string& switchName, int32_t portID) {
  auto swAgentClient = getSwAgentThriftClient(switchName);
  swAgentClient->sync_setPortState(portID, false /* disable port */);
}

void adminEnablePort(const std::string& switchName, int32_t portID) {
  auto swAgentClient = getSwAgentThriftClient(switchName);
  swAgentClient->sync_setPortState(portID, true /* enable port */);
}

void triggerGracefulAgentRestart(const std::string& switchName) {
  try {
    auto swAgentClient = getSwAgentThriftClient(switchName);
    swAgentClient->sync_triggerGracefulExit();
  } catch (...) {
    // Thrift request may throw error as the Agent exits.
    // Ignore it, as we only wanted to trigger exit.
  }
}

} // namespace

namespace facebook::fboss::utility {

MultiNodeUtil::MultiNodeUtil(
    const std::shared_ptr<MultiSwitchDsfNodeMap>& dsfNodeMap) {
  populateDsfNodes(dsfNodeMap);
  populateAllRdsws();
}

void MultiNodeUtil::populateDsfNodes(
    const std::shared_ptr<MultiSwitchDsfNodeMap>& dsfNodeMap) {
  for (const auto& [_, dsfNodes] : std::as_const(*dsfNodeMap)) {
    for (const auto& [_, node] : std::as_const(*dsfNodes)) {
      switchIdToSwitchName_[node->getSwitchId()] = node->getName();

      if (node->getType() == cfg::DsfNodeType::INTERFACE_NODE) {
        CHECK(node->getClusterId().has_value());
        auto clusterId = node->getClusterId().value();
        clusterIdToRdsws_[clusterId].push_back(node->getName());
      } else if (node->getType() == cfg::DsfNodeType::FABRIC_NODE) {
        CHECK(node->getFabricLevel().has_value());
        if (node->getFabricLevel().value() == 1) {
          CHECK(node->getClusterId().has_value());
          auto clusterId = node->getClusterId().value();
          clusterIdToFdsws_[clusterId].push_back(node->getName());
        } else if (node->getFabricLevel().value() == 2) {
          sdsws_.insert(node->getName());
        } else {
          XLOG(FATAL) << "Invalid fabric level"
                      << node->getFabricLevel().value();
        }
      } else {
        XLOG(FATAL) << "Invalid DSF Node type"
                    << apache::thrift::util::enumNameSafe(node->getType());
      }
    }
  }
}

void MultiNodeUtil::populateAllRdsws() {
  for (const auto& [clusterId, rdsws] : std::as_const(clusterIdToRdsws_)) {
    for (const auto& rdsw : rdsws) {
      allRdsws_.insert(rdsw);
    }
  }
}

void MultiNodeUtil::populateAllFdsws() {
  for (const auto& [clusterId, fdsws] : std::as_const(clusterIdToFdsws_)) {
    for (const auto& fdsw : fdsws) {
      allFdsws_.insert(fdsw);
    }
  }
}

std::map<std::string, FabricEndpoint> MultiNodeUtil::getFabricEndpoints(
    const std::string& switchName) {
  std::map<std::string, FabricEndpoint> fabricEndpoints;
  auto hwAgentQueryFn =
      [&fabricEndpoints](
          apache::thrift::Client<facebook::fboss::FbossHwCtrl>& client) {
        std::map<std::string, FabricEndpoint> hwagentEntries;
        client.sync_getHwFabricConnectivity(hwagentEntries);
        fabricEndpoints.merge(hwagentEntries);
      };
  runOnAllHwAgents(switchName, hwAgentQueryFn);

  return fabricEndpoints;
}

std::set<std::string> MultiNodeUtil::getConnectedFabricPorts(
    const std::string& switchName) {
  auto fabricEndpoints = getFabricEndpoints(switchName);

  std::set<std::string> connectedPorts;
  for (const auto& [localPort, fabricEndpoint] : fabricEndpoints) {
    if (fabricEndpoint.isAttached().value()) {
      connectedPorts.insert(localPort);
    }
  }

  return connectedPorts;
}

bool MultiNodeUtil::verifyFabricConnectedSwitchesHelper(
    SwitchType switchType,
    const std::string& switchToVerify,
    const std::set<std::string>& expectedConnectedSwitches) {
  auto logFabricEndpoint = [switchToVerify](
                               const FabricEndpoint& fabricEndpoint) {
    XLOG(DBG2) << "From " << " switchName: " << switchToVerify
               << " actualPeerSwitchId: " << fabricEndpoint.switchId().value()
               << " actualPeerSwitchName: "
               << fabricEndpoint.switchName().value_or("none")
               << " actualPeerPortId: " << fabricEndpoint.portId().value()
               << " actualPortName: "
               << fabricEndpoint.portName().value_or("none")
               << " expectedPeerSwitchId: "
               << fabricEndpoint.expectedSwitchId().value_or(-1)
               << " expectedPeerSwitchName: "
               << fabricEndpoint.expectedSwitchName().value_or("none")
               << " expectedPeerPortId: "
               << fabricEndpoint.expectedPortId().value_or(-1)
               << " expectedPeerPortName: "
               << fabricEndpoint.expectedPortName().value_or("none");
  };

  auto fabricEndpoints = getFabricEndpoints(switchToVerify);

  std::set<std::string> gotConnectedSwitches;
  for (const auto& [portName, fabricEndpoint] : fabricEndpoints) {
    if (fabricEndpoint.isAttached().value()) {
      logFabricEndpoint(fabricEndpoint);

      auto actualRemoteSwitchId = fabricEndpoint.switchId().value();
      auto expectedRemoteSwitchId =
          fabricEndpoint.expectedSwitchId().value_or(-1);
      auto actualRemoteSwitchName = fabricEndpoint.switchName();
      auto expectedRemoteSwitchName = fabricEndpoint.expectedSwitchName();

      auto actualRemotePortId = fabricEndpoint.portId().value();
      auto expectedRemotePortId = fabricEndpoint.expectedPortId().value_or(-1);
      auto actualRemotePortName = fabricEndpoint.portName();
      auto expectedRemotePortName = fabricEndpoint.portName();

      // Expected switch/port ID/name must match for every entry
      if (!(expectedRemoteSwitchId == actualRemoteSwitchId &&
            expectedRemoteSwitchName == actualRemoteSwitchName &&
            expectedRemotePortId == actualRemotePortId &&
            expectedRemotePortName == actualRemotePortName)) {
        return false;
      }
      if (fabricEndpoint.switchName().has_value()) {
        gotConnectedSwitches.insert(fabricEndpoint.switchName().value());
      }
    }
  }

  XLOG(DBG2) << "From " << switchTypeToString(switchType)
             << ":: " << switchToVerify << " Expected Connected Switches: "
             << folly::join(",", expectedConnectedSwitches)
             << " Got Connected Switches: "
             << folly::join(",", gotConnectedSwitches);

  return expectedConnectedSwitches == gotConnectedSwitches;
}

bool MultiNodeUtil::verifyFabricConnectedSwitchesForRdsw(
    int clusterId,
    const std::string& rdswToVerify) {
  // Every RDSW is connected to all FDSWs in its cluster
  std::set<std::string> expectedConnectedSwitches(
      clusterIdToFdsws_[clusterId].begin(), clusterIdToFdsws_[clusterId].end());

  return verifyFabricConnectedSwitchesHelper(
      SwitchType::RDSW, rdswToVerify, expectedConnectedSwitches);
}

bool MultiNodeUtil::verifyFabricConnectedSwitchesForAllRdsws() {
  for (const auto& [clusterId, rdsws] : std::as_const(clusterIdToRdsws_)) {
    for (const auto& rdsw : rdsws) {
      if (!verifyFabricConnectedSwitchesForRdsw(clusterId, rdsw)) {
        return false;
      }
    }
  }

  return true;
}

bool MultiNodeUtil::verifyFabricConnectedSwitchesForFdsw(
    int clusterId,
    const std::string& fdswToVerify) {
  // Every FDSW is connected to all RDSWs in its cluster and all SDSWs
  std::set<std::string> expectedConnectedSwitches(
      clusterIdToRdsws_[clusterId].begin(), clusterIdToRdsws_[clusterId].end());
  expectedConnectedSwitches.insert(sdsws_.begin(), sdsws_.end());

  return verifyFabricConnectedSwitchesHelper(
      SwitchType::FDSW, fdswToVerify, expectedConnectedSwitches);
}

bool MultiNodeUtil::verifyFabricConnectedSwitchesForAllFdsws() {
  for (const auto& [clusterId, fdsws] : std::as_const(clusterIdToFdsws_)) {
    for (const auto& fdsw : fdsws) {
      if (!verifyFabricConnectedSwitchesForFdsw(clusterId, fdsw)) {
        return false;
      }
    }
  }

  return true;
}

bool MultiNodeUtil::verifyFabricConnectedSwitchesForSdsw(
    const std::string& sdswToVerify) {
  // Every SDSW is connected to all FDSWs in all clusters
  auto expectedConnectedSwitches = allFdsws_;

  return verifyFabricConnectedSwitchesHelper(
      SwitchType::SDSW, sdswToVerify, expectedConnectedSwitches);
}

bool MultiNodeUtil::verifyFabricConnectedSwitchesForAllSdsws() {
  for (const auto& sdsw : sdsws_) {
    if (!verifyFabricConnectedSwitchesForSdsw(sdsw)) {
      return false;
    }
  }

  return true;
}

bool MultiNodeUtil::verifyFabricConnectivity() {
  return verifyFabricConnectedSwitchesForAllRdsws() &&
      verifyFabricConnectedSwitchesForAllFdsws() &&
      verifyFabricConnectedSwitchesForAllSdsws();
}

bool MultiNodeUtil::verifyFabricReachablityForRdsw(
    const std::string& rdswToVerify) {
  auto logReachability = [rdswToVerify](
                             const std::string& remoteSwitchName,
                             const std::vector<std::string>& reachablePorts) {
    XLOG(DBG2) << "From " << rdswToVerify
               << " remoteSwitchName: " << remoteSwitchName
               << " reachablePorts: " << folly::join(",", reachablePorts);
  };

  // Every remote RDSW across all clusters is reachable from the local RDSW
  std::vector<std::string> remoteSwitchNames;
  std::copy_if(
      allRdsws_.begin(),
      allRdsws_.end(),
      std::back_inserter(remoteSwitchNames),
      [rdswToVerify](const std::string& switchName) {
        return switchName != rdswToVerify; // exclude self
      });

  auto swAgentClient = getSwAgentThriftClient(rdswToVerify);
  std::map<std::string, std::vector<std::string>> reachability;
  swAgentClient->sync_getSwitchReachability(reachability, remoteSwitchNames);

  // Every remote RDSW must be reachable via every local active port.
  // TODO: This assertion is not true when Input Balanced Mode is enabled.
  // We will enhance this check for DSF Dual Stage where Input Balanced Mode
  // is enabled.
  auto activePorts = getActiveFabricPorts(rdswToVerify);

  for (auto& [remoteSwitchName, reachablePorts] : reachability) {
    logReachability(remoteSwitchName, reachablePorts);

    std::set<std::string> reachablePortsSet(
        reachablePorts.begin(), reachablePorts.end());
    XLOG(DBG2) << "From RDSW:: " << rdswToVerify
               << " Expected reachable ports (Active Ports): "
               << folly::join(",", activePorts) << " Got reachable ports: "
               << folly::join(",", reachablePortsSet);

    if (activePorts != reachablePortsSet) {
      return false;
    }
  }

  return true;
}

bool MultiNodeUtil::verifyFabricReachability() {
  for (const auto& rdsw : allRdsws_) {
    if (!verifyFabricReachablityForRdsw(rdsw)) {
      return false;
    }
  }

  return true;
}

std::map<int32_t, facebook::fboss::PortInfoThrift> MultiNodeUtil::getPorts(
    const std::string& switchName) {
  std::map<int32_t, facebook::fboss::PortInfoThrift> portEntries;
  auto swAgentClient = getSwAgentThriftClient(switchName);
  swAgentClient->sync_getAllPortInfo(portEntries);

  return portEntries;
}

std::set<std::string> MultiNodeUtil::getActiveFabricPorts(
    const std::string& switchName) {
  std::set<std::string> activePorts;
  for (const auto& [_, portInfo] : getFabricPortNameToPortInfo(switchName)) {
    if (portInfo.activeState().has_value() &&
        portInfo.activeState().value() == PortActiveState::ACTIVE) {
      activePorts.insert(portInfo.name().value());
    }
  }

  return activePorts;
}

std::map<std::string, PortInfoThrift>
MultiNodeUtil::getActiveFabricPortNameToPortInfo(
    const std::string& switchName) {
  std::map<std::string, PortInfoThrift> activeFabricPortNameToPortInfo;
  for (const auto& [_, portInfo] : getFabricPortNameToPortInfo(switchName)) {
    if (portInfo.activeState().has_value() &&
        portInfo.activeState().value() == PortActiveState::ACTIVE) {
      activeFabricPortNameToPortInfo.emplace(portInfo.name().value(), portInfo);
    }
  }

  return activeFabricPortNameToPortInfo;
}

std::map<std::string, PortInfoThrift>
MultiNodeUtil::getFabricPortNameToPortInfo(const std::string& switchName) {
  std::map<std::string, PortInfoThrift> fabricPortNameToPortInfo;
  for (const auto& [_, portInfo] : getPorts(switchName)) {
    if (portInfo.portType().value() == cfg::PortType::FABRIC_PORT) {
      fabricPortNameToPortInfo.emplace(portInfo.name().value(), portInfo);
    }
  }

  return fabricPortNameToPortInfo;
}

bool MultiNodeUtil::verifyPortActiveStateForSwitch(
    SwitchType switchType,
    const std::string& switchName) {
  // Every Connected Fabric Port must be Active
  auto expectedActivePorts = getConnectedFabricPorts(switchName);
  auto gotActivePorts = getActiveFabricPorts(switchName);

  XLOG(DBG2) << "From " << switchTypeToString(switchType) << ":: " << switchName
             << " Expected Active Ports (connected fabric ports): "
             << folly::join(",", expectedActivePorts)
             << " Got Active Ports: " << folly::join(",", gotActivePorts);

  return expectedActivePorts == gotActivePorts;
}

bool MultiNodeUtil::verifyNoPortErrorsForSwitch(
    SwitchType switchType,
    const std::string& switchName) {
  // No ports should have errors
  auto ports = getPorts(switchName);
  for (const auto& port : ports) {
    auto portInfo = port.second;
    if (portInfo.activeErrors()->size() != 0) {
      XLOG(DBG2) << "From " << switchTypeToString(switchType)
                 << ":: " << switchName << " Port: " << portInfo.name().value()
                 << " has errors: "
                 << folly::join(",", *portInfo.activeErrors());
      return false;
    }
  }

  return true;
}

bool MultiNodeUtil::verifyPortsForSwitch(
    SwitchType switchType,
    const std::string& switchName) {
  return verifyPortActiveStateForSwitch(switchType, switchName) &&
      verifyNoPortErrorsForSwitch(switchType, switchName);
}

bool MultiNodeUtil::verifyPorts() {
  // The checks are identical for all Switch types at the moment
  // as we only verify Fabric ports. We may add Ethernet port checks
  // specific to RDSWs in the future.
  for (const auto& rdsw : allRdsws_) {
    if (!verifyPortsForSwitch(SwitchType::RDSW, rdsw)) {
      return false;
    }
  }

  for (const auto& fdsw : allFdsws_) {
    if (!verifyPortsForSwitch(SwitchType::FDSW, fdsw)) {
      return false;
    }
  }

  for (const auto& sdsw : sdsws_) {
    if (!verifyPortsForSwitch(SwitchType::SDSW, sdsw)) {
      return false;
    }
  }

  return true;
}

std::set<std::string> MultiNodeUtil::getGlobalSystemPortsOfType(
    const std::string& rdsw,
    const std::set<RemoteSystemPortType>& types) {
  auto logSystemPort =
      [rdsw](const facebook::fboss::SystemPortThrift& systemPort) {
        XLOG(DBG2)
            << "From " << rdsw << " portId: " << systemPort.portId().value()
            << " switchId: " << systemPort.switchId().value()
            << " portName: " << systemPort.portName().value()
            << " remoteSystemPortType: "
            << apache::thrift::util::enumNameSafe(
                   systemPort.remoteSystemPortType().value_or(-1))
            << " remoteSystemPortLivenessStatus: "
            << folly::to<std::string>(
                   systemPort.remoteSystemPortLivenessStatus().value_or(-1))
            << " scope: "
            << apache::thrift::util::enumNameSafe(systemPort.scope().value());
      };

  auto swAgentClient = getSwAgentThriftClient(rdsw);
  std::map<int64_t, facebook::fboss::SystemPortThrift> systemPortEntries;
  swAgentClient->sync_getSystemPorts(systemPortEntries);

  auto matchesPortType =
      [&types](const facebook::fboss::SystemPortThrift& systemPort) {
        if (systemPort.remoteSystemPortType().has_value()) {
          return types.find(systemPort.remoteSystemPortType().value()) !=
              types.end();
        } else {
          return types.empty();
        }
      };

  std::set<std::string> systemPortsOfType;
  for (const auto& [_, systemPort] : systemPortEntries) {
    logSystemPort(systemPort);
    if (*systemPort.scope() == cfg::Scope::GLOBAL &&
        matchesPortType(systemPort)) {
      systemPortsOfType.insert(systemPort.portName().value());
    }
  }

  return systemPortsOfType;
}

bool MultiNodeUtil::verifySystemPortsForRdsw(const std::string& rdswToVerify) {
  // Every GLOBAL system port of every remote RDSW is either a STATIC_ENTRY or
  // DYNAMIC_ENTRY of the local RDSW
  std::set<std::string> gotSystemPorts;
  std::set<std::string> expectedSystemPorts;
  for (const auto& [clusterId, rdsws] : std::as_const(clusterIdToRdsws_)) {
    for (const auto& rdsw : std::as_const(rdsws)) {
      if (rdsw == rdswToVerify) {
        gotSystemPorts = getGlobalSystemPortsOfType(
            rdswToVerify,
            {RemoteSystemPortType::STATIC_ENTRY,
             RemoteSystemPortType::
                 DYNAMIC_ENTRY} /* Get Global Remote ports of self */);
      } else {
        auto systemPortsForRdsw = getGlobalSystemPortsOfType(
            rdsw, {} /* Get Global ports of Remote RDSW */);
        expectedSystemPorts.insert(
            systemPortsForRdsw.begin(), systemPortsForRdsw.end());
      }
    }
  }

  XLOG(DBG2) << "From " << rdswToVerify << " Expected System Ports: "
             << folly::join(",", expectedSystemPorts)
             << " Got System Ports: " << folly::join(",", gotSystemPorts);

  return expectedSystemPorts == gotSystemPorts;
}

bool MultiNodeUtil::verifySystemPorts() {
  for (const auto& rdsw : allRdsws_) {
    if (!verifySystemPortsForRdsw(rdsw)) {
      return false;
    }
  }

  return true;
}

std::set<int> MultiNodeUtil::getGlobalRifsOfType(
    const std::string& rdsw,
    const std::set<RemoteInterfaceType>& types) {
  auto logRif = [rdsw](const facebook::fboss::InterfaceDetail& rif) {
    XLOG(DBG2)
        << "From " << rdsw << " interfaceName: " << rif.interfaceName().value()
        << " interfaceId: " << rif.interfaceId().value() << " remoteIntfType: "
        << apache::thrift::util::enumNameSafe(rif.remoteIntfType().value_or(-1))
        << " remoteIntfLivenessStatus: "
        << folly::to<std::string>(rif.remoteIntfLivenessStatus().value_or(-1))
        << " scope: "
        << apache::thrift::util::enumNameSafe(rif.scope().value());
  };

  auto swAgentClient = getSwAgentThriftClient(rdsw);
  std::map<int32_t, facebook::fboss::InterfaceDetail> rifs;
  swAgentClient->sync_getAllInterfaces(rifs);

  auto matchesRifType = [&types](const facebook::fboss::InterfaceDetail& rif) {
    if (rif.remoteIntfType().has_value()) {
      return types.find(rif.remoteIntfType().value()) != types.end();
    } else {
      return types.empty();
    }
  };

  std::set<int> rifsOfType;
  for (const auto& [_, rif] : rifs) {
    logRif(rif);
    if (*rif.scope() == cfg::Scope::GLOBAL && matchesRifType(rif)) {
      rifsOfType.insert(rif.interfaceId().value());
    }
  }

  return rifsOfType;
}

bool MultiNodeUtil::verifyRifsForRdsw(const std::string& rdswToVerify) {
  // Every GLOBAL rif of every remote RDSW is either a STATIC_ENTRY or
  // DYNAMIC_ENTRY of the local RDSW
  std::set<int> gotRifs;
  std::set<int> expectedRifs;
  for (const auto& [clusterId, rdsws] : std::as_const(clusterIdToRdsws_)) {
    for (const auto& rdsw : std::as_const(rdsws)) {
      if (rdsw == rdswToVerify) {
        gotRifs = getGlobalRifsOfType(
            rdswToVerify,
            {RemoteInterfaceType::STATIC_ENTRY,
             RemoteInterfaceType::
                 DYNAMIC_ENTRY} /* Get Global Remote rifs of self */);
      } else {
        auto rifsForRdsw =
            getGlobalRifsOfType(rdsw, {} /* Get Global rifs of Remote RDSW */);
        expectedRifs.insert(rifsForRdsw.begin(), rifsForRdsw.end());
      }
    }
  }

  XLOG(DBG2) << "From " << rdswToVerify
             << " Expected Rifs: " << folly::join(",", expectedRifs)
             << " Got Rifs: " << folly::join(",", gotRifs);

  return expectedRifs == gotRifs;
}

bool MultiNodeUtil::verifyRifs() {
  for (const auto& rdsw : allRdsws_) {
    if (!verifyRifsForRdsw(rdsw)) {
      return false;
    }
  }

  return true;
}

std::set<std::pair<std::string, std::string>>
MultiNodeUtil::getNdpEntriesAndSwitchOfType(
    const std::string& rdsw,
    const std::set<std::string>& types) {
  auto logNdpEntry = [rdsw](const facebook::fboss::NdpEntryThrift& ndpEntry) {
    auto ip = folly::IPAddress::fromBinary(folly::ByteRange(
        folly::StringPiece(ndpEntry.ip().value().addr().value())));

    XLOG(DBG2) << "From " << rdsw << " ip: " << ip.str()
               << " state: " << ndpEntry.state().value()
               << " switchId: " << ndpEntry.switchId().value_or(-1);
  };

  auto swAgentClient = getSwAgentThriftClient(rdsw);
  std::vector<facebook::fboss::NdpEntryThrift> ndpEntries;
  swAgentClient->sync_getNdpTable(ndpEntries);

  auto matchesNdpType =
      [&types](const facebook::fboss::NdpEntryThrift& ndpEntry) {
        return types.find(ndpEntry.state().value()) != types.end();
      };

  std::set<std::pair<std::string, std::string>> ndpEntriesAndSwitchOfType;
  for (const auto& ndpEntry : ndpEntries) {
    logNdpEntry(ndpEntry);
    if (matchesNdpType(ndpEntry)) {
      CHECK(ndpEntry.switchId().has_value());
      CHECK(
          switchIdToSwitchName_.find(SwitchID(ndpEntry.switchId().value())) !=
          std::end(switchIdToSwitchName_));

      auto ip = folly::IPAddress::fromBinary(folly::ByteRange(
          folly::StringPiece(ndpEntry.ip().value().addr().value())));

      ndpEntriesAndSwitchOfType.insert(std::make_pair(
          ip.str(),
          switchIdToSwitchName_[SwitchID(ndpEntry.switchId().value())]));
    }
  }

  return ndpEntriesAndSwitchOfType;
}

bool MultiNodeUtil::verifyStaticNdpEntries() {
  // Every remote RDSW must have a STATIC NDP entry in the local RDSW
  auto expectedRdsws = allRdsws_;

  for (const auto& [clusterId, rdsws] : std::as_const(clusterIdToRdsws_)) {
    for (const auto& rdsw : std::as_const(rdsws)) {
      auto ndpEntriesAndSwitchOfType =
          getNdpEntriesAndSwitchOfType(rdsw, {"STATIC"});

      std::set<std::string> gotRdsws;
      std::transform(
          ndpEntriesAndSwitchOfType.begin(),
          ndpEntriesAndSwitchOfType.end(),
          std::inserter(gotRdsws, gotRdsws.begin()),
          [](const auto& pair) { return pair.second; });

      if (expectedRdsws != gotRdsws) {
        XLOG(DBG2) << "STATIC NDP Entries from " << rdsw
                   << " Expected RDSWs: " << folly::join(",", expectedRdsws)
                   << " Got RDSWs: " << folly::join(",", gotRdsws);
        return false;
      }
    }
  }

  return true;
}

std::map<std::string, DsfSessionThrift> MultiNodeUtil::getPeerToDsfSession(
    const std::string& rdsw) {
  auto logDsfSession =
      [rdsw](const facebook::fboss::DsfSessionThrift& session) {
        XLOG(DBG2) << "From " << rdsw << " session: " << *session.remoteName()
                   << " state: "
                   << apache::thrift::util::enumNameSafe(*session.state())
                   << " lastEstablishedAt: "
                   << session.lastEstablishedAt().value_or(0)
                   << " lastDisconnectedAt: "
                   << session.lastDisconnectedAt().value_or(0);
      };

  auto swAgentClient = getSwAgentThriftClient(rdsw);
  std::vector<facebook::fboss::DsfSessionThrift> sessions;
  swAgentClient->sync_getDsfSessions(sessions);

  std::map<std::string, DsfSessionThrift> peerToDsfSession;
  for (const auto& session : sessions) {
    logDsfSession(session);
    // remoteName format: peerName::peerIP, extract peerName.
    size_t pos = (*session.remoteName()).find("::");
    if (pos != std::string::npos) {
      auto peer = (*session.remoteName()).substr(0, pos);
      peerToDsfSession.emplace(peer, session);
    }
  }

  return peerToDsfSession;
}

std::set<std::string> MultiNodeUtil::getRdswsWithEstablishedDsfSessions(
    const std::string& rdsw) {
  std::set<std::string> gotRdsws;
  for (const auto& [peer, session] : getPeerToDsfSession(rdsw)) {
    if (session.state() == facebook::fboss::DsfSessionState::ESTABLISHED) {
      gotRdsws.insert(peer);
    }
  }

  return gotRdsws;
}

bool MultiNodeUtil::verifyDsfSessions() {
  // Every RDSW must have an ESTABLISHED DSF Session with every other RDSW

  for (const auto& [clusterId, rdsws] : std::as_const(clusterIdToRdsws_)) {
    for (const auto& rdsw : std::as_const(rdsws)) {
      auto expectedRdsws = allRdsws_;
      expectedRdsws.erase(rdsw); // exclude self
      auto gotRdsws = getRdswsWithEstablishedDsfSessions(rdsw);
      if (expectedRdsws != gotRdsws) {
        XLOG(DBG2) << "DSF ESTABLISHED sessions from " << rdsw
                   << " Expected RDSWs: " << folly::join(",", expectedRdsws)
                   << " Got RDSWs: " << folly::join(",", gotRdsws);
        return false;
      }
    }
  }

  return true;
}

bool MultiNodeUtil::verifyNoSessionsFlap(
    const std::string& rdswToVerify,
    const std::map<std::string, DsfSessionThrift>& baselinePeerToDsfSession) {
  auto noSessionFlap =
      [this, rdswToVerify, baselinePeerToDsfSession]() -> bool {
    auto currentPeerToDsfSession = getPeerToDsfSession(rdswToVerify);
    // All entries must be identical i.e.
    // DSF Session state (ESTABLISHED or not) is the same.
    // For any session the establishedAt and connnectedAt is the same.
    return baselinePeerToDsfSession == currentPeerToDsfSession;
  };

  // It may take several (> 15) seconds for ESTABLISHED => CONNECT.
  // Thus, keep retrying for several seconds to ensure that the session
  // stays ESTABLISHED.
  return checkAlwaysTrueWithRetryErrorReturn(
      noSessionFlap, 30 /* num retries */);
}

bool MultiNodeUtil::verifyNoSessionsEstablished(
    const std::string& rdswToVerify) {
  auto noSessionsEstablished = [this, rdswToVerify]() -> bool {
    for (const auto& [peer, session] : getPeerToDsfSession(rdswToVerify)) {
      if (session.state() == facebook::fboss::DsfSessionState::ESTABLISHED) {
        return false;
      }
    }

    return true;
  };

  // It may take several (> 15) seconds for ESTABLISHED => CONNECT. Thus,
  // try for several seconds and check if the session transitions to
  // CONNECT.
  return checkWithRetryErrorReturn(noSessionsEstablished, 30 /* num retries */);
}

bool MultiNodeUtil::verifyAllSessionsEstablished(
    const std::string& rdswToVerify) {
  auto allSessionsEstablished = [this, rdswToVerify]() -> bool {
    for (const auto& [peer, session] : getPeerToDsfSession(rdswToVerify)) {
      if (session.state() != facebook::fboss::DsfSessionState::ESTABLISHED) {
        return false;
      }
    }

    return true;
  };

  // It may take several seconds(>15) for ESTABLISHED => CONNECT. Thus,
  // try for several seconds and check if the session transitions to
  // ESTABLISHED.
  return checkWithRetryErrorReturn(
      allSessionsEstablished, 30 /* num retries */);
}

bool MultiNodeUtil::verifyGracefulFabricLinkDown(
    const std::string& rdswToVerify,
    const std::map<std::string, PortInfoThrift>&
        activeFabricPortNameToPortInfo) {
  CHECK(activeFabricPortNameToPortInfo.size() > 2);
  auto rIter = activeFabricPortNameToPortInfo.rbegin();
  auto lastActivePort = rIter->first;
  auto secondLastActivePort = std::next(rIter)->first;
  auto baselinePeerToDsfSession = getPeerToDsfSession(rdswToVerify);

  // Admin disable all Active fabric ports
  // Verify that all sessions stay established till last port is disabled.
  // TODO: modify the config to set minLink threshold, and then extend the
  // logic to verify that the sessions stay established while the minLink
  // requirement is met.
  for (const auto& [portName, portInfo] : activeFabricPortNameToPortInfo) {
    XLOG(DBG2) << __func__
               << " Admin disabling port:: " << portInfo.name().value()
               << " portID: " << portInfo.portId().value();
    adminDisablePort(rdswToVerify, portInfo.portId().value());

    bool checkPassed = true;
    if (portName == lastActivePort) {
      checkPassed = verifyNoSessionsEstablished(rdswToVerify);
    } else if (portName == secondLastActivePort) {
      // verify no flaps is expensive.
      // Thus, only verify just before disabling the last port.
      // There is no loss of signal due to this approach as if the sessions
      // flap due to an intermediate port admin disable, it will be detected by
      // this check failure anyway.
      checkPassed =
          verifyNoSessionsFlap(rdswToVerify, baselinePeerToDsfSession);
    }
    if (!checkPassed) {
      return false;
    }
  }

  return true;
}

bool MultiNodeUtil::verifyGracefulFabricLinkUp(
    const std::string& rdswToVerify,
    const std::map<std::string, PortInfoThrift>&
        activeFabricPortNameToPortInfo) {
  CHECK(activeFabricPortNameToPortInfo.size() > 2);
  auto firstActivePort = activeFabricPortNameToPortInfo.begin()->first;
  auto rIter = activeFabricPortNameToPortInfo.rbegin();
  auto lastActivePort = rIter->first;
  // Admin Re-enable these fabric ports
  for (const auto& [portName, portInfo] : activeFabricPortNameToPortInfo) {
    XLOG(DBG2) << __func__
               << " Admin enabling port:: " << portInfo.name().value()
               << " portID: " << portInfo.portId().value();
    adminEnablePort(rdswToVerify, portInfo.portId().value());

    bool checkPassed = true;
    if (portName == firstActivePort || portName == lastActivePort) {
      checkPassed = verifyAllSessionsEstablished(rdswToVerify);
    }

    if (!checkPassed) {
      return false;
    }
  }

  return true;
}

bool MultiNodeUtil::verifyGracefulFabricLinkDownUp() {
  auto myHostname = getLocalHostname();
  auto activeFabricPortNameToPortInfo =
      getActiveFabricPortNameToPortInfo(myHostname);

  if (!verifyGracefulFabricLinkDown(
          myHostname, activeFabricPortNameToPortInfo)) {
    return false;
  }

  if (!verifyGracefulFabricLinkUp(myHostname, activeFabricPortNameToPortInfo)) {
    return false;
  }

  return true;
}

bool MultiNodeUtil::verifySwSwitchRunState(
    const std::string& rdswToVerify,
    const SwitchRunState& expectedSwitchRunState) {
  auto switchRunStateMatches =
      [this, rdswToVerify, expectedSwitchRunState]() -> bool {
    auto multiSwitchRunState = getMultiSwitchRunState(rdswToVerify);
    auto gotSwitchRunState = multiSwitchRunState.swSwitchRunState();
    return gotSwitchRunState == expectedSwitchRunState;
  };

  // Thrift client queries will throw exception while the Agent is initializing.
  // Thus, continue to retry while absorbing exceptions.
  return checkWithRetryErrorReturn(
      switchRunStateMatches,
      30 /* num retries */,
      std::chrono::milliseconds(5000) /* sleep between retries */,
      true /* retry on exception */);
}

bool MultiNodeUtil::verifyGracefulDeviceDownUpForRemoteRdsws() {
  // For any one RDSW in every remote cluster issue graceful restart
  auto myHostname = network::NetworkUtil::getLocalHost(
      true /* stripFbDomain */, true /* stripTFbDomain */);
  for (const auto& [_, rdsws] : std::as_const(clusterIdToRdsws_)) {
    for (const auto& rdsw : std::as_const(rdsws)) {
      if (rdsw == myHostname) { // exclude self
        continue;
      }

      triggerGracefulAgentRestart(rdsw);
      // Wait for the switch to come up
      if (!verifySwSwitchRunState(rdsw, SwitchRunState::CONFIGURED)) {
        XLOG(DBG2) << "Agent failed to come up post warmboot: " << rdsw;
        return false;
      }

      // TODO verify
      // Gracefully restart only one remote RDSW per cluster

      // Restart only one remote RDSW per cluster
      break;
    }
  }

  return true;
}

bool MultiNodeUtil::verifyGracefulDeviceDownUpForRemoteFdsws() {
  // For any one FDSW in every remote cluster issue graceful restart
  for (const auto& [_, fdsws] : std::as_const(clusterIdToFdsws_)) {
    // Gracefully restart only one remote FDSW per cluster
    if (!fdsws.empty()) {
      auto fdsw = fdsws.front();
      triggerGracefulAgentRestart(fdsw);
      // Wait for the switch to come up
      if (!verifySwSwitchRunState(fdsw, SwitchRunState::CONFIGURED)) {
        XLOG(DBG2) << "Agent failed to come up post warmboot: " << fdsw;
        return false;
      }
    }
  }

  return true;
}

bool MultiNodeUtil::verifyGracefulDeviceDownUpForRemoteSdsws() {
  return true;
}

bool MultiNodeUtil::verifyGracefulDeviceDownUp() {
  return verifyGracefulDeviceDownUpForRemoteRdsws() &&
      verifyGracefulDeviceDownUpForRemoteFdsws() &&
      verifyGracefulDeviceDownUpForRemoteSdsws();
}

} // namespace facebook::fboss::utility
