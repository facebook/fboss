/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "CmdShowFabric.h"

#include <re2/re2.h>
#include <algorithm>
#include <string>
#include "fboss/agent/if/gen-cpp2/ctrl_types.h"
#include "fboss/cli/fboss2/utils/CmdClientUtils.h"
#include "fboss/cli/fboss2/utils/CmdUtils.h"
#include "fboss/cli/fboss2/utils/HostInfo.h"
#include "fboss/cli/fboss2/utils/Table.h"
#include "folly/Conv.h"

namespace facebook::fboss {

using utils::Table;
using RetType = CmdShowFabricTraits::RetType;

RetType CmdShowFabric::queryClient(const HostInfo& hostInfo) {
  return createModel(utils::getFabricEndpoints(hostInfo));
}

inline void CmdShowFabric::udpateNametoIdString(
    std::string& name,
    int64_t value) {
  auto idToString =
      value == -1 ? "(-)" : folly::to<std::string>("(", value, ")");
  name += idToString;
}

void CmdShowFabric::printOutput(const RetType& model, std::ostream& out) {
  Table table;
  table.setHeader({
      "Local Port",
      "Peer Switch (Id)",
      "Exp Peer Switch (Id)",
      "Peer Port (Id)",
      "Exp Peer Port (Id)",
      "Match",
  });

  for (auto const& entry : model.fabricEntries().value()) {
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
            get_NeighborStyle(remoteSwitchNameId, expectedRemoteSwitchNameId)),
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

Table::Style CmdShowFabric::get_NeighborStyle(
    const std::string& actualId,
    const std::string& expectedId) {
  if (actualId == expectedId) {
    return Table::Style::GOOD;
  }
  return Table::Style::ERROR;
}

RetType CmdShowFabric::createModel(
    std::map<std::string, FabricEndpoint> fabricEntries) {
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
        endpoint.expectedPortId().has_value() ? *endpoint.expectedPortId() : -1;
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
        return utils::comparePortName(
            a.localPort().value(), b.localPort().value());
      });

  return model;
}

} // namespace facebook::fboss
