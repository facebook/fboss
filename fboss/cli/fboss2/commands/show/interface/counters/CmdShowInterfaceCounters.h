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

#include "fboss/cli/fboss2/CmdHandler.h"
#include "fboss/cli/fboss2/commands/show/interface/counters/gen-cpp2/model_types.h"
#include "fboss/cli/fboss2/utils/Table.h"

#include <boost/algorithm/string.hpp>
#include <unordered_set>

namespace facebook::fboss {

using utils::Table;

struct CmdShowInterfaceCountersTraits {
  static constexpr utils::ObjectArgTypeId ObjectArgTypeId =
      utils::ObjectArgTypeId::OBJECT_ARG_TYPE_ID_PORT_LIST;
  using ObjectArgType = std::vector<std::string>;
  using RetType = cli::InterfaceCountersModel;
};

class CmdShowInterfaceCounters
    : public CmdHandler<CmdShowInterfaceCounters, CmdShowInterfaceCountersTraits> {
 public:
  using ObjectArgType = CmdShowInterfaceCountersTraits::ObjectArgType;
  using RetType = CmdShowInterfaceCountersTraits::RetType;

  RetType queryClient(
      const HostInfo& hostInfo,
      const ObjectArgType& queriedIfs) {

    RetType countersEntries;

    auto client =
        utils::createClient<facebook::fboss::FbossCtrlAsyncClient>(hostInfo);

    std::map<int32_t, facebook::fboss::PortInfoThrift> portCounters;
    client->sync_getAllPortInfo(portCounters);

    return createModel(portCounters, queriedIfs);
  }

  RetType createModel(
    std::map<int32_t, facebook::fboss::PortInfoThrift> portCounters,
    const ObjectArgType& queriedIfs) {
    RetType ret;

    std::unordered_set<std::string> queriedSet(
      queriedIfs.begin(), queriedIfs.end()
    );

    for (const auto& port : portCounters) {
      auto portInfo = port.second;
      if (queriedIfs.size() == 0 || queriedSet.count(portInfo.get_name())) {
        cli::InterfaceCounters counter;

        counter.interfaceName_ref() = portInfo.get_name();
        counter.inputBytes_ref() = portInfo.get_input().get_bytes();
        counter.inputUcastPkts_ref() = portInfo.get_input().get_ucastPkts();
        counter.inputMulticastPkts_ref() = portInfo.get_input().get_multicastPkts();
        counter.inputBroadcastPkts_ref() = portInfo.get_input().get_broadcastPkts();
        counter.outputBytes_ref() = portInfo.get_output().get_bytes();
        counter.outputUcastPkts_ref() = portInfo.get_output().get_ucastPkts();
        counter.outputMulticastPkts_ref() = portInfo.get_output().get_multicastPkts();
        counter.outputBroadcastPkts_ref() = portInfo.get_output().get_broadcastPkts();

        ret.int_counters_ref()->push_back(counter);
      }
    }

    std::sort(
      ret.int_counters_ref()->begin(),
      ret.int_counters_ref()->end(),
      [](cli::InterfaceCounters& a, cli::InterfaceCounters b) {
        return a.get_interfaceName() < b.get_interfaceName();
      }
    );

    return ret;
  }

  void printOutput(const RetType& model, std::ostream& out = std::cout) {
    Table table;
    table.setHeader(
      {
        "Interface Name", "Bytes(in)", "Unicast Pkts(in)", "Multicast Pkts(in)",
        "Broadcast Pkts(in)", "Bytes(out)", "Unicast Pkts(out)", "Multicast Pkts(out)",
        "Broadcast Pkts(out)"
      }
    );

    for (const auto& counter : model.get_int_counters()) {
      table.addRow({
          counter.get_interfaceName(),
          std::to_string(counter.get_inputBytes()),
          std::to_string(counter.get_inputUcastPkts()),
          std::to_string(counter.get_inputMulticastPkts()),
          std::to_string(counter.get_inputBroadcastPkts()),
          std::to_string(counter.get_outputBytes()),
          std::to_string(counter.get_outputUcastPkts()),
          std::to_string(counter.get_outputMulticastPkts()),
          std::to_string(counter.get_outputBroadcastPkts()),

      });
    }

    out << table << std::endl;
  }
};

} // namespace facebook::fboss
