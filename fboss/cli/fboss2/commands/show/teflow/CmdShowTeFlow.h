/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#pragma once

#include <fboss/agent/if/gen-cpp2/ctrl_types.h>
#include <fboss/cli/fboss2/utils/CmdUtils.h>
#include <folly/String.h>
#include <cstddef>
#include <cstdint>
#include <type_traits>
#include "fboss/agent/if/gen-cpp2/common_types.h"
#include "fboss/cli/fboss2/CmdHandler.h"
#include "fboss/cli/fboss2/commands/show/route/utils.h"
#include "fboss/cli/fboss2/commands/show/teflow/gen-cpp2/model_types.h"

namespace facebook::fboss {

struct CmdShowTeFlowTraits : public BaseCommandTraits {
  static constexpr utils::ObjectArgTypeId ObjectArgTypeId =
      utils::ObjectArgTypeId::OBJECT_ARG_TYPE_ID_IPV6_LIST;
  using ObjectArgType = std::vector<std::string>;
  using RetType = cli::ShowTeFlowEntryModel;
};

class CmdShowTeFlow : public CmdHandler<CmdShowTeFlow, CmdShowTeFlowTraits> {
 public:
  using NextHopThrift = facebook::fboss::NextHopThrift;
  using TeFlowDetails = facebook::fboss::TeFlowDetails;

  RetType queryClient(
      const HostInfo& hostInfo,
      const ObjectArgType& queriedPrefixEntries) {
    std::vector<TeFlowDetails> entries;
    std::map<int32_t, facebook::fboss::PortInfoThrift> portInfo;
    auto client =
        utils::createClient<facebook::fboss::FbossCtrlAsyncClient>(hostInfo);

    client->sync_getTeFlowTableDetails(entries);
    client->sync_getAllPortInfo(portInfo);
    return createModel(entries, portInfo, queriedPrefixEntries);
  }

  void printOutput(const RetType& model, std::ostream& out = std::cout) {
    for (const auto& entry : model.get_flowEntries()) {
      out << fmt::format(
          "\nFlow key: dst prefix {}/{}, src port {}\n",
          entry.get_dstIp(),
          entry.get_dstIpPrefixLength(),
          entry.get_srcPortName());
      out << fmt::format("Match Action:\n");
      out << fmt::format("  Counter ID: {}\n", entry.get_counterID());
      out << fmt::format("  Redirect to Nexthops:\n");
      for (const auto& nh : entry.get_nextHops()) {
        out << fmt::format(
            "    {}\n", show::route::utils::getNextHopInfoStr(nh));
      }
      out << fmt::format("State:\n");
      out << fmt::format("  Enabled: {}\n", entry.get_enabled());
      out << fmt::format("  Resolved Nexthops:\n");
      for (const auto& nh : entry.get_resolvedNextHops()) {
        out << fmt::format(
            "    {}\n", show::route::utils::getNextHopInfoStr(nh));
      }
    }
  }

  RetType createModel(
      std::vector<facebook::fboss::TeFlowDetails>& flowEntries,
      std::map<int32_t, facebook::fboss::PortInfoThrift> portInfo,
      const ObjectArgType& queriedPrefixEntries) {
    RetType model;
    std::unordered_set<std::string> queriedSet(
        queriedPrefixEntries.begin(), queriedPrefixEntries.end());

    for (const auto& entry : flowEntries) {
      auto dstIpStr = utils::getAddrStr(*entry.flow()->dstPrefix()->ip());
      auto dstPrefix = dstIpStr + "/" +
          std::to_string(*entry.flow()->dstPrefix()->prefixLength());
      // Fill in entries if no prefix is specified or prefix specified is
      // matching
      if (queriedPrefixEntries.size() == 0 || queriedSet.count(dstPrefix)) {
        cli::TeFlowEntry flowEntry;
        flowEntry.dstIp() = dstIpStr;
        flowEntry.dstIpPrefixLength() =
            *entry.flow()->dstPrefix()->prefixLength();
        flowEntry.srcPort() = *(entry.flow()->srcPort());
        flowEntry.srcPortName_ref() =
            *portInfo[*(entry.flow()->srcPort())].name_ref();
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
};

} // namespace facebook::fboss
