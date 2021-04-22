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

namespace facebook::fboss {

struct CmdShowArpTraits {
  static constexpr utils::ObjectArgTypeId ObjectArgTypeId =
      utils::ObjectArgTypeId::OBJECT_ARG_TYPE_ID_NONE;
  using ObjectArgType = std::monostate;
  using RetType = std::vector<facebook::fboss::ArpEntryThrift>;
};

class CmdShowArp : public CmdHandler<CmdShowArp, CmdShowArpTraits> {
 public:
  using RetType = CmdShowArpTraits::RetType;

  RetType queryClient(const folly::IPAddress& hostIp) {
    RetType retVal;
    auto client = utils::createClient<facebook::fboss::FbossCtrlAsyncClient>(
        hostIp.str());

    client->sync_getArpTable(retVal);
    return retVal;
  }

  void printOutput(const RetType& arpEntries) {
    std::string fmtString = "{:<22}{:<19}{:<12}{:<19}{:<14}{:<9}{:<12}\n";

    std::cout << fmt::format(
        fmtString,
        "IP Address",
        "MAC Address",
        "Port",
        "VLAN",
        "State",
        "TTL",
        "CLASSID");

    for (auto const& arpEntry : arpEntries) {
      auto ip = folly::IPAddress::fromBinary(
          folly::ByteRange(folly::StringPiece(arpEntry.get_ip().get_addr())));
      auto vlan = folly::to<std::string>(
          arpEntry.get_vlanName(), " (", arpEntry.get_vlanID(), ")");

      std::cout << fmt::format(
          fmtString,
          ip.str(),
          arpEntry.get_mac(),
          arpEntry.get_port(),
          vlan,
          arpEntry.get_state(),
          arpEntry.get_ttl(),
          arpEntry.get_classID());
    }
    std::cout << std::endl;
  }
};

} // namespace facebook::fboss
