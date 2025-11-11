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

int64_t getPortOutBytes(
    const std::string& switchName,
    const std::string& portName) {
  auto counterNameToCount = getCounterNameToCount(switchName);
  auto counterName = portName + ".out_bytes.sum";
  auto iter = counterNameToCount.find(counterName);
  CHECK(iter != counterNameToCount.end());
  return iter->second;
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

bool verifyRoutePresent(
    const std::string& switchName,
    const folly::IPAddress& destPrefix,
    const int16_t prefixLength) {
  auto verifyRoutePresentHelper = [switchName, destPrefix, prefixLength]() {
    for (const auto& route : getAllRoutes(switchName)) {
      auto ip = folly::IPAddress::fromBinary(
          folly::ByteRange(
              reinterpret_cast<const unsigned char*>(
                  route.dest()->ip()->addr()->data()),
              route.dest()->ip()->addr()->size()));
      if (ip == destPrefix && *route.dest()->prefixLength() == prefixLength) {
        XLOG(DBG2) << "rdsw: " << switchName
                   << " Found route:: prefix: " << ip.str()
                   << " prefixLength: " << *route.dest()->prefixLength();
        return true;
      }
    }

    return false;
  };

  return checkWithRetryErrorReturn(
      verifyRoutePresentHelper,
      30 /* num retries */,
      std::chrono::milliseconds(1000) /* sleep between retries */,
      true /* retry on exception */);
}

bool verifyLineRate(const std::string& switchName, int32_t portID) {
  auto portIdToPortInfo = getPortIdToPortInfo(switchName);
  CHECK(portIdToPortInfo.find(portID) != portIdToPortInfo.end());
  auto portName = portIdToPortInfo[portID].name().value();
  auto portSpeedMbps = portIdToPortInfo[portID].speedMbps().value();

  // Verify is line rate is achieved i.e. in bytes within 5% of line rate.
  constexpr float kVariance = 0.05; // i.e. + or - 5%
  auto lowPortSpeedMbps = portSpeedMbps * (1 - kVariance);

  auto verifyLineRateHelper =
      [switchName, portName, portSpeedMbps, lowPortSpeedMbps]() {
        auto counterNameToCount = getCounterNameToCount(switchName);
        auto outSpeedMbps =
            counterNameToCount[portName + ".out_bytes.rate.60"] * 8 / 1000000;
        XLOG(DBG2) << "Switch: " << switchName << " portName: " << portName
                   << " portSpeedMbps: " << portSpeedMbps
                   << " lowPortSpeedMbps: " << lowPortSpeedMbps
                   << " outSpeedMbps: " << outSpeedMbps;

        return lowPortSpeedMbps < outSpeedMbps;
      };

  return checkWithRetryErrorReturn(
      verifyLineRateHelper,
      60 /* num retries, flood takes about ~30s to reach line rate */,
      std::chrono::milliseconds(1000) /* sleep between retries */,
      true /* retry on exception */);
}

bool verifyPortOutBytesIncrementByMinValue(
    const std::string& switchName,
    const std::map<std::string, int64_t>& beforePortToOutBytes,
    const int64_t& minIncrement) {
  auto verifyPortOutBytesIncrementHelper =
      [switchName, beforePortToOutBytes, minIncrement] {
        for (const auto& [port, beforeOutBytes] : beforePortToOutBytes) {
          auto afterOutBytes = getPortOutBytes(switchName, port);
          XLOG(DBG2) << "Switch:: " << switchName << "Port: " << port
                     << " before out bytes: " << beforeOutBytes
                     << " after out bytes: " << afterOutBytes;

          if (afterOutBytes < beforeOutBytes + minIncrement) {
            XLOG(DBG2) << "Traffic did not increase by at least "
                       << minIncrement << " on port: " << port
                       << " before: " << beforeOutBytes
                       << " after: " << afterOutBytes;
            return false;
          }
        }
        return true;
      };

  if (!checkWithRetryErrorReturn(
          verifyPortOutBytesIncrementHelper, 30 /* num retries */)) {
    XLOG(ERR) << "Port out bytes increment verification failed";
    return false;
  }

  return true;
}

} // namespace facebook::fboss::utility
