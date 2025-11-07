// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/agent/test/agent_multinode_tests/AgentMultiNodeUtils.h"

#include "fboss/agent/test/thrift_client_utils/ThriftClientUtils.h"
#include "fboss/lib/CommonUtils.h"

namespace {

void logNdpEntry(
    const std::string& switchName,
    const facebook::fboss::NdpEntryThrift& ndpEntry) {
  auto ip = folly::IPAddress::fromBinary(
      folly::ByteRange(
          folly::StringPiece(ndpEntry.ip().value().addr().value())));

  XLOG(DBG2) << "From " << switchName << " ip: " << ip.str()
             << " state: " << ndpEntry.state().value()
             << " switchId: " << ndpEntry.switchId().value_or(-1);
}

void logSwitchToNdpEntries(
    const std::map<std::string, std::vector<facebook::fboss::NdpEntryThrift>>&
        switchToNdpEntries) {
  for (const auto& [switchName, ndpEntries] : switchToNdpEntries) {
    for (const auto& ndpEntry : ndpEntries) {
      logNdpEntry(switchName, ndpEntry);
    }
  }
}

// if allNeighborsMustBePresent is true, then all neighbors must be present
// for every switch in switchToNdpEntries.
// if allNeighborsMustBePresent is false, then all neighbors must be absent
// for every switch in switchToNdpEntries.
bool verifyNeighborsAllPresentOrAllAbsent(
    const std::vector<facebook::fboss::utility::Neighbor>& neighbors,
    const std::map<std::string, std::vector<facebook::fboss::NdpEntryThrift>>&
        switchToNdpEntries,
    bool allNeighborsMustBePresent) {
  auto isNeighborPresentHelper = [](const auto& ndpEntries,
                                    const auto& neighbor) {
    for (const auto& ndpEntry : ndpEntries) {
      auto ndpEntryIp = folly::IPAddress::fromBinary(
          folly::ByteRange(
              folly::StringPiece(ndpEntry.ip().value().addr().value())));

      if (ndpEntry.interfaceID().value() == neighbor.intfID &&
          ndpEntryIp == neighbor.ip) {
        return true;
      }
    }

    return false;
  };

  for (const auto& neighbor : neighbors) {
    for (const auto& [switchName, ndpEntries] : switchToNdpEntries) {
      auto isNeighborPresent = isNeighborPresentHelper(ndpEntries, neighbor);
      if (allNeighborsMustBePresent) {
        if (!isNeighborPresent) {
          XLOG(DBG2) << "Switch: " << switchName
                     << " neighbor missing: " << neighbor.str();
          return false;
        }
      } else { // allNeighborsMust NOT be present
        if (isNeighborPresent) {
          XLOG(DBG2) << "Switch: " << switchName
                     << " excess neighbor: " << neighbor.str();
          return false;
        }
      }
    }
  }

  return true;
}

} // namespace

namespace facebook::fboss::utility {

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
        logNdpEntry(rdsw, ndpEntry);
        return types.find(ndpEntry.state().value()) != types.end();
      });

  return filteredNdpEntries;
}

std::map<std::string, PortInfoThrift> getUpEthernetPortNameToPortInfo(
    const std::string& switchName) {
  std::map<std::string, PortInfoThrift> upEthernetPortNameToPortInfo;
  for (const auto& [_, portInfo] : getPortIdToPortInfo(switchName)) {
    if (portInfo.portType().value() == cfg::PortType::INTERFACE_PORT &&
        portInfo.operState().value() == PortOperState::UP) {
      upEthernetPortNameToPortInfo.emplace(portInfo.name().value(), portInfo);
    }
  }

  return upEthernetPortNameToPortInfo;
}

std::map<std::string, int64_t> getSwitchNameToQsfpAliveSinceEpoch(
    const std::map<std::string, std::set<facebook::fboss::SwitchID>>&
        switchNameToSwitchIds) {
  std::map<std::string, int64_t> switchNameToQsfpAliveSinceEpoch;
  for (const auto& [switchName, _] : switchNameToSwitchIds) {
    switchNameToQsfpAliveSinceEpoch[switchName] =
        getQsfpAliveSinceEpoch(switchName);
  }
  return switchNameToQsfpAliveSinceEpoch;
}

std::map<std::string, int64_t> getSwitchNameToFsdbAliveSinceEpoch(
    const std::map<std::string, std::set<facebook::fboss::SwitchID>>&
        switchNameToSwitchIds) {
  std::map<std::string, int64_t> switchNameToFsdbAliveSinceEpoch;
  for (const auto& [switchName, _] : switchNameToSwitchIds) {
    switchNameToFsdbAliveSinceEpoch[switchName] =
        getFsdbAliveSinceEpoch(switchName);
  }
  return switchNameToFsdbAliveSinceEpoch;
}

