// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/cli/fboss2/commands/show/mysid/CmdShowMySid.h"
#include "fboss/cli/fboss2/CmdHandler.cpp"

#include <unordered_map>

namespace facebook::fboss {

namespace {

// Helper struct for indexing ARP/NDP entries by IP and interface ID
struct IpIntfKey {
  std::string ip;
  int32_t intfId;

  bool operator<(const IpIntfKey& other) const {
    if (ip != other.ip) {
      return ip < other.ip;
    }
    return intfId < other.intfId;
  }
};

// Helper function to build IP+Interface to PortDescriptor map from ARP/NDP
// tables
std::map<IpIntfKey, cfg::PortDescriptor> buildIpIntfToPortDescMap(
    const std::vector<ArpEntryThrift>& arpTable,
    const std::vector<NdpEntryThrift>& ndpTable) {
  std::map<IpIntfKey, cfg::PortDescriptor> ipIntfToPortDesc;

  // Lambda to process entries from both ARP and NDP tables
  auto processEntries = [&](const auto& table) {
    for (const auto& entry : table) {
      auto ip = network::toIPAddress(entry.ip().value()).str();
      int32_t intfId = entry.interfaceID().value();
      ipIntfToPortDesc[{ip, intfId}] = entry.portDescriptor().value();
    }
  };

  processEntries(arpTable);
  processEntries(ndpTable);

  return ipIntfToPortDesc;
}

// Helper function to build port ID to port name map
std::unordered_map<int32_t, std::string> buildPortIdToNameMap(
    const std::map<int32_t, PortInfoThrift>& portInfo) {
  std::unordered_map<int32_t, std::string> portIdToName;
  for (const auto& [portId, pInfo] : portInfo) {
    portIdToName[portId] = pInfo.name().value();
  }
  return portIdToName;
}

// Helper function to build aggregate port ID to name map
std::unordered_map<int32_t, std::string> buildAggPortIdToNameMap(
    const std::vector<AggregatePortThrift>& aggregatePorts) {
  std::unordered_map<int32_t, std::string> aggPortIdToName;
  for (const auto& aggPort : aggregatePorts) {
    aggPortIdToName[aggPort.key().value()] = aggPort.name().value();
  }
  return aggPortIdToName;
}

// Helper function to get port name from port descriptor
std::string getPortNameFromDescriptor(
    const cfg::PortDescriptor& portDesc,
    const std::unordered_map<int32_t, std::string>& portIdToName,
    const std::unordered_map<int32_t, std::string>& aggPortIdToName,
    const std::string& ifName = "") {
  int32_t portId = portDesc.portId().value();
  auto portType = portDesc.portType().value();
  std::string portName;

  switch (portType) {
    case cfg::PortDescriptorType::Aggregate: {
      auto aggPortIt = aggPortIdToName.find(portId);
      if (aggPortIt != aggPortIdToName.end()) {
        portName = aggPortIt->second;
      } else {
        portName = fmt::format("{}", portId);
      }
      break;
    }
    case cfg::PortDescriptorType::Physical: {
      auto portNameIt = portIdToName.find(portId);
      if (portNameIt != portIdToName.end()) {
        portName = portNameIt->second;
      } else {
        portName = fmt::format("{}", portId);
      }
      break;
    }
    case cfg::PortDescriptorType::SystemPort:
      throw std::runtime_error(
          "SystemPort type not supported for MySid egress resolution");
  }

  // Append interface name in parentheses if available
  if (!ifName.empty()) {
    portName += fmt::format(", {}", ifName);
  }

  return portName;
}

} // namespace

CmdShowMySid::RetType CmdShowMySid::queryClient(const HostInfo& hostInfo) {
  auto client =
      utils::createClient<apache::thrift::Client<FbossCtrl>>(hostInfo);
  std::vector<MySidEntry> entries;
  client->sync_getMySidEntries(entries);

  // Fetch ARP and NDP tables to resolve egress ports for next hops
  std::vector<ArpEntryThrift> arpTable;
  std::vector<NdpEntryThrift> ndpTable;
  client->sync_getArpTable(arpTable);
  client->sync_getNdpTable(ndpTable);

  // Fetch port info and aggregate ports to resolve port names
  std::map<int32_t, PortInfoThrift> portInfo;
  std::vector<AggregatePortThrift> aggregatePorts;
  client->sync_getAllPortInfo(portInfo);
  client->sync_getAggregatePortTable(aggregatePorts);

  return createModel(entries, arpTable, ndpTable, portInfo, aggregatePorts);
}

CmdShowMySid::RetType CmdShowMySid::createModel(
    const std::vector<MySidEntry>& entries,
    const std::vector<ArpEntryThrift>& arpTable,
    const std::vector<NdpEntryThrift>& ndpTable,
    const std::map<int32_t, PortInfoThrift>& portInfo,
    const std::vector<AggregatePortThrift>& aggregatePorts) {
  RetType model;

  // Build maps using helper functions
  auto ipIntfToPortDesc = buildIpIntfToPortDescMap(arpTable, ndpTable);
  auto portIdToName = buildPortIdToNameMap(portInfo);
  auto aggPortIdToName = buildAggPortIdToNameMap(aggregatePorts);

  for (const auto& entry : entries) {
    cli::MySidEntryModel entryModel;

    // Format prefix as addr/len
    const auto& mySid = entry.mySid().value();
    auto prefixAddr = network::toIPAddress(mySid.prefixAddress().value()).str();
    entryModel.prefix() =
        prefixAddr + "/" + std::to_string(mySid.prefixLength().value());

    entryModel.type() =
        apache::thrift::util::enumNameSafe(entry.type().value());

    // Only show nexthop for NODE and BINDING type mysids
    switch (entry.type().value()) {
      case MySidType::NODE_MICRO_SID:
      case MySidType::BINDING_MICRO_SID:
        for (const auto& nh : entry.nextHops().value()) {
          entryModel.nextHops()->push_back(
              network::toIPAddress(nh.address().value()).str());
        }
        break;
      case MySidType::ADJACENCY_MICRO_SID:
      case MySidType::DECAPSULATE_AND_LOOKUP:
        // Do not show nexthop for these types
        break;
    }
    // Continue showing resolvedNextHops for all if present
    // Include egress port information if available
    for (const auto& nh : entry.resolvedNextHops().value()) {
      auto nhIp = network::toIPAddress(nh.address().value()).str();

      // Try to get interface name from the address if available
      bool found = false;
      cfg::PortDescriptor portDesc;
      std::string ifNameStr;
      auto ifNamePtr =
          apache::thrift::get_pointer(nh.address().value().ifName());
      if (ifNamePtr != nullptr) {
        ifNameStr = *ifNamePtr;
        // Address has interface name, extract interface ID
        // Format is typically "fboss<id>" or just "<id>"
        std::string ifName = *ifNamePtr;
        // Remove "fboss" prefix if present
        if (ifName.find("fboss") == 0) {
          ifName = ifName.substr(5);
        }
        try {
          int32_t intfId = folly::to<int32_t>(ifName);
          auto key = IpIntfKey{nhIp, intfId};
          auto it = ipIntfToPortDesc.find(key);
          if (it != ipIntfToPortDesc.end()) {
            portDesc = it->second;
            found = true;
          }
        } catch (const std::exception&) {
          // Failed to parse interface ID
        }
      }

      if (found) {
        // Found in ARP/NDP table, get port name using helper function
        std::string portName = getPortNameFromDescriptor(
            portDesc, portIdToName, aggPortIdToName, ifNameStr);
        entryModel.resolvedNextHops()->push_back(
            fmt::format("{} via {}", nhIp, portName));
      } else {
        // Port not found, just show IP
        entryModel.resolvedNextHops()->push_back(nhIp);
      }
    }

    model.mySidEntries()->push_back(std::move(entryModel));
  }
  return model;
}

void CmdShowMySid::printOutput(const RetType& model, std::ostream& out) {
  for (const auto& entry : model.mySidEntries().value()) {
    out << fmt::format(
        "MySid: {}  Type: {}\n", entry.prefix().value(), entry.type().value());
    for (const auto& nh : entry.nextHops().value()) {
      out << fmt::format("\tdestination {}\n", nh);
    }
    for (const auto& nh : entry.resolvedNextHops().value()) {
      out << fmt::format("\tresolved via {}\n", nh);
    }
  }
}

// Explicit template instantiation
template void CmdHandler<CmdShowMySid, CmdShowMySidTraits>::run();

} // namespace facebook::fboss
