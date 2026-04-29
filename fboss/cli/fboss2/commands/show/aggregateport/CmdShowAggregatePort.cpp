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
#include "fboss/agent/if/gen-cpp2/FbossCtrl.h"
#include "fboss/agent/if/gen-cpp2/FbossCtrlAsyncClient.h"
#include "fboss/cli/fboss2/CmdHandler.cpp"

#include <cstdint>
#include <map>
#include <ostream>
#include <string>
#include <unordered_set>
#include <utility>
#include <vector>
#include "fboss/agent/if/gen-cpp2/ctrl_types.h"
#include "fboss/cli/fboss2/commands/show/aggregateport/gen-cpp2/model_types.h"
#include "fboss/cli/fboss2/utils/CmdClientUtilsCommon.h"
#include "fboss/cli/fboss2/utils/HostInfo.h"
#include "fmt/format.h"
#include "folly/Utility.h"

namespace facebook::fboss {

using RetType = CmdShowAggregatePortTraits::RetType;

RetType CmdShowAggregatePort::queryClient(
    const HostInfo& hostInfo,
    const ObjectArgType& queriedPorts) {
  std::vector<facebook::fboss::AggregatePortThrift> entries;
  std::map<int32_t, facebook::fboss::PortInfoThrift> portInfo;
  auto client =
      utils::createClient<apache::thrift::Client<FbossCtrl>>(hostInfo);

  client->sync_getAggregatePortTable(entries);
  client->sync_getAllPortInfo(portInfo);
  return createModel(entries, std::move(portInfo), queriedPorts);
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
  }
}

RetType CmdShowAggregatePort::createModel(
    const std::vector<facebook::fboss::AggregatePortThrift>&
        aggregatePortEntries,
    std::map<int32_t, facebook::fboss::PortInfoThrift> portInfo,
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
