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
#include "fboss/cli/fboss2/commands/show/ndp/gen-cpp2/model_types.h"

namespace facebook::fboss {

struct CmdShowNdpTraits : public BaseCommandTraits {
  static constexpr utils::ObjectArgTypeId ObjectArgTypeId =
      utils::ObjectArgTypeId::OBJECT_ARG_TYPE_ID_IPV6_LIST;
  using ObjectArgType = std::vector<std::string>;
  using RetType = cli::ShowNdpModel;
};

class CmdShowNdp : public CmdHandler<CmdShowNdp, CmdShowNdpTraits> {
 public:
  using ObjectArgType = CmdShowNdpTraits::ObjectArgType;
  using RetType = CmdShowNdpTraits::RetType;

  RetType queryClient(
      const HostInfo& hostInfo,
      const ObjectArgType& queriedNdpEntries) {
    std::vector<facebook::fboss::NdpEntryThrift> entries;
    auto client =
        utils::createClient<facebook::fboss::FbossCtrlAsyncClient>(hostInfo);

    client->sync_getNdpTable(entries);
    return createModel(entries, queriedNdpEntries);
  }

  void printOutput(const RetType& model, std::ostream& out = std::cout) {
    std::string fmtString = "{:<45}{:<19}{:<12}{:<19}{:<14}{:<9}{:<12}\n";

    out << fmt::format(
        fmtString,
        "IP Address",
        "MAC Address",
        "Port",
        "VLAN",
        "State",
        "TTL",
        "CLASSID");

    for (const auto& entry : model.get_ndpEntries()) {
      auto vlan = folly::to<std::string>(
          entry.get_vlanName(), " (", entry.get_vlanID(), ")");

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
      const ObjectArgType& queriedNdpEntries) {
    RetType model;
    std::unordered_set<std::string> queriedSet(
        queriedNdpEntries.begin(), queriedNdpEntries.end());

    for (const auto& entry : ndpEntries) {
      auto ip = folly::IPAddress::fromBinary(
          folly::ByteRange(folly::StringPiece(entry.get_ip().get_addr())));

      if (queriedNdpEntries.size() == 0 || queriedSet.count(ip.str())) {
        cli::NdpEntry ndpDetails;
        ndpDetails.ip_ref() = ip.str();
        ndpDetails.mac_ref() = entry.get_mac();
        ndpDetails.port_ref() = entry.get_port();
        ndpDetails.vlanName_ref() = entry.get_vlanName();
        ndpDetails.vlanID_ref() = entry.get_vlanID();
        ndpDetails.state_ref() = entry.get_state();
        ndpDetails.ttl_ref() = entry.get_ttl();
        ndpDetails.classID_ref() = entry.get_classID();

        model.ndpEntries_ref()->push_back(ndpDetails);
      }
    }
    return model;
  }
};

} // namespace facebook::fboss
