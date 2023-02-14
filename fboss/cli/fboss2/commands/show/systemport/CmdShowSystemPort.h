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

#include <thrift/lib/cpp/transport/TTransportException.h>
#include "fboss/cli/fboss2/CmdGlobalOptions.h"
#include "fboss/cli/fboss2/commands/show/systemport/gen-cpp2/model_types.h"
#include "fboss/cli/fboss2/commands/show/systemport/gen-cpp2/model_visitation.h"
#include "fboss/cli/fboss2/utils/CmdUtils.h"
#include "fboss/cli/fboss2/utils/Table.h"

#include <unistd.h>
#include <algorithm>

namespace facebook::fboss {

using utils::Table;

struct CmdShowSystemPortTraits : public BaseCommandTraits {
  using ObjectArgType = utils::SystemPortList;
  using RetType = cli::ShowSystemPortModel;
  static constexpr bool ALLOW_FILTERING = true;
  static constexpr bool ALLOW_AGGREGATION = true;
};

class CmdShowSystemPort
    : public CmdHandler<CmdShowSystemPort, CmdShowSystemPortTraits> {
 public:
  using ObjectArgType = CmdShowSystemPortTraits::ObjectArgType;
  using RetType = CmdShowSystemPortTraits::RetType;

  RetType queryClient(
      const HostInfo& hostInfo,
      const ObjectArgType& queriedSystemPorts) {
    std::map<int64_t, facebook::fboss::SystemPortThrift> systemportEntries;
    auto client =
        utils::createClient<apache::thrift::Client<FbossCtrl>>(hostInfo);
    client->sync_getSystemPorts(systemportEntries);

    return createModel(systemportEntries, queriedSystemPorts.data());
  }

  std::unordered_map<std::string, std::vector<std::string>>
  getAcceptedFilterValues() {
    return {{"adminState", {"Enabled", "Disabled"}}};
  }

  void printOutput(const RetType& model, std::ostream& out = std::cout) {
    Table table;
    table.setHeader(
        {"ID",
         "Name",
         "AdminState",
         "Speed",
         "NumVoqs",
         "QosPolicy",
         "CoreIndex",
         "CorePortIndex"});

    for (auto const& systemportInfo : model.get_sysPortEntries()) {
      table.addRow(
          {folly::to<std::string>(systemportInfo.get_id()),
           systemportInfo.get_name(),
           systemportInfo.get_adminState(),
           systemportInfo.get_speed(),
           folly::to<std::string>(systemportInfo.get_numVoqs()),
           systemportInfo.get_qosPolicy(),
           folly::to<std::string>(systemportInfo.get_coreIndex()),
           folly::to<std::string>(systemportInfo.get_corePortIndex())});
    }

    out << table << std::endl;
  }

  std::string getAdminStateStr(bool adminState) {
    return (adminState ? "Enabled" : "Disabled");
  }

  std::string getSpeedGbps(int64_t speedMbps) {
    return std::to_string(speedMbps / 1000) + "G";
  }

  RetType createModel(
      std::map<int64_t, facebook::fboss::SystemPortThrift> systemPortEntries,
      const ObjectArgType& queriedSystemPorts) {
    RetType model;
    std::unordered_set<std::string> queriedSet(
        queriedSystemPorts.begin(), queriedSystemPorts.end());

    for (const auto& entry : systemPortEntries) {
      auto systemPortInfo = entry.second;
      const auto& systemPortName = systemPortInfo.get_portName();

      if (queriedSystemPorts.size() == 0 || queriedSet.count(systemPortName)) {
        cli::SystemPortEntry systemPortDetails;
        systemPortDetails.id() = systemPortInfo.get_portId();
        systemPortDetails.name() = systemPortInfo.get_portName();
        systemPortDetails.adminState() =
            getAdminStateStr(systemPortInfo.get_enabled());
        systemPortDetails.speed() =
            getSpeedGbps(systemPortInfo.get_speedMbps());
        systemPortDetails.numVoqs() = systemPortInfo.get_numVoqs();
        systemPortDetails.qosPolicy() =
            (systemPortInfo.get_qosPolicy() ? *systemPortInfo.get_qosPolicy()
                                            : " -- ");
        systemPortDetails.coreIndex() = systemPortInfo.get_coreIndex();
        systemPortDetails.corePortIndex() = systemPortInfo.get_corePortIndex();
        model.sysPortEntries()->push_back(systemPortDetails);
      }
    }

    std::sort(
        model.sysPortEntries()->begin(),
        model.sysPortEntries()->end(),
        [&](const cli::SystemPortEntry& a, const cli::SystemPortEntry& b) {
          return utils::compareSystemPortName(a.get_name(), b.get_name());
        });

    return model;
  }
};

} // namespace facebook::fboss
