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
#include "fboss/agent/if/gen-cpp2/hw_ctrl_types.h"
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
    if (utils::isFbossFeatureEnabled(hostInfo.getName(), "multi_switch")) {
      auto hwAgentQueryFn =
          [&entries](
              apache::thrift::Client<facebook::fboss::FbossHwCtrl>& client) {
            std::map<std::string, FabricEndpoint> hwagentEntries;
            client.sync_getHwFabricConnectivity(hwagentEntries);
            entries.merge(hwagentEntries);
          };
      utils::runOnAllHwAgents(hostInfo, hwAgentQueryFn);
    } else {
      auto client =
          utils::createClient<apache::thrift::Client<FbossCtrl>>(hostInfo);
      client->sync_getFabricConnectivity(entries);
    }
    return createModel(entries);
  }

  inline void udpateNametoIdString(std::string& name, int64_t value) {
    auto idToString =
        value == -1 ? "(-)" : folly::to<std::string>("(", value, ")");
    name += idToString;
  }

  void printOutput(const RetType& model, std::ostream& out = std::cout) {
    Table table;
    table.setHeader({
        "Local Port",
        "Peer Switch (Id)",
        "Exp Peer Switch (Id)",
        "Peer Port (Id)",
        "Exp Peer Port (Id)",
        "Match",
    });

    for (auto const& entry : model.get_fabricEntries()) {
      std::string remoteSwitchNameId =
          utils::removeFbDomains(*entry.remoteSwitchName());
      udpateNametoIdString(remoteSwitchNameId, *entry.remoteSwitchId());

      std::string expectedRemoteSwitchNameId =
          utils::removeFbDomains(*entry.expectedRemoteSwitchName());
      udpateNametoIdString(
          expectedRemoteSwitchNameId, *entry.expectedRemoteSwitchId());

      std::string remotePortNameId = *entry.remotePortName();
      udpateNametoIdString(remotePortNameId, *entry.remotePortId());

      std::string expectedRemotePortNameId = *entry.expectedRemotePortName();
      udpateNametoIdString(
          expectedRemotePortNameId, *entry.expectedRemotePortId());

      bool match =
          (remoteSwitchNameId == expectedRemoteSwitchNameId &&
           remotePortNameId == expectedRemotePortNameId);

      table.addRow({
          *entry.localPort(),
          Table::StyledCell(
              remoteSwitchNameId,
              get_NeighborStyle(
                  remoteSwitchNameId, expectedRemoteSwitchNameId)),
          expectedRemoteSwitchNameId,
          Table::StyledCell(
              remotePortNameId,
              get_NeighborStyle(remotePortNameId, expectedRemotePortNameId)),
          expectedRemotePortNameId,
          match ? "Yes" : "No",
      });
    }

    out << table << std::endl;
  }

  Table::Style get_NeighborStyle(
      const std::string& actualId,
      const std::string& expectedId) {
    if (actualId == expectedId) {
      return Table::Style::GOOD;
    }
    return Table::Style::ERROR;
  }

  RetType createModel(std::map<std::string, FabricEndpoint> fabricEntries) {
    RetType model;
    const std::string kUnavail;
    const std::string kUnattached = "NOT_ATTACHED";
    for (const auto& entry : fabricEntries) {
      cli::FabricEntry fabricDetails;
      fabricDetails.localPort() = entry.first;
      auto endpoint = entry.second;
      // if endpoint is not attached and no expected neighbor configured, skip
      // the endpoint
      if (!*endpoint.isAttached() &&
          (!endpoint.expectedSwitchName().has_value())) {
        continue;
      }
      // hw endpoint
      if (!*endpoint.isAttached()) {
        fabricDetails.remotePortName() = kUnattached;
        fabricDetails.remoteSwitchName() = kUnattached;
      } else {
        fabricDetails.remotePortName() =
            endpoint.portName() ? *endpoint.portName() : kUnavail;
        fabricDetails.remoteSwitchName() =
            endpoint.switchName() ? *endpoint.switchName() : kUnavail;
      }
      fabricDetails.remoteSwitchId() = *endpoint.switchId();
      fabricDetails.remotePortId() = *endpoint.portId();

      // expected endpoint per cfg
      fabricDetails.expectedRemoteSwitchId() =
          endpoint.expectedSwitchId().has_value() ? *endpoint.expectedSwitchId()
                                                  : -1;
      fabricDetails.expectedRemotePortId() =
          endpoint.expectedPortId().has_value() ? *endpoint.expectedPortId()
                                                : -1;
      fabricDetails.expectedRemotePortName() =
          endpoint.expectedPortName().has_value() ? *endpoint.expectedPortName()
                                                  : kUnavail;
      fabricDetails.expectedRemoteSwitchName() =
          endpoint.expectedSwitchName().has_value()
          ? *endpoint.expectedSwitchName()
          : kUnavail;
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
