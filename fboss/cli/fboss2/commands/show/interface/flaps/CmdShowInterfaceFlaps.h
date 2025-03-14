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
#include "fboss/cli/fboss2/commands/show/interface/flaps/gen-cpp2/model_types.h"
#include "fboss/cli/fboss2/utils/Table.h"

#include <boost/algorithm/string.hpp>
#include <unordered_set>

namespace facebook::fboss {

using utils::Table;

struct CmdShowInterfaceFlapsTraits : public BaseCommandTraits {
  using ParentCmd = CmdShowInterface;
  static constexpr utils::ObjectArgTypeId ObjectArgTypeId =
      utils::ObjectArgTypeId::OBJECT_ARG_TYPE_ID_NONE;
  using ObjectArgType = std::monostate;
  using RetType = cli::InterfaceFlapsModel;
  static constexpr bool ALLOW_FILTERING = true;
  static constexpr bool ALLOW_AGGREGATION = true;
};

class CmdShowInterfaceFlaps
    : public CmdHandler<CmdShowInterfaceFlaps, CmdShowInterfaceFlapsTraits> {
 public:
  using ObjectArgType = CmdShowInterfaceFlapsTraits::ObjectArgType;
  using RetType = CmdShowInterfaceFlapsTraits::RetType;

  RetType queryClient(
      const HostInfo& hostInfo,
      const std::vector<std::string>& queriedIfs) {
    /* Interface flap stats are stored as a FB303 counter.  Will call
       getRegexCounters so we can filter out just the interface counters and
       ignore the multitude of other counters we don't need.
    */
    RetType flapsEntries;

    auto client =
        utils::createClient<facebook::fboss::FbossCtrlAsyncClient>(hostInfo);

    std::map<std::string, int64_t> wedgeCounters;
#ifdef IS_OSS
    // TODO - figure out why getRegexCounters fails for OSS
    client->sync_getCounters(wedgeCounters);
#else
    client->sync_getRegexCounters(wedgeCounters, "^(eth|fab).*flap.sum.*");
#endif
    std::unordered_set<std::string> distinctInterfaceNames;
    for (const auto& counter : wedgeCounters) {
      std::vector<std::string> result;
      boost::split(result, counter.first, boost::is_any_of("."));
      distinctInterfaceNames.insert(result[0]);
    }
    std::vector<std::string> ifNames(
        distinctInterfaceNames.begin(), distinctInterfaceNames.end());
    return createModel(ifNames, wedgeCounters, queriedIfs);
  }

  RetType createModel(
      std::vector<std::string>& ifNames,
      std::map<std::string, std::int64_t> wedgeCounters,
      const std::vector<std::string>& queriedIfs) {
    RetType ret;

    std::unordered_set<std::string> queriedSet(
        queriedIfs.begin(), queriedIfs.end());

    for (std::string interface : ifNames) {
      if (queriedIfs.size() == 0 || queriedSet.count(interface)) {
        cli::FlapCounters counter;

        std::string oneMinute = interface + ".link_state.flap.sum.60";
        std::string tenMinutes = interface + ".link_state.flap.sum.600";
        std::string oneHour = interface + ".link_state.flap.sum.3600";
        std::string totalFlaps = interface + ".link_state.flap.sum";

        counter.interfaceName() = interface;
        counter.oneMinute() = wedgeCounters[oneMinute];
        counter.tenMinute() = wedgeCounters[tenMinutes];
        counter.oneHour() = wedgeCounters[oneHour];
        counter.totalFlaps() = wedgeCounters[totalFlaps];

        ret.flap_counters()->push_back(counter);
      }
    }

    std::sort(
        ret.flap_counters()->begin(),
        ret.flap_counters()->end(),
        [](cli::FlapCounters& a, cli::FlapCounters b) {
          return a.interfaceName().value() < b.interfaceName().value();
        });

    return ret;
  }

  Table::Style get_FlapStyle(int64_t counter) {
    if (counter == 0) {
      return Table::Style::NONE;
    } else if (counter > 0 && counter < 100) {
      return Table::Style::WARN;
    } else {
      return Table::Style::ERROR;
    }
  }

  void printOutput(const RetType& model, std::ostream& out = std::cout) {
    Table table;
    table.setHeader(
        {"Interface Name",
         "1 Min",
         "10 Min",
         "60 Min",
         "Total (since last reboot)"});

    for (const auto& counter : model.flap_counters().value()) {
      table.addRow({
          counter.interfaceName().value(),
          Table::StyledCell(
              std::to_string(folly::copy(counter.oneMinute().value())),
              get_FlapStyle(folly::copy(counter.oneMinute().value()))),
          Table::StyledCell(
              std::to_string(folly::copy(counter.tenMinute().value())),
              get_FlapStyle(folly::copy(counter.tenMinute().value()))),
          Table::StyledCell(
              std::to_string(folly::copy(counter.oneHour().value())),
              get_FlapStyle(folly::copy(counter.oneHour().value()))),
          Table::StyledCell(
              std::to_string(folly::copy(counter.totalFlaps().value())),
              get_FlapStyle(folly::copy(counter.totalFlaps().value()))),
      });
    }

    out << table << std::endl;
  }
};

} // namespace facebook::fboss
