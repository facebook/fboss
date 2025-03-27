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

#include "common/network/NetworkUtil.h"
#include "fboss/cli/fboss2/CmdGlobalOptions.h"
#include "fboss/cli/fboss2/CmdHandler.h"
#include "fboss/cli/fboss2/commands/show/host/gen-cpp2/model_types.h"
#include "fboss/cli/fboss2/utils/CmdClientUtils.h"
#include "fboss/cli/fboss2/utils/CmdUtils.h"
#include "fboss/cli/fboss2/utils/Table.h"

#include <boost/algorithm/string.hpp>

namespace facebook::fboss {

using facebook::network::NetworkUtil;
using utils::Table;

struct CmdShowHostTraits : public BaseCommandTraits {
  static constexpr utils::ObjectArgTypeId ObjectArgTypeId =
      utils::ObjectArgTypeId::OBJECT_ARG_TYPE_ID_PORT_LIST;
  using ObjectArgType = utils::PortList;
  using RetType = cli::ShowHostModel;
};

class CmdShowHost : public CmdHandler<CmdShowHost, CmdShowHostTraits> {
 public:
  using ObjectArgType = CmdShowHostTraits::ObjectArgType;
  using RetType = CmdShowHostTraits::RetType;

  RetType queryClient(
      const HostInfo& hostInfo,
      const ObjectArgType& queriedPorts) {
    std::vector<int32_t> ports;
    std::vector<NdpEntryThrift> ndpEntries;
    std::map<int32_t, PortInfoThrift> portInfoEntries;
    std::map<int32_t, PortStatus> portStatusEntries;
    auto agentClient =
        utils::createClient<apache::thrift::Client<FbossCtrl>>(hostInfo);
    agentClient->sync_getNdpTable(ndpEntries);
    agentClient->sync_getAllPortInfo(portInfoEntries);
    agentClient->sync_getPortStatus(portStatusEntries, ports);
    return createModel(
        ndpEntries, portInfoEntries, portStatusEntries, queriedPorts);
  }

  RetType createModel(
      const std::vector<NdpEntryThrift>& ndpEntries,
      const std::map<int32_t, PortInfoThrift>& portInfoEntries,
      const std::map<int32_t, PortStatus>& portStatusEntries,
      const ObjectArgType& queriedPorts) {
    RetType model;
    for (const auto& ndpEntry : ndpEntries) {
      cli::ShowHostModelEntry hostDetails;
      std::string ndpEntryAddr = utils::getAddrStr(ndpEntry.ip().value());
      // Skip link-local addresses
      if (boost::algorithm::starts_with(ndpEntryAddr, "fe80:")) {
        continue;
      }
      std::unordered_set<std::string> queriedSet(
          queriedPorts.begin(), queriedPorts.end());
      int32_t ndpEntryPort = folly::copy(ndpEntry.port().value());
      hostDetails.portID() = ndpEntryPort;
      int32_t ndpEntryClassID = folly::copy(ndpEntry.classID().value());
      if (ndpEntryClassID != 0) {
        hostDetails.queueID() = folly::to<std::string>(ndpEntryClassID - 10);
      } else {
        hostDetails.queueID() = "Olympic";
      }
      std::string ndpEntryHostName =
          utils::removeFbDomains(NetworkUtil::getHostByAddr(ndpEntryAddr));
      if (ndpEntryHostName.empty()) {
        hostDetails.hostName() = ndpEntryAddr;
      } else {
        hostDetails.hostName() = ndpEntryHostName;
      }
      auto ndpEntryPortInfoEntry = portInfoEntries.find(ndpEntryPort);
      if (ndpEntryPortInfoEntry != portInfoEntries.end()) {
        const auto& ndpEntryPortInfo = ndpEntryPortInfoEntry->second;
        auto ndpEntryPortName = ndpEntryPortInfo.name().value();
        if (queriedSet.size() > 0 && queriedSet.count(ndpEntryPortName) == 0) {
          continue;
        }
        hostDetails.portName() = ndpEntryPortName;
        hostDetails.speed() = utils::getSpeedGbps(
            folly::copy(ndpEntryPortInfo.speedMbps().value()));
        hostDetails.fecMode() = ndpEntryPortInfo.fecMode().value();
        hostDetails.inErrors() = folly::copy(
            ndpEntryPortInfo.input().value().errors().value().errors().value());
        hostDetails.inDiscards() = folly::copy(ndpEntryPortInfo.input()
                                                   .value()
                                                   .errors()
                                                   .value()
                                                   .discards()
                                                   .value());
        hostDetails.outErrors() = folly::copy(ndpEntryPortInfo.output()
                                                  .value()
                                                  .errors()
                                                  .value()
                                                  .errors()
                                                  .value());
        hostDetails.outDiscards() = folly::copy(ndpEntryPortInfo.output()
                                                    .value()
                                                    .errors()
                                                    .value()
                                                    .discards()
                                                    .value());
      } else {
        continue;
      }
      auto ndpEntryPortStatusEntry = portStatusEntries.find(ndpEntryPort);
      if (ndpEntryPortStatusEntry != portStatusEntries.end()) {
        const auto& ndpEntryPortStatus = ndpEntryPortStatusEntry->second;
        hostDetails.adminState() =
            (folly::copy(ndpEntryPortStatus.enabled().value())) ? "Enabled"
                                                                : "Disabled";
        hostDetails.linkState() =
            (folly::copy(ndpEntryPortStatus.up().value())) ? "Up" : "Down";
      } else {
        continue;
      }
      model.hostEntries()->push_back(hostDetails);
    }
    std::sort(
        model.hostEntries()->begin(),
        model.hostEntries()->end(),
        [&](const cli::ShowHostModelEntry& a,
            const cli::ShowHostModelEntry& b) {
          return folly::copy(a.portID().value()) <
              folly::copy(b.portID().value());
        });
    return model;
  }

  void printOutput(const RetType& model, std::ostream& out = std::cout) {
    Table table;
    table.setHeader(
        {"Port",
         "ID",
         "Queue ID",
         "Hostname",
         "Admin State",
         "Link State",
         "Speed",
         "FEC",
         "InErr",
         "InDiscard",
         "OutErr",
         "OutDiscard"});
    for (const auto& hostEntry : model.hostEntries().value()) {
      table.addRow(
          {hostEntry.portName().value(),
           folly::to<std::string>(folly::copy(hostEntry.portID().value())),
           hostEntry.queueID().value(),
           hostEntry.hostName().value(),
           hostEntry.adminState().value(),
           hostEntry.linkState().value(),
           hostEntry.speed().value(),
           hostEntry.fecMode().value(),
           folly::to<std::string>(folly::copy(hostEntry.inErrors().value())),
           folly::to<std::string>(folly::copy(hostEntry.inDiscards().value())),
           folly::to<std::string>(folly::copy(hostEntry.outErrors().value())),
           folly::to<std::string>(
               folly::copy(hostEntry.outDiscards().value()))});
    }
    out << table << std::endl;
  }
};

} // namespace facebook::fboss
