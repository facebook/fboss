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
#include "fboss/cli/fboss2/commands/show/interface/counters/CmdShowInterfaceCounters.h"
#include "fboss/cli/fboss2/commands/show/interface/counters/mka/gen-cpp2/model_types.h"
#include "fboss/cli/fboss2/utils/Table.h"

#include <boost/algorithm/string.hpp>
#include <unordered_set>

namespace facebook::fboss {

using utils::Table;

struct CmdShowInterfaceCountersMKATraits : public BaseCommandTraits {
  using ParentCmd = CmdShowInterfaceCounters;
  static constexpr utils::ObjectArgTypeId ObjectArgTypeId =
      utils::ObjectArgTypeId::OBJECT_ARG_TYPE_ID_NONE;
  using ObjectArgType = std::monostate;
  using RetType = cli::InterfaceCountersMKAModel;
};

class CmdShowInterfaceCountersMKA : public CmdHandler<
                                        CmdShowInterfaceCountersMKA,
                                        CmdShowInterfaceCountersMKATraits> {
 public:
  using ObjectArgType = CmdShowInterfaceCountersMKATraits::ObjectArgType;
  using RetType = CmdShowInterfaceCountersMKATraits::RetType;

  RetType queryClient(
      const HostInfo& hostInfo,
      const std::vector<std::string>& queriedIfs) {
    auto client =
        utils::createClient<facebook::fboss::QsfpServiceAsyncClient>(hostInfo);

    std::map<std::string, facebook::fboss::MacsecStats> intfsMKAStatsMap;

    if (queriedIfs.empty()) {
      client->sync_getAllMacsecPortStats(intfsMKAStatsMap, true);
    } else {
      client->sync_getMacsecPortStats(intfsMKAStatsMap, queriedIfs, true);
    }

    return createModel(intfsMKAStatsMap);
  }

  RetType createModel(const std::map<std::string, facebook::fboss::MacsecStats>&
                          intfsMKAStatsMap) {
    RetType model;
    model.intfMKAStats_ref() = intfsMKAStatsMap;

    return model;
  }

  void printOutput(const RetType& model, std::ostream& out = std::cout) {
    Table table;

    auto printPortStats = [&out](const auto& modelIntfMKAStats) {
      const auto& inStats = *modelIntfMKAStats.ingressPortStats_ref();
      const auto& outStats = *modelIntfMKAStats.egressPortStats_ref();

      Table table;
      table.setHeader({"Counter Name", "IN", "OUT"});
      table.addRow(
          {"PreMacsecDropPackets",
           std::to_string(*inStats.preMacsecDropPkts_ref()),
           std::to_string(*outStats.preMacsecDropPkts_ref())});
      table.addRow(
          {"ControlPackets",
           std::to_string(*inStats.controlPkts_ref()),
           std::to_string(*outStats.controlPkts_ref())});
      table.addRow(
          {"DataPackets",
           std::to_string(*inStats.dataPkts_ref()),
           std::to_string(*outStats.dataPkts_ref())});
      table.addRow(
          {"(En/De)cryptedOctets",
           std::to_string(*inStats.octetsEncrypted_ref()),
           std::to_string(*outStats.octetsEncrypted_ref())});
      table.addRow(
          {"MacsecTagPkts",
           std::to_string(*inStats.noMacsecTagPkts_ref()),
           std::to_string(*outStats.noMacsecTagPkts_ref())});
      table.addRow(
          {"BadOrNoMacsecTagDroppedPkts",
           std::to_string(*outStats.inBadOrNoMacsecTagDroppedPkts_ref()),
           "--"});
      table.addRow(
          {"NoSciDroppedPkts",
           std::to_string(*inStats.inNoSciDroppedPkts_ref()),
           "--"});
      table.addRow(
          {"UnknownSciPkts",
           std::to_string(*inStats.inUnknownSciPkts_ref()),
           "--"});
      table.addRow(
          {"OverrunDroppedPkts",
           std::to_string(*inStats.inOverrunDroppedPkts_ref()),
           "--"});
      table.addRow(
          {"DelayedPkts", std::to_string(*inStats.inDelayedPkts_ref()), "--"});
      table.addRow(
          {"LateDroppedPkts",
           std::to_string(*inStats.inLateDroppedPkts_ref()),
           "--"});
      table.addRow(
          {"NotValidDroppedPkts",
           std::to_string(*inStats.inNotValidDroppedPkts_ref()),
           "--"});
      table.addRow(
          {"InvalidPkts", std::to_string(*inStats.inInvalidPkts_ref()), "--"});
      table.addRow(
          {"NoSaDroppedPkts",
           std::to_string(*inStats.inNoSaDroppedPkts_ref()),
           "--"});
      table.addRow(
          {"UnusedSaPkts",
           std::to_string(*inStats.inUnusedSaPkts_ref()),
           "--"});
      table.addRow(
          {"TooLongDroppedPkts",
           "--",
           std::to_string(*outStats.outTooLongDroppedPkts_ref())});

      out << table << std::endl;
    };

    for (auto& m : model.get_intfMKAStats()) {
      out << "Port: " << m.first << std::endl;
      out << std::string(20, '=') << std::endl;

      printPortStats(m.second);
    }
  }
};

} // namespace facebook::fboss
