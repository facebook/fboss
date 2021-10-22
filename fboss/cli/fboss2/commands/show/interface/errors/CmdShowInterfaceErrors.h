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
#include "fboss/cli/fboss2/commands/show/interface/CmdShowInterface.h"
#include "fboss/cli/fboss2/commands/show/interface/errors/gen-cpp2/model_types.h"
#include "fboss/cli/fboss2/utils/Table.h"

#include <boost/algorithm/string.hpp>
#include <unordered_set>

namespace facebook::fboss {

using utils::Table;

struct CmdShowInterfaceErrorsTraits : public BaseCommandTraits {
  using ParentCmd = CmdShowInterface;
  static constexpr utils::ObjectArgTypeId ObjectArgTypeId =
      utils::ObjectArgTypeId::OBJECT_ARG_TYPE_ID_NONE;
  using ObjectArgType = std::monostate;
  using RetType = cli::InterfaceErrorsModel;
};

class CmdShowInterfaceErrors
    : public CmdHandler<CmdShowInterfaceErrors, CmdShowInterfaceErrorsTraits> {
 public:
  using ObjectArgType = CmdShowInterfaceErrorsTraits::ObjectArgType;
  using RetType = CmdShowInterfaceErrorsTraits::RetType;

  RetType queryClient(
      const HostInfo& hostInfo,
      const std::vector<std::string>& queriedIfs) {
    RetType countersEntries;

    auto client =
        utils::createClient<facebook::fboss::FbossCtrlAsyncClient>(hostInfo);

    std::map<int32_t, facebook::fboss::PortInfoThrift> portCounters;
    client->sync_getAllPortInfo(portCounters);

    return createModel(portCounters, queriedIfs);
  }

  RetType createModel(
      std::map<int32_t, facebook::fboss::PortInfoThrift> portCounters,
      const std::vector<std::string>& queriedIfs) {
    RetType ret;

    std::unordered_set<std::string> queriedSet(
        queriedIfs.begin(), queriedIfs.end());

    for (const auto& port : portCounters) {
      auto portInfo = port.second;
      if (queriedIfs.size() == 0 || queriedSet.count(portInfo.get_name())) {
        cli::ErrorCounters counter;

        counter.interfaceName_ref() = portInfo.get_name();
        counter.inputErrors_ref() =
            portInfo.get_input().get_errors().get_errors();
        counter.inputDiscards_ref() =
            portInfo.get_input().get_errors().get_discards();
        counter.outputErrors_ref() =
            portInfo.get_output().get_errors().get_errors();
        counter.outputDiscards_ref() =
            portInfo.get_output().get_errors().get_discards();

        ret.error_counters_ref()->push_back(counter);
      }
    }

    std::sort(
        ret.error_counters_ref()->begin(),
        ret.error_counters_ref()->end(),
        [](cli::ErrorCounters& a, cli::ErrorCounters b) {
          return a.get_interfaceName() < b.get_interfaceName();
        });

    return ret;
  }

  Table::StyledCell stylizeCounterValue(int64_t counterValue) {
    return counterValue == 0
        ? Table::StyledCell(std::to_string(counterValue), Table::Style::NONE)
        : Table::StyledCell(std::to_string(counterValue), Table::Style::ERROR);
  }

  void printOutput(const RetType& model, std::ostream& out = std::cout) {
    Table table;
    table.setHeader(
        {"Interface Name",
         "Input Errors",
         "Input Discards",
         "Output Errors",
         "Output Discards"});

    for (const auto& counter : model.get_error_counters()) {
      table.addRow({
          counter.get_interfaceName(),
          stylizeCounterValue(counter.get_inputErrors()),
          stylizeCounterValue(counter.get_inputDiscards()),
          stylizeCounterValue(counter.get_outputErrors()),
          stylizeCounterValue(counter.get_outputDiscards()),
      });
    }
    out << table << std::endl;
  }
};

} // namespace facebook::fboss
