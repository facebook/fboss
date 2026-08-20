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
#include "fboss/cli/fboss2/CmdHandler.cpp"

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

std::string_view CmdShowFabricTraits::description() {
  return "Displays the switch's fabric links: for each local fabric port, the discovered and expected peer switch (with switch ID), peer port, and whether they match. DSF-only — applies to disaggregated scheduled fabric (DSF) switches and returns data only in a DSF topology.";
}

RetType CmdShowFabric::sampleModel() {
  RetType model;

  cli::FabricEntry entry1;
  entry1.localPort() = "fab1/1/1";
  entry1.remoteSwitchName() = "fdsw001";
  entry1.remoteSwitchId() = 2722;
  entry1.expectedRemoteSwitchName() = "fdsw001";
  entry1.expectedRemoteSwitchId() = 2722;
  entry1.remotePortName() = "fab1/103/5";
  entry1.remotePortId() = 63;
  entry1.expectedRemotePortName() = "fab1/103/5";
  entry1.expectedRemotePortId() = 63;
  model.fabricEntries()->push_back(entry1);

  cli::FabricEntry entry2;
  entry2.localPort() = "fab1/1/2";
  entry2.remoteSwitchName() = "fdsw001";
  entry2.remoteSwitchId() = 2720;
  entry2.expectedRemoteSwitchName() = "fdsw001";
  entry2.expectedRemoteSwitchId() = 2720;
  entry2.remotePortName() = "fab1/103/6";
  entry2.remotePortId() = 195;
  entry2.expectedRemotePortName() = "fab1/103/6";
  entry2.expectedRemotePortId() = 195;
  model.fabricEntries()->push_back(entry2);

  cli::FabricEntry entry3;
  entry3.localPort() = "fab1/1/5";
  entry3.remoteSwitchName() = "fdsw002";
  entry3.remoteSwitchId() = 2726;
  entry3.expectedRemoteSwitchName() = "fdsw002";
  entry3.expectedRemoteSwitchId() = 2726;
  entry3.remotePortName() = "fab1/103/5";
  entry3.remotePortId() = 63;
  entry3.expectedRemotePortName() = "fab1/103/5";
  entry3.expectedRemotePortId() = 63;
  model.fabricEntries()->push_back(entry3);

  return model;
}

// Explicit template instantiation
template void CmdHandler<CmdShowFabric, CmdShowFabricTraits>::run();
template const ValidFilterMapType
CmdHandler<CmdShowFabric, CmdShowFabricTraits>::getValidFilters();

} // namespace facebook::fboss