bool verifySwSwitchRunState(
    const std::string& switchName,
    const SwitchRunState& expectedSwitchRunState) {
  auto switchRunStateMatches = [switchName, expectedSwitchRunState] {
    auto multiSwitchRunState = getMultiSwitchRunState(switchName);
    auto gotSwitchRunState = multiSwitchRunState.swSwitchRunState();
    return gotSwitchRunState == expectedSwitchRunState;
  };

  // Thrift client queries will throw exception while the Agent is
  // initializing. Thus, continue to retry while absorbing exceptions.
  return checkWithRetryErrorReturn(
      switchRunStateMatches,
      30 /* num retries */,
      std::chrono::milliseconds(5000) /* sleep between retries */,
      true /* retry on exception */);
}

bool verifyQsfpRestarted(
    const std::map<std::string, std::set<SwitchID>>& switchNameToSwitchIds,
    const std::map<std::string, int64_t>& baselinePeerToQsfpAliveSinceEpoch) {
  auto allQsfpRestarted = [switchNameToSwitchIds,
                           baselinePeerToQsfpAliveSinceEpoch] {
    auto currentSwitchNameToQsfpAliveSinceEpoch =
        getSwitchNameToQsfpAliveSinceEpoch(switchNameToSwitchIds);
    // Verify all QSFPs restarted
    return std::all_of(
        currentSwitchNameToQsfpAliveSinceEpoch.begin(),
        currentSwitchNameToQsfpAliveSinceEpoch.end(),
        [&](const auto& pair) {
          const auto& switchName = pair.first;
          const auto& aliveSinceEpoch = pair.second;
          auto baselineIt = baselinePeerToQsfpAliveSinceEpoch.find(switchName);
          CHECK(baselineIt != baselinePeerToQsfpAliveSinceEpoch.end());
          // If the QSFP has restarted, aliveSince will be greater i.e. later
          // timestamp.
          return baselineIt->second < aliveSinceEpoch;
        });
  };

  return checkWithRetryErrorReturn(
      allQsfpRestarted,
      30 /* num retries */,
      std::chrono::milliseconds(5000) /* sleep between retries */,
      true /* retry on exception */);
}

bool verifyQsfpServiceRunState(
    const std::string& switchName,
    const QsfpServiceRunState& expectedQsfpRunState) {
  auto qsfpServiceRunStateMatches = [switchName, expectedQsfpRunState]() {
    auto gotQsfpServiceRunState = getQsfpServiceRunState(switchName);
    return gotQsfpServiceRunState == expectedQsfpRunState;
  };

  // Thrift client queries will throw exception while QSFP is initializing.
  // Thus, continue to retry while absorbing exceptions.
  return checkWithRetryErrorReturn(
      qsfpServiceRunStateMatches,
      30 /* num retries */,
      std::chrono::milliseconds(5000) /* sleep between retries */,
      true /* retry on exception */);
}

bool verifyFsdbRestarted(
    const std::map<std::string, std::set<SwitchID>>& switchNameToSwitchIds,
    const std::map<std::string, int64_t>&
        baselineSwitchNameToFsdbAliveSinceEpoch) {
  auto allFsdbRestarted = [switchNameToSwitchIds,
                           baselineSwitchNameToFsdbAliveSinceEpoch] {
    auto currentSwitchNameToFsdbAliveSinceEpoch =
        getSwitchNameToFsdbAliveSinceEpoch(switchNameToSwitchIds);
    // Verify all FSDBs restarted
    return std::all_of(
        currentSwitchNameToFsdbAliveSinceEpoch.begin(),
        currentSwitchNameToFsdbAliveSinceEpoch.end(),
        [&](const auto& pair) {
          const auto& switchName = pair.first;
          const auto& aliveSinceEpoch = pair.second;
          auto baselineIt =
              baselineSwitchNameToFsdbAliveSinceEpoch.find(switchName);
          CHECK(baselineIt != baselineSwitchNameToFsdbAliveSinceEpoch.end());
          // If the FSDB has restarted, aliveSince will be greater i.e. later
          // timestamp.
          return baselineIt->second < aliveSinceEpoch;
        });
  };

  return checkWithRetryErrorReturn(
      allFsdbRestarted,
      30 /* num retries */,
      std::chrono::milliseconds(5000) /* sleep between retries */,
      true /* retry on exception */);
}

bool verifyFsdbIsUp(const std::string& switchName) {
  auto fsdbIsUp = [switchName]() -> bool {
    // Unlike Agent and QSFP, FSDB lacks notion of "RunState" that can be
    // queried to confirm whether FSDB has completed initialization.
    // In the meanwhile, query some FSDB thrift method.
    // If FSDB has not up yet, this will throw an error and
    // checkwithRetryErrorReturn will retry the specified number of times.
    // TODO: T238268316 will add "RunState" to FSDB. Leverage it then.
    getSubscriberIdToOperSusbscriberInfos(switchName);
    return true;
  };

  // Thrift client queries will throw exception while FSDB is initializing.
  // Thus, continue to retry while absorbing exceptions.
  return checkWithRetryErrorReturn(
      fsdbIsUp,
      30 /* num retries */,
      std::chrono::milliseconds(5000) /* sleep between retries */,
      true /* retry on exception */);
}

