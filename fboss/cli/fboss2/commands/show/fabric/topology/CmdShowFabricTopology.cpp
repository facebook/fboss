/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/cli/fboss2/commands/show/fabric/topology/CmdShowFabricTopology.h"
#include "fboss/cli/fboss2/CmdHandler.cpp"

#include "fboss/cli/fboss2/utils/CmdClientUtils.h"
#include "fboss/cli/fboss2/utils/Table.h"
#include "folly/String.h"

namespace facebook::fboss {

using utils::Table;

CmdShowFabricTopology::RetType CmdShowFabricTopology::queryClient(
    const HostInfo& hostInfo) {
  std::map<int64_t, std::map<int64_t, std::vector<RemoteEndpoint>>> entries;
  if (utils::isMultiSwitchEnabled(hostInfo)) {
    auto hwAgentQueryFn =
        [&entries](
            apache::thrift::Client<facebook::fboss::FbossHwCtrl>& client) {
          std::map<int64_t, std::map<int64_t, std::vector<RemoteEndpoint>>>
              hwAgentEntries;
          client.sync_getVirtualDeviceToConnectionGroups(hwAgentEntries);
          entries.merge(hwAgentEntries);
        };
    utils::runOnAllHwAgents(hostInfo, hwAgentQueryFn);
  } else {
    throw std::runtime_error(
        "Command only supported for multi switch enabled devices");
  }
  return createModel(entries);
}

CmdShowFabricTopology::RetType CmdShowFabricTopology::createModel(
    const std::map<int64_t, std::map<int64_t, std::vector<RemoteEndpoint>>>&
        entries) {
  RetType model;
  for (const auto& [vid, connectionGroups] : entries) {
    bool isSymmetric = connectionGroups.size() <= 1;
    for (const auto& [numConnections, connectionGroup] : connectionGroups) {
      for (const auto& remoteEndpoint : connectionGroup) {
        cli::FabricVirtualDeviceTopology entry;
        entry.virtualDeviceId() = vid;
        entry.numConnections() = numConnections;
        entry.remoteSwitchId() = *remoteEndpoint.switchId();
        entry.remoteSwitchName() = *remoteEndpoint.switchName();
        entry.connectingPorts() = *remoteEndpoint.connectingPorts();
        entry.isSymmetric() = isSymmetric;
        model.virtualDeviceTopology()->push_back(entry);
      }
    }
  }
  return model;
}

void CmdShowFabricTopology::printOutput(
    const RetType& model,
    std::ostream& out) {
  Table table;
  table.setHeader({
      "Virtual device Id",
      "Num connections",
      "Remote Switch (Id)",
      "Remote Switch",
      "Connecting Ports",
  });

  for (auto const& entry : model.virtualDeviceTopology().value()) {
    auto connectingPortsStr = folly::join(",", *entry.connectingPorts());
    table.addRow(
        {folly::to<std::string>(*entry.virtualDeviceId()),
         folly::to<std::string>(*entry.numConnections()),
         folly::to<std::string>(*entry.remoteSwitchId()),
         *entry.remoteSwitchName(),
         connectingPortsStr},
        getSymmetryStyle(*entry.isSymmetric()));
  }
  out << table << std::endl;
}

Table::Style CmdShowFabricTopology::getSymmetryStyle(bool isSymmetric) const {
  return isSymmetric ? Table::Style::GOOD : Table::Style::ERROR;
}

std::string_view CmdShowFabricTopologyTraits::description() {
  return "Displays the fabric topology from a fabric switch's view: per virtual device, the number of connections and the remote interface switch reached, with the connecting fabric ports. DSF-only, and specifically FDSW-only — it works on fabric switches (FDSW), not on rack switches (RDSW).";
}

CmdShowFabricTopology::RetType CmdShowFabricTopology::sampleModel() {
  RetType model;

  cli::FabricVirtualDeviceTopology entry1;
  entry1.virtualDeviceId() = 0;
  entry1.numConnections() = 1;
  entry1.remoteSwitchId() = 0;
  entry1.remoteSwitchName() = "rdsw001";
  entry1.connectingPorts() = {"fab1/1/4"};
  entry1.isSymmetric() = true;
  model.virtualDeviceTopology()->push_back(entry1);

  cli::FabricVirtualDeviceTopology entry2;
  entry2.virtualDeviceId() = 0;
  entry2.numConnections() = 1;
  entry2.remoteSwitchId() = 4;
  entry2.remoteSwitchName() = "rdsw002";
  entry2.connectingPorts() = {"fab1/1/8"};
  entry2.isSymmetric() = true;
  model.virtualDeviceTopology()->push_back(entry2);

  cli::FabricVirtualDeviceTopology entry3;
  entry3.virtualDeviceId() = 0;
  entry3.numConnections() = 1;
  entry3.remoteSwitchId() = 8;
  entry3.remoteSwitchName() = "rdsw003";
  entry3.connectingPorts() = {"fab1/9/4"};
  entry3.isSymmetric() = true;
  model.virtualDeviceTopology()->push_back(entry3);

  return model;
}

// Explicit template instantiation
template void
CmdHandler<CmdShowFabricTopology, CmdShowFabricTopologyTraits>::run();
template const ValidFilterMapType CmdHandler<
    CmdShowFabricTopology,
    CmdShowFabricTopologyTraits>::getValidFilters();
} // namespace facebook::fboss
