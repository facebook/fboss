/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/cli/fboss2/commands/show/teflow/CmdShowTeFlow.h"

namespace facebook::fboss {

CmdShowTeFlow::RetType CmdShowTeFlow::queryClient(
    const HostInfo& hostInfo,
    const ObjectArgType& queriedPrefixEntries) {
  std::vector<TeFlowDetails> entries;
  std::map<int32_t, facebook::fboss::PortInfoThrift> portInfo;
  auto client =
      utils::createClient<apache::thrift::Client<FbossCtrl>>(hostInfo);

  client->sync_getTeFlowTableDetails(entries);
  client->sync_getAllPortInfo(portInfo);
  return createModel(entries, portInfo, queriedPrefixEntries);
}

void CmdShowTeFlow::printOutput(const RetType& model, std::ostream& out) {
  for (const auto& entry : model.flowEntries().value()) {
    out << fmt::format(
        "\nFlow key: dst prefix {}/{}, src port {}\n",
        entry.dstIp().value(),
        folly::copy(entry.dstIpPrefixLength().value()),
        entry.srcPortName().value());
    out << fmt::format("Match Action:\n");
    out << fmt::format("  Counter ID: {}\n", entry.counterID().value());
    out << fmt::format("  Redirect to Nexthops:\n");
    for (const auto& nh : entry.nextHops().value()) {
      out << fmt::format("    {}\n", show::route::utils::getNextHopInfoStr(nh));
    }
    out << fmt::format("State:\n");
    out << fmt::format("  Enabled: {}\n", folly::copy(entry.enabled().value()));
    out << fmt::format("  Resolved Nexthops:\n");
    for (const auto& nh : entry.resolvedNextHops().value()) {
      out << fmt::format("    {}\n", show::route::utils::getNextHopInfoStr(nh));
    }
  }
}

CmdShowTeFlow::RetType CmdShowTeFlow::createModel(
    std::vector<facebook::fboss::TeFlowDetails>& flowEntries,
    std::map<int32_t, facebook::fboss::PortInfoThrift> portInfo,
    const ObjectArgType& queriedPrefixEntries) {
  RetType model;
  std::unordered_set<std::string> queriedSet(
      queriedPrefixEntries.begin(), queriedPrefixEntries.end());

  for (const auto& entry : flowEntries) {
    auto dstIpStr = utils::getAddrStr(
        *apache::thrift::can_throw(*entry.flow()->dstPrefix()).ip());
    auto dstPrefix = dstIpStr + "/" +
        std::to_string(*apache::thrift::can_throw(*entry.flow()->dstPrefix())
                            .prefixLength());
    // Fill in entries if no prefix is specified or prefix specified is
    // matching
    if (queriedPrefixEntries.size() == 0 || queriedSet.count(dstPrefix)) {
      cli::TeFlowEntry flowEntry;
      flowEntry.dstIp() = dstIpStr;
      flowEntry.dstIpPrefixLength() =
          *apache::thrift::can_throw(*entry.flow()->dstPrefix()).prefixLength();
      flowEntry.srcPort() = *(entry.flow()->srcPort());
      flowEntry.srcPortName() = *portInfo[*(entry.flow()->srcPort())].name();
      flowEntry.enabled() = *(entry.enabled());
      if (entry.counterID()) {
        flowEntry.counterID() = *(entry.counterID());
      }
      for (const auto& nhop : *(entry.nexthops())) {
        cli::NextHopInfo nhInfo;
        show::route::utils::getNextHopInfoAddr(*nhop.address(), nhInfo);
        nhInfo.weight() = *nhop.weight();
        flowEntry.nextHops()->emplace_back(nhInfo);
      }
      for (const auto& nhop : *(entry.resolvedNexthops())) {
        cli::NextHopInfo nhInfo;
        show::route::utils::getNextHopInfoAddr(*nhop.address(), nhInfo);
        nhInfo.weight() = *nhop.weight();
        flowEntry.resolvedNextHops()->emplace_back(nhInfo);
      }

      model.flowEntries()->emplace_back(flowEntry);
    }
  }
  return model;
}

} // namespace facebook::fboss
