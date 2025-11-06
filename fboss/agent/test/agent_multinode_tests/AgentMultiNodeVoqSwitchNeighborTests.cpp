// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/agent/test/agent_multinode_tests/AgentMultiNodeDsfUtils.h"
#include "fboss/agent/test/agent_multinode_tests/AgentMultiNodeTest.h"
#include "fboss/agent/test/agent_multinode_tests/AgentMultiNodeUtils.h"
#include "fboss/agent/test/thrift_client_utils/ThriftClientUtils.h"

namespace facebook::fboss {

class AgentMultiNodeVoqSwitchNeighborTest : public AgentMultiNodeTest {
 private:
  // For given RDSWs:
  //    - Find Up Ethernet ports
  //    - Compute InterfaceID corresponding to each of those ports
  //    - Get IP address for that InterfaceID
  //    - Add some offset to this IP to derive neighbor IP
  //    - Use lower bits of the neighbor IP to derive some neighbor MAC
  //
  // Generate specified number of neighbors and return.
  std::vector<utility::Neighbor> computeNeighborsForRdsw(
      const std::unique_ptr<utility::TopologyInfo>& topologyInfo,
      const std::string& rdsw,
      const int& numNeighbors) const {
    auto populateUpEthernetPorts = [rdsw, numNeighbors](auto& neighbors) {
      auto upEthernetPortNameToPortInfo =
          utility::getUpEthernetPortNameToPortInfo(rdsw);
      CHECK(upEthernetPortNameToPortInfo.size() >= numNeighbors);

      for (const auto& [portName, portInfo] : upEthernetPortNameToPortInfo) {
        if (neighbors.size() >= static_cast<size_t>(numNeighbors)) {
          break;
        }

        utility::Neighbor neighborInfo;
        neighborInfo.portID = *portInfo.portId();
        neighbors.push_back(neighborInfo);
      }

      return neighbors;
    };

    auto switchNameToSystemPortRanges =
        topologyInfo->getSwitchNameToSystemPortRanges();
    auto getSystemPortMin = [switchNameToSystemPortRanges, rdsw]() {
      // Get system portID range for the RDSW
      CHECK(
          switchNameToSystemPortRanges.find(rdsw) !=
          switchNameToSystemPortRanges.end());
      auto ranges = switchNameToSystemPortRanges.at(rdsw);
      // TODO: Extend to work with multiple system port ranges
      CHECK(ranges.systemPortRanges()->size() >= 1);
      auto systemPortMin = *ranges.systemPortRanges()->front().minimum();

      return systemPortMin;
    };

    auto populateIntfIDs = [rdsw, getSystemPortMin](auto& neighbors) {
      auto systemPortMin = getSystemPortMin();
      for (auto& neighbor : neighbors) {
        neighbor.intfID = int32_t(systemPortMin) + neighbor.portID;
      }
    };

    auto getIntfIDToIp = [this, rdsw]() {
      std::map<int32_t, folly::IPAddress> intfIDToIp;
      for (const auto& [_, rif] : utility::getIntfIdToIntf(rdsw)) {
        for (const auto& ipPrefix : *rif.address()) {
          auto ip = folly::IPAddress::fromBinary(
              folly::ByteRange(
                  reinterpret_cast<const unsigned char*>(
                      ipPrefix.ip()->addr()->data()),
                  ipPrefix.ip()->addr()->size()));

          if (!folly::IPAddress(ip).isLinkLocal()) {
            // Pick any one non-local IP per interface
            intfIDToIp[*rif.interfaceId()] = ip;
            break;
          }
        }
      }

      return intfIDToIp;
    };

    auto computeNeighborIpAndMac = [this](const std::string& ipAddress) {
      auto constexpr kOffset = 0x10;
      auto ipv6Address = folly::IPAddressV6::tryFromString(ipAddress);
      std::array<uint8_t, 16> bytes = ipv6Address->toByteArray();
      bytes[15] += kOffset; // add some offset to derive neighbor IP
      auto neighborIp = folly::IPAddressV6::fromBinary(bytes);

      // Resolve Neighbor to Router MAC.
      //
      // Neighbor is resolved on a loopback port.
      // Thus, packets out of this port get looped back. Since those packets
      // carry router MAC as dstMac the packets get routed and help us create
      // traffic loop.
      auto macStr =
          utility::getMacForFirstInterfaceWithPorts(getSw()->getState());
      auto neighborMac = folly::MacAddress(macStr);

      return std::make_pair(neighborIp, neighborMac);
    };

    auto populateNeighborIpAndMac =
        [rdsw, getIntfIDToIp, computeNeighborIpAndMac](auto& neighbors) {
          auto intfIDToIp = getIntfIDToIp();

          for (auto& neighbor : neighbors) {
            CHECK(intfIDToIp.find(neighbor.intfID) != intfIDToIp.end())
                << "rdsw: " << rdsw << " neighbor.intfID: " << neighbor.intfID;
            auto ip = intfIDToIp.at(neighbor.intfID);
            auto [neighborIp, neighborMac] = computeNeighborIpAndMac(ip.str());

            neighbor.ip = neighborIp;
            neighbor.mac = neighborMac;
          }
        };

    std::vector<utility::Neighbor> neighbors;
    populateUpEthernetPorts(neighbors);
    populateIntfIDs(neighbors);
    populateNeighborIpAndMac(neighbors);

    return neighbors;
  }
};

TEST_F(AgentMultiNodeVoqSwitchNeighborTest, verifyNeighborAddRemove) {}

} // namespace facebook::fboss
