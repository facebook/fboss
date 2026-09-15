/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/cli/fboss2/commands/show/route/CmdShowRouteCounters.h"
#include "fboss/cli/fboss2/CmdHandler.cpp"

#include "fboss/cli/fboss2/utils/CmdClientUtils.h"
#include "fboss/cli/fboss2/utils/Table.h"

namespace facebook::fboss {

using utils::Table;

CmdShowRouteCounters::RetType CmdShowRouteCounters::queryClient(
    const HostInfo& hostInfo) {
  std::map<std::string, HwSwitchCounter> routeCounters;
  auto client =
      utils::createClient<apache::thrift::Client<FbossCtrl>>(hostInfo);
  client->sync_getRouteCounters(routeCounters);
  return createModel(routeCounters);
}

CmdShowRouteCounters::RetType CmdShowRouteCounters::createModel(
    const std::map<std::string, HwSwitchCounter>& routeCounters) const {
  RetType model;
  for (const auto& [counterID, counter] : routeCounters) {
    cli::RouteCounterEntry entry;
    entry.counterID() = counterID;
    if (counter.bytes().has_value()) {
      entry.bytes() = *counter.bytes();
    }
    if (counter.packets().has_value()) {
      entry.packets() = *counter.packets();
    }
    model.routeCounters()->push_back(std::move(entry));
  }
  return model;
}

void CmdShowRouteCounters::printOutput(
    const RetType& model,
    std::ostream& out) {
  Table table;
  table.setHeader({"Counter ID", "Bytes", "Packets"});
  for (const auto& counter : *model.routeCounters()) {
    table.addRow({
        *counter.counterID(),
        counter.bytes().has_value() ? std::to_string(*counter.bytes()) : "-",
        counter.packets().has_value() ? std::to_string(*counter.packets())
                                      : "-",
    });
  }
  out << table << std::endl;
}

std::string_view CmdShowRouteCountersTraits::description() {
  return "Displays route counter values aggregated across all hardware switches, including byte and packet counts.";
}

CmdShowRouteCounters::RetType CmdShowRouteCounters::sampleModel() {
  RetType model;
  cli::RouteCounterEntry entry;
  entry.counterID() = "route.counter.example";
  entry.bytes() = 4096;
  entry.packets() = 32;
  model.routeCounters()->push_back(std::move(entry));
  return model;
}

template void
CmdHandler<CmdShowRouteCounters, CmdShowRouteCountersTraits>::run();
template const ValidFilterMapType
CmdHandler<CmdShowRouteCounters, CmdShowRouteCountersTraits>::getValidFilters();

} // namespace facebook::fboss
