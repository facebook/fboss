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

struct CmdShowLldpTraits {
  static constexpr utils::ObjectArgTypeId ObjectArgTypeId =
      utils::ObjectArgTypeId::OBJECT_ARG_TYPE_ID_NONE;
  using ObjectArgType = std::monostate;
  using RetType = std::vector<facebook::fboss::LinkNeighborThrift>;
};

class CmdShowLldp : public CmdHandler<CmdShowLldp, CmdShowLldpTraits> {
 public:
  using RetType = CmdShowLldpTraits::RetType;

  RetType queryClient(const folly::IPAddress& hostIp) {
    RetType retVal;
    auto client = utils::createClient<facebook::fboss::FbossCtrlAsyncClient>(
        hostIp.str());

    client->sync_getLldpNeighbors(retVal);
    return retVal;
  }

  void printOutput(const RetType& lldpNeighbors) {
    std::string fmtString = "{:<17}{:<38}{:<12}\n";

    std::cout << fmt::format(
        fmtString,
        "Local Port",
        "Name",
        "Port");
    std::cout << std::string(80, '-') << std::endl;

    for (auto const& nbr : lldpNeighbors) {
      std::string systemName = nbr.get_systemName()
          ? *nbr.get_systemName()
          : nbr.get_printableChassisId();

      std::cout << fmt::format(
          fmtString,
          nbr.get_localPort(),
          systemName,
          nbr.get_printablePortId());
    }
    std::cout << std::endl;
  }
};

} // namespace facebook::fboss
