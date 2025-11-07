// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/agent/test/agent_multinode_tests/AgentMultiNodeUtils.h"

#include "fboss/agent/test/thrift_client_utils/ThriftClientUtils.h"
#include "fboss/lib/CommonUtils.h"

namespace {

void logNdpEntry(
    const std::string& rdsw,
    const facebook::fboss::NdpEntryThrift& ndpEntry) {
  auto ip = folly::IPAddress::fromBinary(
      folly::ByteRange(
          folly::StringPiece(ndpEntry.ip().value().addr().value())));

  XLOG(DBG2) << "From " << rdsw << " ip: " << ip.str()
             << " state: " << ndpEntry.state().value()
             << " switchId: " << ndpEntry.switchId().value_or(-1);
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

} // namespace facebook::fboss::utility
