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
#include "fboss/cli/fboss2/commands/show/interface/CmdShowInterface.h"
#include "fboss/cli/fboss2/commands/show/interface/phymap/gen-cpp2/model_types.h"
#include "fboss/cli/fboss2/utils/Table.h"
#include "fboss/mka_service/if/gen-cpp2/mka_structs_types.h"

#include <boost/algorithm/string.hpp>
#include <unordered_set>

namespace facebook::fboss {

using utils::Table;

struct CmdShowInterfacePhymapTraits : public BaseCommandTraits {
  using ParentCmd = CmdShowInterface;
  static constexpr utils::ObjectArgTypeId ObjectArgTypeId =
      utils::ObjectArgTypeId::OBJECT_ARG_TYPE_ID_NONE;
  using ObjectArgType = std::monostate;
  using RetType = cli::InterfacePhymapModel;
};

class CmdShowInterfacePhymap
    : public CmdHandler<CmdShowInterfacePhymap, CmdShowInterfacePhymapTraits> {
 public:
  using ObjectArgType = CmdShowInterfacePhymapTraits::ObjectArgType;
  using RetType = CmdShowInterfacePhymapTraits::RetType;

  RetType queryClient(
      const HostInfo& hostInfo,
      const std::vector<std::string>& queriedIfs) {
    auto client =
        utils::createClient<facebook::fboss::QsfpServiceAsyncClient>(hostInfo);

    facebook::fboss::mka::MacsecPortPhyMap portsPhyMap;

    client->sync_macsecGetPhyPortInfo(portsPhyMap, queriedIfs);

    return createModel(portsPhyMap);
  }

  RetType createModel(
      const facebook::fboss::mka::MacsecPortPhyMap& portsPhyMap) {
    RetType model;
    model.portsPhyMap_ref() = portsPhyMap;

    return model;
  }

  void printOutput(const RetType& model, std::ostream& out = std::cout) {
    auto printPortMap = [&out](const auto& portsPhyMap) {
      Table table;
      table.setHeader(
          {"SW Port", "SlotId", "MdioId", "PhyId", "SaiSwitchId", "SliceId"});

      for (auto& phyMap : portsPhyMap) {
        table.addRow(
            {std::to_string(phyMap.first),
             std::to_string(phyMap.second.slotId_ref().value()),
             std::to_string(phyMap.second.mdioId_ref().value()),
             std::to_string(phyMap.second.phyId_ref().value()),
             std::to_string(phyMap.second.saiSwitchId_ref().value()),
             std::to_string(phyMap.second.saiSwitchId_ref().value() - 1)});
      }
      out << table << std::endl;
    };

    auto& portsPhyMap = model.get_portsPhyMap();
    if (!portsPhyMap.macsecPortPhyMap_ref().is_set()) {
      out << "No Phy port map for this platform" << std::endl;
      return;
    }

    out << "Printing Port PHY map for system:" << std::endl;
    printPortMap(portsPhyMap.macsecPortPhyMap_ref().value());
  }
};

} // namespace facebook::fboss
