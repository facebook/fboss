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
#include "fboss/cli/fboss2/utils/CmdUtils.h"
#include "fboss/cli/fboss2/utils/Table.h"

#include <boost/algorithm/string.hpp>

namespace facebook::fboss {

using facebook::network::NetworkUtil;
using utils::Table;

struct CmdShowHostTraits : public BaseCommandTraits {
  static constexpr utils::ObjectArgTypeId ObjectArgTypeId =
      utils::ObjectArgTypeId::OBJECT_ARG_TYPE_ID_NONE;
  // TODO: Add argument taking PortList
  using ObjectArgType = std::monostate;
  using RetType = cli::ShowHostModel;
};

class CmdShowHost : public CmdHandler<CmdShowHost, CmdShowHostTraits> {
 public:
  using RetType = CmdShowHostTraits::RetType;

  RetType queryClient(const HostInfo& hostInfo) {
    std::vector<int32_t> ports;
    std::vector<NdpEntryThrift> ndpEntries;
    std::map<int32_t, PortInfoThrift> portInfoEntries;
    std::map<int32_t, PortStatus> portStatusEntries;
    auto agentClient =
        utils::createClient<apache::thrift::Client<FbossCtrl>>(hostInfo);
    agentClient->sync_getNdpTable(ndpEntries);
    agentClient->sync_getAllPortInfo(portInfoEntries);
    agentClient->sync_getPortStatus(portStatusEntries, ports);
    return createModel(ndpEntries, portInfoEntries, portStatusEntries);
  }

  RetType createModel(
      const std::vector<NdpEntryThrift>& ndpEntries,
      const std::map<int32_t, PortInfoThrift>& portInfoEntries,
      const std::map<int32_t, PortStatus>& portStatusEntries) {
    RetType model;
    for (const auto& ndpEntry : ndpEntries) {
      cli::ShowHostModelEntry hostDetails;
      std::string ndpEntryAddr = utils::getAddrStr(ndpEntry.get_ip());
      // Skip link-local addresses
      if (boost::algorithm::starts_with(ndpEntryAddr, "fe80:")) {
        continue;
      }
      int32_t ndpEntryPort = ndpEntry.get_port();
      hostDetails.portID() = ndpEntryPort;
      int32_t ndpEntryClassID = ndpEntry.get_classID();
      if (ndpEntryClassID != 0) {
        hostDetails.queueID() = folly::to<std::string>(ndpEntryClassID - 10);
      } else {
        hostDetails.queueID() = "Olympic";
      }
      std::string ndpEntryHostName =
          removeFbDomains(NetworkUtil::getHostByAddr(ndpEntryAddr));
      if (ndpEntryHostName.empty()) {
        hostDetails.hostName() = ndpEntryAddr;
      } else {
        hostDetails.hostName() = ndpEntryHostName;
      }
      if (auto ndpEntryPortInfoEntry = portInfoEntries.find(ndpEntryPort);
          ndpEntryPortInfoEntry != portInfoEntries.end()) {
        const auto& ndpEntryPortInfo = ndpEntryPortInfoEntry->second;
        hostDetails.portName() = ndpEntryPortInfo.get_name();
        hostDetails.speed() = getSpeedGbps(ndpEntryPortInfo.get_speedMbps());
        hostDetails.fecMode() = ndpEntryPortInfo.get_fecMode();
        hostDetails.inErrors() =
            ndpEntryPortInfo.get_input().get_errors().get_errors();
        hostDetails.inDiscards() =
            ndpEntryPortInfo.get_input().get_errors().get_discards();
        hostDetails.outErrors() =
            ndpEntryPortInfo.get_output().get_errors().get_errors();
        hostDetails.outDiscards() =
            ndpEntryPortInfo.get_output().get_errors().get_discards();
      } else {
        continue;
      }
      if (auto ndpEntryPortStatusEntry = portStatusEntries.find(ndpEntryPort);
          ndpEntryPortStatusEntry != portStatusEntries.end()) {
        const auto& ndpEntryPortStatus = ndpEntryPortStatusEntry->second;
        hostDetails.adminState() =
            (ndpEntryPortStatus.get_enabled()) ? "Enabled" : "Disabled";
        hostDetails.linkState() = (ndpEntryPortStatus.get_up()) ? "Up" : "Down";
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
          return a.get_portID() < b.get_portID();
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
    for (const auto& hostEntry : model.get_hostEntries()) {
      table.addRow(
          {hostEntry.get_portName(),
           folly::to<std::string>(hostEntry.get_portID()),
           hostEntry.get_queueID(),
           hostEntry.get_hostName(),
           hostEntry.get_adminState(),
           hostEntry.get_linkState(),
           hostEntry.get_speed(),
           hostEntry.get_fecMode(),
           folly::to<std::string>(hostEntry.get_inErrors()),
           folly::to<std::string>(hostEntry.get_inDiscards()),
           folly::to<std::string>(hostEntry.get_outErrors()),
           folly::to<std::string>(hostEntry.get_outDiscards())});
    }
    out << table << std::endl;
  }

  const std::string removeFbDomains(const std::string& hostname) {
    std::string host_copy = hostname;
    const RE2 fb_domains(".facebook.com$|.tfbnw.net$");
    RE2::Replace(&host_copy, fb_domains, "");
    return host_copy;
  }

  const std::string getSpeedGbps(int64_t speedMbps) {
    return std::to_string(speedMbps / 1000) + "G";
  }
};

} // namespace facebook::fboss
