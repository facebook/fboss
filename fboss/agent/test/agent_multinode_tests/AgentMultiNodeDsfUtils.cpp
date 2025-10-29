// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/agent/test/agent_multinode_tests/AgentMultiNodeDsfUtils.h"

#include "fboss/agent/test/thrift_client_utils/ThriftClientUtils.h"
#include "fboss/lib/CommonUtils.h"

namespace {
using namespace facebook::fboss::utility;
using facebook::fboss::FabricEndpoint;
using facebook::fboss::NdpEntryThrift;
using facebook::fboss::PortActiveState;
using facebook::fboss::PortInfoThrift;
using facebook::fboss::RemoteInterfaceType;
using facebook::fboss::RemoteSystemPortType;
using facebook::fboss::cfg::PortType;
using facebook::fboss::cfg::Scope;

bool verifyFabricConnectivityHelper(
    const std::string& switchToVerify,
    const std::set<std::string>& expectedConnectedSwitches) {
  std::set<std::string> gotConnectedSwitches;
  for (const auto& [portName, fabricEndpoint] :
       getFabricPortToFabricEndpoint(switchToVerify)) {
    if (fabricEndpoint.isAttached().value()) {
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
  return expectedConnectedSwitches == gotConnectedSwitches;
}

std::set<std::string> getConnectedFabricPorts(const std::string& switchName) {
  std::set<std::string> connectedFabricPorts;
  for (const auto& [portName, fabricEndpoint] :
       getFabricPortToFabricEndpoint(switchName)) {
    if (fabricEndpoint.isAttached().value()) {
      connectedFabricPorts.insert(portName);
    }
  }

  return connectedFabricPorts;
}

std::map<std::string, PortInfoThrift> getFabricPortNameToPortInfo(
    const std::string& switchName) {
  std::map<std::string, PortInfoThrift> fabricPortNameToPortInfo;
  for (const auto& [_, portInfo] : getPortIdToPortInfo(switchName)) {
    if (portInfo.portType().value() == PortType::FABRIC_PORT) {
      fabricPortNameToPortInfo.emplace(portInfo.name().value(), portInfo);
    }
  }

  return fabricPortNameToPortInfo;
}

std::map<std::string, PortInfoThrift> getActiveFabricPortNameToPortInfo(
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

std::set<std::string> getActiveFabricPorts(const std::string& switchName) {
  auto activeFabricPortNameToPortInfo =
      getActiveFabricPortNameToPortInfo(switchName);
  std::set<std::string> activePorts;

  std::transform(
      activeFabricPortNameToPortInfo.begin(),
      activeFabricPortNameToPortInfo.end(),
      std::inserter(activePorts, activePorts.begin()),
      [](const auto& pair) {
        const auto& portInfo = pair.second;
        return portInfo.name().value();
      });

  return activePorts;
}

std::set<std::string> getGlobalSystemPortsOfType(
    const std::string& rdsw,
    const std::set<RemoteSystemPortType>& types) {
  auto matchesSystemPortType =
      [&types](const facebook::fboss::SystemPortThrift& systemPort) {
        if (systemPort.remoteSystemPortType().has_value()) {
          return types.find(systemPort.remoteSystemPortType().value()) !=
              types.end();
        } else {
          return types.empty();
        }
      };

  std::set<std::string> systemPortsOfType;
  for (const auto& [_, systemPort] : getSystemPortdIdToSystemPort(rdsw)) {
    if (*systemPort.scope() == Scope::GLOBAL &&
        matchesSystemPortType(systemPort)) {
      systemPortsOfType.insert(systemPort.portName().value());
    }
  }
  return systemPortsOfType;
}

std::set<int> getGlobalRifsOfType(
    const std::string& rdsw,
    const std::set<RemoteInterfaceType>& types) {
  auto matchesRifType = [&types](const facebook::fboss::InterfaceDetail& rif) {
    if (rif.remoteIntfType().has_value()) {
      return types.find(rif.remoteIntfType().value()) != types.end();
    } else {
      return types.empty();
    }
  };

  std::set<int> rifsOfType;
  for (const auto& [_, rif] : getIntfIdToIntf(rdsw)) {
    if (*rif.scope() == Scope::GLOBAL && matchesRifType(rif)) {
      rifsOfType.insert(rif.interfaceId().value());
    }
  }
  return rifsOfType;
}

std::vector<NdpEntryThrift> getNdpEntriesOfType(
    const std::string& rdsw,
    const std::set<std::string>& types) {
  auto ndpEntries = getNdpEntries(rdsw);

  std::vector<NdpEntryThrift> filteredNdpEntries;
  std::copy_if(
      ndpEntries.begin(),
      ndpEntries.end(),
      std::back_inserter(filteredNdpEntries),
      [rdsw, &types](const facebook::fboss::NdpEntryThrift& ndpEntry) {
        return types.find(ndpEntry.state().value()) != types.end();
      });

  return filteredNdpEntries;
}

} // namespace

namespace facebook::fboss::utility {

bool verifyFabricConnectivityForRdsw(
    const std::unique_ptr<TopologyInfo>& topologyInfo,
    int clusterId,
    const std::string& rdswToVerify) {
  // Every RDSW is connected to all FDSWs in its cluster
  std::set<std::string> expectedConnectedSwitches(
      std::begin(topologyInfo->getClusterIdToFdsws().at(clusterId)),
      std::end(topologyInfo->getClusterIdToFdsws().at(clusterId)));

  return verifyFabricConnectivityHelper(
      rdswToVerify, expectedConnectedSwitches);
}

bool verifyFabricConnectivityForRdsws(
    const std::unique_ptr<TopologyInfo>& topologyInfo) {
  for (const auto& [clusterId, rdsws] :
       std::as_const(topologyInfo->getClusterIdToRdsws())) {
    for (const auto& rdsw : rdsws) {
      if (!verifyFabricConnectivityForRdsw(topologyInfo, clusterId, rdsw)) {
        return false;
      }
    }
  }

  return true;
}

bool verifyFabricConnectivityForFdsw(
    const std::unique_ptr<TopologyInfo>& topologyInfo,
    int clusterId,
    const std::string& fdswToVerify) {
  // Every FDSW is connected to all RDSWs in its cluster and all SDSWs
  std::set<std::string> expectedConnectedSwitches(
      std::begin(topologyInfo->getClusterIdToRdsws().at(clusterId)),
      std::end(topologyInfo->getClusterIdToRdsws().at(clusterId)));
  expectedConnectedSwitches.insert(
      topologyInfo->getSdsws().begin(), topologyInfo->getSdsws().end());

  return verifyFabricConnectivityHelper(
      fdswToVerify, expectedConnectedSwitches);
}

bool verifyFabricConnectivityForFdsws(
    const std::unique_ptr<TopologyInfo>& topologyInfo) {
  for (const auto& [clusterId, fdsws] :
       std::as_const(topologyInfo->getClusterIdToFdsws())) {
    for (const auto& fdsw : fdsws) {
      if (!verifyFabricConnectivityForFdsw(topologyInfo, clusterId, fdsw)) {
        return false;
      }
    }
  }

  return true;
}

bool verifyFabricConnectivityForSdsw(
    const std::unique_ptr<TopologyInfo>& topologyInfo,
    const std::string& sdswToVerify) {
  // Every SDSW is connected to all FDSWs in all clusters
  return verifyFabricConnectivityHelper(sdswToVerify, topologyInfo->getFdsws());
}

bool verifyFabricConnectivityForSdsws(
    const std::unique_ptr<TopologyInfo>& topologyInfo) {
  for (const auto& sdsw : std::as_const(topologyInfo->getSdsws())) {
    if (!verifyFabricConnectivityForSdsw(topologyInfo, sdsw)) {
      return false;
    }
  }

  return true;
}

bool verifyFabricConnectivity(
    const std::unique_ptr<TopologyInfo>& topologyInfo) {
  return verifyFabricConnectivityForRdsws(topologyInfo) &&
      verifyFabricConnectivityForFdsws(topologyInfo) &&
      verifyFabricConnectivityForSdsws(topologyInfo);
}

bool verifyFabricReachabilityForRdsw(
    const std::unique_ptr<TopologyInfo>& topologyInfo,
    const std::string& rdswToVerify) {
  // Every remote RDSW across all clusters is reachable from the local RDSW
  std::vector<std::string> remoteSwitchNames;
  std::copy_if(
      topologyInfo->getRdsws().begin(),
      topologyInfo->getRdsws().end(),
      std::back_inserter(remoteSwitchNames),
      [rdswToVerify](const std::string& switchName) {
        return switchName != rdswToVerify; // exclude self
      });

  auto remoteSwitchToReachablePorts =
      getRemoteSwitchToReachablePorts(rdswToVerify, remoteSwitchNames);

  // Every remote RDSW must be reachable via every local active port.
  // TODO: This assertion is not true when Input Balanced Mode is enabled.
  // We will enhance this check for DSF Dual Stage where Input Balanced Mode
  // is enabled.
  auto activePorts = getActiveFabricPorts(rdswToVerify);

  for (auto& [remoteSwitchName, reachablePorts] :
       remoteSwitchToReachablePorts) {
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

bool verifyFabricReachability(
    const std::unique_ptr<TopologyInfo>& topologyInfo) {
  for (const auto& rdsw : topologyInfo->getRdsws()) {
    if (!verifyFabricReachabilityForRdsw(topologyInfo, rdsw)) {
      return false;
    }
  }

  return true;
}

bool verifyPortActiveStateForSwitch(const std::string& switchName) {
  // Every Connected Fabric Port must be Active
  auto connectedPorts = getConnectedFabricPorts(switchName);
  auto activePorts = getActiveFabricPorts(switchName);

  return connectedPorts == activePorts;
}

bool verifyNoPortErrorsForSwitch(const std::string& switchName) {
  // No ports should have errors
  for (const auto& [_, portInfo] : getPortIdToPortInfo(switchName)) {
    if (portInfo.activeErrors()->size() != 0) {
      return false;
    }
  }

  return true;
}

bool verifyPortCableLength(const std::string& switchName) {
  // Cable length query (i.e. cable propagation delay attribute) works only on
  // below fabric ports::
  // DSF Single Stage system:
  //   o RDSW Fabric ports towards FDSW
  // DSF Dual Stage system:
  //   o RDSW Fabric ports towards FDSW
  //   o FDSW Fabric ports towards SDSW
  //
  // TODO Enhance this check for DSF Dual stage:
  //    FDSW Fabric ports towards SDSW

  // Verify if all connected fabric ports have valid cable length
  for (const auto& [_, portInfo] :
       getActiveFabricPortNameToPortInfo(switchName)) {
    if (portInfo.cableLengthMeters().has_value() &&
        portInfo.cableLengthMeters().value() >= 0) {
      // Cable length may vary on different test setups. Thus, verify
      // if the Cable length query returns a valid non-0 cable length.
      continue;
    }

    return false;
  }

  return true;
}

bool verifyFabricPorts(const std::string& switchName) {
  return verifyPortActiveStateForSwitch(switchName) &&
      verifyNoPortErrorsForSwitch(switchName);
}

bool verifyPortsForRdsws(const std::unique_ptr<TopologyInfo>& topologyInfo) {
  for (const auto& rdsw : topologyInfo->getRdsws()) {
    if (!verifyFabricPorts(rdsw)) {
      return false;
    }
    if (!verifyPortCableLength(rdsw)) {
      return false;
    }
  }

  return true;
}

bool verifyPortsForFdsws(const std::unique_ptr<TopologyInfo>& topologyInfo) {
  for (const auto& fdsw : topologyInfo->getFdsws()) {
    if (!verifyFabricPorts(fdsw)) {
      return false;
    }
  }

  return true;
}

bool verifyPortsForSdsws(const std::unique_ptr<TopologyInfo>& topologyInfo) {
  for (const auto& sdsw : topologyInfo->getSdsws()) {
    if (!verifyFabricPorts(sdsw)) {
      return false;
    }
  }

  return true;
}

bool verifyPorts(const std::unique_ptr<TopologyInfo>& topologyInfo) {
  return verifyPortsForRdsws(topologyInfo) &&
      verifyPortsForFdsws(topologyInfo) && verifyPortsForSdsws(topologyInfo);
}

bool verifySystemPortsForRdsw(
    const std::unique_ptr<TopologyInfo>& topologyInfo,
    const std::string& rdswToVerify) {
  // Every GLOBAL system port of every remote RDSW is either a STATIC_ENTRY
  // or DYNAMIC_ENTRY of the local RDSW
  std::set<std::string> gotSystemPorts;
  std::set<std::string> expectedSystemPorts;
  for (const auto& [clusterId, rdsws] :
       std::as_const(topologyInfo->getClusterIdToRdsws())) {
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

  return expectedSystemPorts == gotSystemPorts;
}

bool verifySystemPorts(const std::unique_ptr<TopologyInfo>& topologyInfo) {
  for (const auto& rdsw : topologyInfo->getRdsws()) {
    if (!verifySystemPortsForRdsw(topologyInfo, rdsw)) {
      return false;
    }
  }

  return true;
}

bool verifyRifsForRdsw(
    const std::unique_ptr<TopologyInfo>& topologyInfo,
    const std::string& rdswToVerify) {
  // Every GLOBAL rif of every remote RDSW is either a STATIC_ENTRY or
  // DYNAMIC_ENTRY of the local RDSW
  std::set<int> gotRifs;
  std::set<int> expectedRifs;
  for (const auto& [clusterId, rdsws] :
       std::as_const(topologyInfo->getClusterIdToRdsws())) {
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

  return expectedRifs == gotRifs;
}

bool verifyRifs(const std::unique_ptr<TopologyInfo>& topologyInfo) {
  for (const auto& rdsw : topologyInfo->getRdsws()) {
    if (!verifyRifsForRdsw(topologyInfo, rdsw)) {
      return false;
    }
  }

  return true;
}

bool verifyStaticNdpEntriesForRdsw(
    const std::unique_ptr<TopologyInfo>& topologyInfo,
    const std::string& rdswToVerify) {
  return true;
}

bool verifyStaticNdpEntries(const std::unique_ptr<TopologyInfo>& topologyInfo) {
  for (const auto& rdsw : topologyInfo->getRdsws()) {
    if (!verifyStaticNdpEntriesForRdsw(topologyInfo, rdsw)) {
      return false;
    }
  }

  return true;
}

void verifyDsfCluster(const std::unique_ptr<TopologyInfo>& topologyInfo) {
  WITH_RETRIES_N_TIMED(10, std::chrono::milliseconds(5000), {
    verifyFabricConnectivity(topologyInfo);
    verifyFabricReachability(topologyInfo);
    verifyPorts(topologyInfo);
    verifySystemPorts(topologyInfo);
    verifyRifs(topologyInfo);
    verifyStaticNdpEntries(topologyInfo);
  });
}

void verifyDsfAgentDownUp() {}

} // namespace facebook::fboss::utility