bool verifyNeighborsPresent(
    const std::set<std::string>& allSwitches,
    const std::string& switchToVerify,
    const std::vector<Neighbor>& neighbors) {
  auto verifyNeighborPresentHelper = [allSwitches, switchToVerify, neighbors] {
    auto getSwitchToNdpEntries = [allSwitches, switchToVerify]() {
      std::map<std::string, std::vector<NdpEntryThrift>> switchToNdpEntries;
      for (const auto& switchName : allSwitches) {
        if (switchName ==
            switchToVerify) { // PROBE/REACHABLE for switchToVerify
          switchToNdpEntries[switchName] =
              getNdpEntriesOfType(switchToVerify, {"PROBE", "REACHABLE"});
        } else { // DYNAMIC for every remote switch
          switchToNdpEntries[switchName] =
              getNdpEntriesOfType(switchName, {"DYNAMIC"});
        }
      }

      return switchToNdpEntries;
    };

    // Every neighbor added to switchToVerify, the neighbor must be:
    //    - PROBE/REACHABLE for switchToVerify
    //    - DYNAMIC for every other switch.
    auto switchToNdpEntries = getSwitchToNdpEntries();
    logSwitchToNdpEntries(switchToNdpEntries);
    return verifyNeighborsAllPresentOrAllAbsent(
        neighbors, switchToNdpEntries, true /* allNeighborsMustBePresent */);
  };

  return checkWithRetryErrorReturn(
      verifyNeighborPresentHelper,
      10 /* num retries */,
      std::chrono::milliseconds(1000) /* sleep between retries */,
      true /* retry on exception */);
}

bool verifyNeighborsAbsent(
    const std::set<std::string>& allSwitches,
    const std::vector<Neighbor>& neighbors,
    const std::set<std::string>& switchesToExclude) {
  auto verifyNeighborAbsentHelper = [allSwitches,
                                     neighbors,
                                     switchesToExclude] {
    auto getSwitchToAllNdpEntries = [allSwitches, switchesToExclude]() {
      std::map<std::string, std::vector<NdpEntryThrift>> switchToAllNdpEntries;
      for (const auto& switchName : allSwitches) {
        if (switchesToExclude.find(switchName) != switchesToExclude.end()) {
          continue;
        }

        switchToAllNdpEntries[switchName] = getNdpEntries(switchName);
      }

      return switchToAllNdpEntries;
    };

    auto switchToAllNdpEntries = getSwitchToAllNdpEntries();
    logSwitchToNdpEntries(switchToAllNdpEntries);
    return verifyNeighborsAllPresentOrAllAbsent(
        neighbors,
        switchToAllNdpEntries,
        false /* allNeighborsMust NOT be present */);
  };

  return checkWithRetryErrorReturn(
      verifyNeighborAbsentHelper,
      10 /* num retries */,
      std::chrono::milliseconds(1000) /* sleep between retries */,
      true /* retry on exception */);
}

bool verifyNeighborsLocallyPresent(
    const std::string& switchName,
    const std::vector<Neighbor>& neighbors) {
  auto verifyNeighborLocalPresentHelper = [switchName, neighbors]() {
    auto isLocalNeighborPresent = [switchName](const auto& neighbor) {
      auto ndpEntries = getNdpEntriesOfType(switchName, {"PROBE", "REACHABLE"});

      for (const auto& ndpEntry : ndpEntries) {
        auto ndpEntryIp = folly::IPAddress::fromBinary(
            folly::ByteRange(
                folly::StringPiece(ndpEntry.ip().value().addr().value())));

        if (ndpEntry.interfaceID().value() == neighbor.intfID &&
            ndpEntryIp == neighbor.ip) {
          return true;
        }
      }

      return false;
    };

    for (const auto& neighbor : neighbors) {
      if (isLocalNeighborPresent(neighbor)) {
        return true;
      }
    }

    return false;
  };

  return checkWithRetryErrorReturn(
      verifyNeighborLocalPresentHelper,
      10 /* num retries */,
      std::chrono::milliseconds(1000) /* sleep between retries */,
      true /* retry on exception */);
}

bool verifyNeighborsLocallyPresentRemoteAbsent(
    const std::set<std::string>& allSwitches,
    const std::vector<Neighbor>& neighbors,
    const std::string& switchName) {
  // verify neighbor entry is present on local RDSW, but absent
  // from all remote RDSWs
  if (verifyNeighborsLocallyPresent(switchName, neighbors)) {
    return verifyNeighborsAbsent(allSwitches, neighbors, {switchName});
  }

  return false;
}

} // namespace facebook::fboss::utility
