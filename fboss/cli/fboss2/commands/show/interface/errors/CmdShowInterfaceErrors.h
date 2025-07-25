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

struct CmdShowInterfaceErrorsTraits : public ReadCommandTraits {
  using ParentCmd = CmdShowInterface;
  static constexpr utils::ObjectArgTypeId ObjectArgTypeId =
      utils::ObjectArgTypeId::OBJECT_ARG_TYPE_ID_NONE;
  using ObjectArgType = std::monostate;
  using RetType = cli::InterfaceErrorsModel;
  static constexpr bool ALLOW_FILTERING = true;
  static constexpr bool ALLOW_AGGREGATION = true;
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
      if (queriedIfs.size() == 0 || queriedSet.count(portInfo.name().value())) {
        cli::ErrorCounters counter;

        counter.interfaceName() = portInfo.name().value();
        counter.inputErrors() = folly::copy(
            portInfo.input().value().errors().value().errors().value());
        counter.inputDiscards() = folly::copy(
            portInfo.input().value().errors().value().discards().value());
        counter.outputErrors() = folly::copy(
            portInfo.output().value().errors().value().errors().value());
        counter.outputDiscards() = folly::copy(
            portInfo.output().value().errors().value().discards().value());

        ret.error_counters()->push_back(counter);
      }
    }

    std::sort(
        ret.error_counters()->begin(),
        ret.error_counters()->end(),
        [](cli::ErrorCounters& a, cli::ErrorCounters b) {
          return a.interfaceName().value() < b.interfaceName().value();
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

    for (const auto& counter : model.error_counters().value()) {
      table.addRow({
          counter.interfaceName().value(),
          stylizeCounterValue(folly::copy(counter.inputErrors().value())),
          stylizeCounterValue(folly::copy(counter.inputDiscards().value())),
          stylizeCounterValue(folly::copy(counter.outputErrors().value())),
          stylizeCounterValue(folly::copy(counter.outputDiscards().value())),
      });
    }
    out << table << std::endl;
  }
};

} // namespace facebook::fboss
