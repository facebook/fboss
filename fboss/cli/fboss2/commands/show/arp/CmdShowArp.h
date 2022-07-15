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

#include <fboss/agent/if/gen-cpp2/ctrl_types.h>
#include <cstdint>
#include "fboss/cli/fboss2/CmdHandler.h"
#include "fboss/cli/fboss2/commands/show/arp/gen-cpp2/model_types.h"

namespace facebook::fboss {

struct CmdShowArpTraits : public BaseCommandTraits {
  static constexpr utils::ObjectArgTypeId ObjectArgTypeId =
      utils::ObjectArgTypeId::OBJECT_ARG_TYPE_ID_NONE;
  using ObjectArgType = std::monostate;
  using RetType = cli::ShowArpModel;
  static constexpr bool ALLOW_FILTERING = true;
};

class CmdShowArp : public CmdHandler<CmdShowArp, CmdShowArpTraits> {
 public:
  RetType queryClient(const HostInfo& hostInfo) {
    std::vector<facebook::fboss::ArpEntryThrift> entries;
    std::map<int32_t, facebook::fboss::PortInfoThrift> portEntries;
    auto client =
        utils::createClient<facebook::fboss::FbossCtrlAsyncClient>(hostInfo);

    client->sync_getArpTable(entries);
    client->sync_getAllPortInfo(portEntries);
    return createModel(entries, portEntries);
  }

  std::unordered_map<std::string, std::vector<std::string>>
  getAcceptedFilterValues() {
    return {{"state", {"REACHABLE", "UNREACHABLE"}}};
  }

  void printOutput(const RetType& model, std::ostream& out = std::cout) {
    constexpr auto fmtString = "{:<22}{:<19}{:<12}{:<19}{:<14}{:<9}{:<12}\n";

    out << fmt::format(
        fmtString,
        "IP Address",
        "MAC Address",
        "Interface",
        "VLAN",
        "State",
        "TTL",
        "CLASSID");

    for (const auto& entry : model.get_arpEntries()) {
      out << fmt::format(
          fmtString,
          entry.get_ip(),
          entry.get_mac(),
          entry.get_ifName(),
          entry.get_vlan(),
          entry.get_state(),
          entry.get_ttl(),
          entry.get_classID());
    }
    out << std::endl;
  }

  RetType createModel(
      std::vector<facebook::fboss::ArpEntryThrift> arpEntries,
      std::map<int32_t, facebook::fboss::PortInfoThrift> portEntries) {
    RetType model;

    for (const auto& entry : arpEntries) {
      cli::ArpEntry arpDetails;

      auto ip = folly::IPAddress::fromBinary(
          folly::ByteRange(folly::StringPiece(entry.get_ip().get_addr())));
      arpDetails.ip() = ip.str();
      arpDetails.mac() = entry.get_mac();
      arpDetails.port() = entry.get_port();
      arpDetails.vlan() = folly::to<std::string>(
          entry.get_vlanName(), " (", entry.get_vlanID(), ")");
      arpDetails.state() = entry.get_state();
      arpDetails.ttl() = entry.get_ttl();
      arpDetails.classID() = entry.get_classID();
      arpDetails.ifName() = portEntries[entry.get_port()].get_name();

      model.arpEntries()->push_back(arpDetails);
    }
    return model;
  }
};

} // namespace facebook::fboss
