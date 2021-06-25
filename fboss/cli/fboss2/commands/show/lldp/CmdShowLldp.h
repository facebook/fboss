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
#include "fboss/cli/fboss2/commands/show/lldp/gen-cpp2/model_types.h"

namespace facebook::fboss {

struct CmdShowLldpTraits {
  static constexpr utils::ObjectArgTypeId ObjectArgTypeId =
      utils::ObjectArgTypeId::OBJECT_ARG_TYPE_ID_NONE;
  using ObjectArgType = std::monostate;
  using RetType = cli::ShowLldpModel;
};

class CmdShowLldp : public CmdHandler<CmdShowLldp, CmdShowLldpTraits> {
 public:
  using RetType = CmdShowLldpTraits::RetType;

  RetType queryClient(const folly::IPAddress& hostIp) {
    std::vector<facebook::fboss::LinkNeighborThrift> entries;
    auto client = utils::createClient<facebook::fboss::FbossCtrlAsyncClient>(
        hostIp.str());

    client->sync_getLldpNeighbors(entries);
    return createModel(entries);
  }

  void printOutput(const RetType& model, std::ostream& out = std::cout) {
    std::string fmtString = "{:<17}{:<38}{:<12}\n";

    out << fmt::format(
        fmtString,
        "Local Port",
        "Name",
        "Port");
    out << std::string(80, '-') << std::endl;

    for (auto const& entry : model.get_lldpEntries()) {
      out << fmt::format(
          fmtString,
          entry.get_localPort(),
          entry.get_systemName(),
          entry.get_remotePort());
    }
    out << std::endl;
  }

  private:
   RetType createModel(std::vector<facebook::fboss::LinkNeighborThrift> lldpEntries) {
     RetType model;

     for (const auto& entry : lldpEntries) {
       cli::LldpEntry lldpDetails;

       lldpDetails.localPort_ref() = std::to_string(entry.get_localPort());
       lldpDetails.systemName_ref() = entry.get_systemName()
          ? *entry.get_systemName()
          : entry.get_printableChassisId();
       lldpDetails.remotePort_ref() = entry.get_printablePortId();

       model.lldpEntries_ref()->push_back(lldpDetails);
     }
     return model;
   }
};

} // namespace facebook::fboss
