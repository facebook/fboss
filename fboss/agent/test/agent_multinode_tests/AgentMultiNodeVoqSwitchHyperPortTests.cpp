// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/agent/test/agent_multinode_tests/AgentMultiNodeDsfUtils.h"
#include "fboss/agent/test/agent_multinode_tests/AgentMultiNodeTest.h"
#include "fboss/agent/test/agent_multinode_tests/AgentMultiNodeUtils.h"

#include "fboss/agent/test/thrift_client_utils/ThriftClientUtils.h"
#include "fboss/lib/CommonUtils.h"

namespace facebook::fboss {

class AgentMultiNodeVoqSwitchHyperPortTest : public AgentMultiNodeTest {
 private:
  // Get non-link-local IPv6 IPs for hyper port interfaces on the given EDSW.
  // Returns pairs of (interfaceID, IP) for each hyper port.
  std::vector<std::pair<int32_t, folly::IPAddressV6>> getHyperPortInterfaceIPs(
      const std::string& edsw) const {
    // Get aggregate port keys to identify hyper port interfaces
    std::vector<AggregatePortThrift> aggregatePorts;
    auto swAgentClient = utility::getSwAgentThriftClient(edsw);
    swAgentClient->sync_getAggregatePortTable(aggregatePorts);

    std::set<int32_t> aggPortKeys;
    for (const auto& aggPort : aggregatePorts) {
      aggPortKeys.insert(*aggPort.key());
    }

    // Get system port range to compute hyper port interface IDs
    auto switchNameToSystemPortRanges =
        topologyInfo_->getSwitchNameToSystemPortRanges();
    CHECK(switchNameToSystemPortRanges.count(edsw))
        << "No system port ranges for EDSW: " << edsw;
    auto ranges = switchNameToSystemPortRanges.at(edsw);
    CHECK_GE(ranges.systemPortRanges()->size(), 1);
    auto systemPortMin = *ranges.systemPortRanges()->front().minimum();

    // Hyper port intfID = systemPortMin + aggregatePortKey
    std::set<int32_t> hyperPortIntfIDs;
    for (auto key : aggPortKeys) {
      hyperPortIntfIDs.insert(int32_t(systemPortMin) + key);
    }

    // Get IPs for those interfaces
    std::vector<std::pair<int32_t, folly::IPAddressV6>> result;
    for (const auto& [_, rif] : utility::getIntfIdToIntf(edsw)) {
      if (hyperPortIntfIDs.count(*rif.interfaceId()) == 0) {
        continue;
      }
      for (const auto& ipPrefix : *rif.address()) {
        auto ip = folly::IPAddress::fromBinary(
            folly::ByteRange(
                reinterpret_cast<const unsigned char*>(
                    ipPrefix.ip()->addr()->data()),
                ipPrefix.ip()->addr()->size()));
        if (ip.isV6() && !ip.isLinkLocal()) {
          result.emplace_back(*rif.interfaceId(), ip.asV6());
          break;
        }
      }
    }

    return result;
  }

 public:
  // Verify NDP resolution behind hyper ports using ping.
  // From the test driver EDSW, ping each hyper port interface IP on the
  // remote EDSW to trigger NDP resolution, then verify NDP entries are
  // REACHABLE on the test driver.
  //
  // Example: edsw1_1 has 3101::1/64 on hyp1/1/1, edsw1_2 has 3101::2/64
  // on hyp1/1/1. Pinging 3101::2 from edsw1_1 resolves NDP for 3101::2
  // on edsw1_1's hyp1/1/1 interface.
  bool verifyNdpBehindHyperPorts(
      const std::unique_ptr<utility::TopologyInfo>& topologyInfo) const {
    XLOG(DBG2) << "Verifying NDP behind hyper ports via ping";

    auto myHostname = topologyInfo->getMyHostname();
    auto localIPs = getHyperPortInterfaceIPs(myHostname);
    CHECK(!localIPs.empty()) << "No hyper port IPs on test driver";

    // Find remote EDSW
    std::string remoteEdsw;
    for (const auto& edsw : topologyInfo->getEdsws()) {
      if (edsw != myHostname) {
        remoteEdsw = edsw;
        break;
      }
    }
    CHECK(!remoteEdsw.empty()) << "No remote EDSW found";
    auto remoteIPs = getHyperPortInterfaceIPs(remoteEdsw);
    CHECK(!remoteIPs.empty()) << "No hyper port IPs on remote EDSW";

    // Ping each remote hyper port IP from the matching local IP
    // (same /64 subnet). This triggers NDP resolution on the local EDSW.
    for (const auto& [remoteIntfID, remoteIP] : remoteIPs) {
      // Find local IP in same /64 subnet
      std::optional<folly::IPAddressV6> localIP;
      for (const auto& [localIntfID, lip] : localIPs) {
        if (lip.mask(64) == remoteIP.mask(64)) {
          localIP = lip;
          break;
        }
      }
      CHECK(localIP.has_value())
          << "No local IP in same subnet as " << remoteIP.str();

      auto cmd =
          "ping -6 -c 5 -W 2 -I " + localIP->str() + " " + remoteIP.str();
      XLOG(DBG2) << "Pinging: " << cmd;
      auto ret = std::system(cmd.c_str());
      if (ret != 0) {
        XLOG(DBG2) << "Ping failed with exit code " << ret << ": " << cmd;
        return false;
      }
    }

    // Verify NDP entries for all remote IPs are REACHABLE on local EDSW
    auto verifyNdpHelper = [&myHostname, &remoteIPs]() {
      auto ndpEntries = utility::getNdpEntriesOfType(myHostname, {"REACHABLE"});
      XLOG(DBG2) << "NDP REACHABLE entries on " << myHostname << ":";
      for (const auto& ndpEntry : ndpEntries) {
        auto ip = folly::IPAddress::fromBinary(
            folly::ByteRange(
                folly::StringPiece(ndpEntry.ip().value().addr().value())));
        XLOG(DBG2) << "  ip=" << ip.str() << " mac=" << ndpEntry.mac().value()
                   << " intfID=" << ndpEntry.interfaceID().value()
                   << " state=" << ndpEntry.state().value();
      }
      for (const auto& [_, remoteIP] : remoteIPs) {
        bool found = false;
        for (const auto& ndpEntry : ndpEntries) {
          auto entryIP = folly::IPAddress::fromBinary(
              folly::ByteRange(
                  folly::StringPiece(ndpEntry.ip().value().addr().value())));
          if (entryIP == remoteIP) {
            found = true;
            break;
          }
        }
        if (!found) {
          XLOG(DBG2) << "NDP entry not REACHABLE for " << remoteIP.str()
                     << " on " << myHostname;
          return false;
        }
      }
      return true;
    };

    return checkWithRetryErrorReturn(
        verifyNdpHelper,
        10 /* num retries */,
        std::chrono::milliseconds(1000) /* sleep between retries */,
        true /* retry on exception */);
  }
};

TEST_F(AgentMultiNodeVoqSwitchHyperPortTest, verifyNdpBehindHyperPorts) {
  if (!FLAGS_hyper_port) {
    GTEST_SKIP() << "Skipping: hyper_port flag is not set";
  }
  runTestWithVerifyCluster([this](const auto& topologyInfo) {
    switch (topologyInfo->getTopologyType()) {
      case utility::TopologyInfo::TopologyType::DSF:
        return this->verifyNdpBehindHyperPorts(topologyInfo);
    }
  });
}

} // namespace facebook::fboss
