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

#include <fboss/agent/if/gen-cpp2/ctrl_constants.h>
#include <fboss/agent/if/gen-cpp2/ctrl_types.h>
#include <cstdint>
#include "fboss/cli/fboss2/CmdHandler.h"
#include "fboss/cli/fboss2/commands/show/ndp/gen-cpp2/model_types.h"

namespace facebook::fboss {

struct CmdShowNdpTraits : public BaseCommandTraits {
  static constexpr utils::ObjectArgTypeId ObjectArgTypeId =
      utils::ObjectArgTypeId::OBJECT_ARG_TYPE_ID_IPV6_LIST;
  using ObjectArgType = std::vector<std::string>;
  using RetType = cli::ShowNdpModel;
  static constexpr bool ALLOW_FILTERING = true;
  static constexpr bool ALLOW_AGGREGATION = true;
};

class CmdShowNdp : public CmdHandler<CmdShowNdp, CmdShowNdpTraits> {
 public:
  using ObjectArgType = CmdShowNdpTraits::ObjectArgType;
  using RetType = CmdShowNdpTraits::RetType;

  RetType queryClient(
      const HostInfo& hostInfo,
      const ObjectArgType& queriedNdpEntries) {
    std::vector<facebook::fboss::NdpEntryThrift> entries;
    std::map<int32_t, facebook::fboss::PortInfoThrift> portEntries;
    auto client =
        utils::createClient<facebook::fboss::FbossCtrlAsyncClient>(hostInfo);

    client->sync_getNdpTable(entries);
    client->sync_getAllPortInfo(portEntries);
    return createModel(entries, queriedNdpEntries, portEntries);
  }

  void printOutput(const RetType& model, std::ostream& out = std::cout) {
    constexpr auto fmtString = "{:<45}{:<19}{:<12}{:<19}{:<14}{:<9}{:<12}\n";

    out << fmt::format(
        fmtString,
        "IP Address",
        "MAC Address",
        "Interface",
        "VLAN",
        "State",
        "TTL",
        "CLASSID");

    for (const auto& entry : model.get_ndpEntries()) {
      auto vlan = entry.get_vlanName();
      if (entry.get_vlanID() != ctrl_constants::NO_VLAN()) {
        vlan += folly::to<std::string>(" (", entry.get_vlanID(), ")");
      }

      out << fmt::format(
          fmtString,
          entry.get_ip(),
          entry.get_mac(),
          entry.get_port(),
          vlan,
          entry.get_state(),
          entry.get_ttl(),
          entry.get_classID());
    }
    out << std::endl;
  }

  RetType createModel(
      std::vector<facebook::fboss::NdpEntryThrift> ndpEntries,
      const ObjectArgType& queriedNdpEntries,
      std::map<int32_t, facebook::fboss::PortInfoThrift>& portEntries) {
    RetType model;
    std::unordered_set<std::string> queriedSet(
        queriedNdpEntries.begin(), queriedNdpEntries.end());

    for (const auto& entry : ndpEntries) {
      auto ip = folly::IPAddress::fromBinary(
          folly::ByteRange(folly::StringPiece(entry.get_ip().get_addr())));

      if (queriedNdpEntries.size() == 0 || queriedSet.count(ip.str())) {
        cli::NdpEntry ndpDetails;
        ndpDetails.ip() = ip.str();
        ndpDetails.mac() = entry.get_mac();
        if (*entry.isLocal()) {
          ndpDetails.port() = portEntries[entry.get_port()].get_name();
        } else {
          ndpDetails.port() = folly::to<std::string>(entry.get_port());
        }
        ndpDetails.vlanName() = entry.get_vlanName();
        ndpDetails.vlanID() = entry.get_vlanID();
        ndpDetails.state() = entry.get_state();
        ndpDetails.ttl() = entry.get_ttl();
        ndpDetails.classID() = entry.get_classID();

        model.ndpEntries()->push_back(ndpDetails);
      }
    }
    return model;
  }
};

} // namespace facebook::fboss
