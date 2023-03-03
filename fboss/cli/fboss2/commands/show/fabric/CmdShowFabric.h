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

#include <re2/re2.h>
#include <string>
#include <unordered_set>
#include "fboss/agent/if/gen-cpp2/ctrl_types.h"
#include "fboss/cli/fboss2/CmdHandler.h"
#include "fboss/cli/fboss2/commands/show/fabric/gen-cpp2/model_types.h"
#include "fboss/cli/fboss2/utils/CmdUtils.h"
#include "fboss/cli/fboss2/utils/Table.h"
#include "folly/container/Access.h"

namespace facebook::fboss {

using utils::Table;

struct CmdShowFabricTraits : public BaseCommandTraits {
  static constexpr utils::ObjectArgTypeId ObjectArgTypeId =
      utils::ObjectArgTypeId::OBJECT_ARG_TYPE_ID_NONE;
  using ObjectArgType = std::monostate;
  using RetType = cli::ShowFabricModel;
  static constexpr bool ALLOW_FILTERING = true;
  static constexpr bool ALLOW_AGGREGATION = true;
};

class CmdShowFabric : public CmdHandler<CmdShowFabric, CmdShowFabricTraits> {
 public:
  using RetType = CmdShowFabricTraits::RetType;

  RetType queryClient(const HostInfo& hostInfo) {
    std::map<std::string, FabricEndpoint> entries;
    auto client =
        utils::createClient<apache::thrift::Client<FbossCtrl>>(hostInfo);

    client->sync_getFabricReachability(entries);

    return createModel(entries);
  }

  void printOutput(const RetType& model, std::ostream& out = std::cout) {
    Table table;
    table.setHeader({
        "Local Port",
        "Peer switch",
        "Peer switchId",
        "Peer Port",
        "Peer PortId",
    });

    for (auto const& entry : model.get_fabricEntries()) {
      std::string remoteSwitchId = *entry.remoteSwitchId() == -1
          ? "--"
          : folly::to<std::string>(*entry.remoteSwitchId());
      table.addRow({
          *entry.localPort(),
          utils::removeFbDomains(*entry.remoteSwitchName()),
          remoteSwitchId,
          *entry.remotePortName(),
          folly::to<std::string>(*entry.remotePortId()),
      });
    }

    out << table << std::endl;
  }

  RetType createModel(std::map<std::string, FabricEndpoint> fabricEntries) {
    RetType model;
    const std::string kUnavail;
    for (const auto& entry : fabricEntries) {
      cli::FabricEntry fabricDetails;
      fabricDetails.localPort() = entry.first;
      auto endpoint = entry.second;
      if (!*endpoint.isAttached()) {
        continue;
      }
      fabricDetails.remoteSwitchId() = *endpoint.switchId();
      fabricDetails.remotePortId() = *endpoint.portId();
      fabricDetails.remotePortName() =
          endpoint.portName() ? *endpoint.portName() : kUnavail;
      fabricDetails.remoteSwitchName() =
          endpoint.switchName() ? *endpoint.switchName() : kUnavail;
      model.fabricEntries()->push_back(fabricDetails);
    }

    std::sort(
        model.fabricEntries()->begin(),
        model.fabricEntries()->end(),
        [](cli::FabricEntry& a, cli::FabricEntry b) {
          return utils::comparePortName(a.get_localPort(), b.get_localPort());
        });

    return model;
  }
};

} // namespace facebook::fboss
