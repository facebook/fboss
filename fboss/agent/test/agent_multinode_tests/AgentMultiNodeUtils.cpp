// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/agent/test/agent_multinode_tests/AgentMultiNodeUtils.h"

#include "fboss/agent/test/thrift_client_utils/ThriftClientUtils.h"
#include "fboss/lib/CommonUtils.h"

namespace facebook::fboss::utility {

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

} // namespace facebook::fboss::utility
