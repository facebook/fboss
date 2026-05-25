/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "CmdShowAggregatePort.h"
#include "fboss/cli/fboss2/CmdHandler.cpp"

#include <unordered_set>
#include "fboss/agent/if/gen-cpp2/ctrl_types.h"
#include "fmt/format.h"
#include "folly/Conv.h"
#include "folly/IPAddress.h"

namespace facebook::fboss {

using RetType = CmdShowAggregatePortTraits::RetType;

RetType CmdShowAggregatePort::queryClient(
    const HostInfo& hostInfo,
    const ObjectArgType& queriedPorts) {
  std::vector<facebook::fboss::AggregatePortThrift> entries;
  std::map<int32_t, facebook::fboss::PortInfoThrift> portInfo;
  std::map<int32_t, facebook::fboss::InterfaceDetail> interfaces;
  auto client =
      utils::createClient<apache::thrift::Client<FbossCtrl>>(hostInfo);

  client->sync_getAggregatePortTable(entries);
  client->sync_getAllPortInfo(portInfo);
  client->sync_getAllInterfaces(interfaces);
  return createModel(entries, std::move(portInfo), interfaces, queriedPorts);
}

void CmdShowAggregatePort::printOutput(
    const RetType& model,
    std::ostream& out) {
  for (const auto& entry : model.aggregatePortEntries().value()) {
    out << fmt::format("\nPort name: {}\n", entry.name().value());
    out << fmt::format("Description: {}\n", entry.description().value());
    if (entry.minMembersToUp().has_value()) {
      out << fmt::format(
          "Active members/Configured members/Min members: {}/{}/[{}, {}]\n",
          folly::copy(entry.activeMembers().value()),
          folly::copy(entry.configuredMembers().value()),
          folly::copy(entry.minMembers().value()),
          folly::copy(entry.minMembersToUp().value()));
    } else {
      out << fmt::format(
          "Active members/Configured members/Min members: {}/{}/{}\n",
          folly::copy(entry.activeMembers().value()),
          folly::copy(entry.configuredMembers().value()),
          folly::copy(entry.minMembers().value()));
    }
    for (const auto& member : entry.members().value()) {
      out << fmt::format(
          "\t Member: {:>10}, id: {:>3}, Link: {:>5}, Fwding: {:>5}, Rate: {}\n",
          member.name().value(),
          folly::copy(member.id().value()),
          folly::copy(member.isLinkUp().value()) ? "Up" : "Down",
          folly::copy(member.isUp().value()) ? "True" : "False",
          member.lacpRate().value());
    }
    if (!entry.rifs()->empty()) {
      out << "RIFs:\n";
      for (const auto& rif : entry.rifs().value()) {
        out << fmt::format(
            "\t OS Interface: {}, ID: {}\n",
            rif.osIfName().value(),
            rif.rifID().value());
        if (!rif.addresses()->empty()) {
          out << "\t Addresses:\n";
          for (const auto& addr : rif.addresses().value()) {
            out << fmt::format("\t\t {}\n", addr);
          }
        }
      }
    }
  }
}

RetType CmdShowAggregatePort::createModel(
    const std::vector<facebook::fboss::AggregatePortThrift>&
        aggregatePortEntries,
    std::map<int32_t, facebook::fboss::PortInfoThrift> portInfo,
    const std::map<int32_t, facebook::fboss::InterfaceDetail>& interfaces,
    const ObjectArgType& queriedPorts) {
  RetType model;
  std::unordered_set<std::string> queriedSet(
      queriedPorts.begin(), queriedPorts.end());

  for (const auto& entry : aggregatePortEntries) {
    const auto& portName = *entry.name();
    if (queriedPorts.size() == 0 || queriedSet.count(portName)) {
      cli::AggregatePortEntry aggPortDetails;
      aggPortDetails.name() = portName;
      aggPortDetails.description() = *entry.description();
      aggPortDetails.minMembers() = *entry.minimumLinkCount();
      aggPortDetails.configuredMembers() = entry.memberPorts()->size();
      if (auto minLinkCountToUp = entry.minimumLinkCountToUp()) {
        aggPortDetails.minMembersToUp() = *minLinkCountToUp;
      }
      int32_t activeMemberCount = 0;
      for (const auto& subport : *entry.memberPorts()) {
        cli::AggregateMemberPortEntry memberDetails;
        memberDetails.id() = *subport.memberPortID();
        memberDetails.name() = *portInfo[*subport.memberPortID()].name();
        memberDetails.isUp() = *subport.isForwarding();
        memberDetails.isLinkUp() =
            *portInfo[*subport.memberPortID()].operState() == PortOperState::UP;
        memberDetails.lacpRate() =
            *subport.rate() == LacpPortRateThrift::FAST ? "Fast" : "Slow";
        if (*subport.isForwarding()) {
          activeMemberCount++;
        }
        aggPortDetails.members()->push_back(memberDetails);
      }
      aggPortDetails.activeMembers() = activeMemberCount;

      // Find RIFs associated with this aggregate port by checking the first
      // member port
      if (!entry.memberPorts()->empty()) {
        auto firstMemberPortId = *entry.memberPorts()->front().memberPortID();
        auto firstMemberPortName = portInfo[firstMemberPortId].name();
        for (const auto& [rifId, intf] : interfaces) {
          if (!intf.portNames()->empty()) {
            for (const auto& intfPortName : *intf.portNames()) {
              if (intfPortName == firstMemberPortName) {
                cli::AggregatePortRifEntry rifEntry;
                rifEntry.rifID() = *intf.interfaceId();
                rifEntry.osIfName() =
                    fmt::format("fboss{}", *intf.interfaceId());
                for (const auto& addr : *intf.address()) {
                  auto ip = folly::IPAddress::fromBinary(
                      folly::ByteRange(
                          reinterpret_cast<const unsigned char*>(
                              addr.ip()->addr()->data()),
                          addr.ip()->addr()->size()));
                  rifEntry.addresses()->push_back(
                      folly::to<std::string>(
                          ip.str(), "/", *addr.prefixLength()));
                }
                aggPortDetails.rifs()->push_back(rifEntry);
                break;
              }
            }
          }
        }
      }

      model.aggregatePortEntries()->push_back(aggPortDetails);
    }
  }
  return model;
}

// Explicit template instantiation
template void
CmdHandler<CmdShowAggregatePort, CmdShowAggregatePortTraits>::run();
template const ValidFilterMapType
CmdHandler<CmdShowAggregatePort, CmdShowAggregatePortTraits>::getValidFilters();

} // namespace facebook::fboss
