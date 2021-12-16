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

    auto printFlowStatsMap = [&out](const auto& modelIntfMKAStats) {
      const auto& inStatsList = *modelIntfMKAStats.ingressFlowStats_ref();
      const auto& outStatsList = *modelIntfMKAStats.egressFlowStats_ref();

      auto printFlowStats = [&out](const auto& flowStat, bool ingress) {
        const auto& sci = *flowStat.sci_ref();
        const auto& stats = *flowStat.flowStats_ref();

        out << "SCI INFO: " << std::endl;
        out << "MAC: " << sci.get_macAddress() << std::endl;
        out << "PORT: " << sci.get_port() << std::endl;
        Table table;
        table.setHeader({"Flow Counter Name", "Value"});
        table.addRow(
            {"UnicastUncontrolledPackets",
             std::to_string(*stats.ucastUncontrolledPkts_ref())});
        table.addRow(
            {"UnicastControlledPackets",
             std::to_string(*stats.ucastControlledPkts_ref())});
        table.addRow(
            {"MulticastUncontrolledPackets",
             std::to_string(*stats.mcastUncontrolledPkts_ref())});
        table.addRow(
            {"MulticastControlledPackets",
             std::to_string(*stats.mcastControlledPkts_ref())});
        table.addRow(
            {"BroadcastUncontrolledPackets",
             std::to_string(*stats.bcastUncontrolledPkts_ref())});
        table.addRow(
            {"BroadcastControlledPackets",
             std::to_string(*stats.bcastControlledPkts_ref())});
        table.addRow(
            {"ControlPackets", std::to_string(*stats.controlPkts_ref())});
        table.addRow(
            {"OtherErroredPackets", std::to_string(*stats.otherErrPkts_ref())});
        table.addRow(
            {"OctetsUncontrolled",
             std::to_string(*stats.octetsUncontrolled_ref())});
        table.addRow(
            {"OctetsControlled",
             std::to_string(*stats.octetsControlled_ref())});

        if (ingress) {
          table.addRow(
              {"UntaggedPackets", std::to_string(*stats.untaggedPkts_ref())});
          table.addRow(
              {"BadTagPackets", std::to_string(*stats.inBadTagPkts_ref())});
          table.addRow(
              {"NoSciPackets", std::to_string(*stats.noSciPkts_ref())});
          table.addRow(
              {"UnknownSciPackets",
               std::to_string(*stats.unknownSciPkts_ref())});
        } else {
          table.addRow(
              {"CommonOctets", std::to_string(*stats.outCommonOctets_ref())});
          table.addRow(
              {"TooLongPackets", std::to_string(*stats.outTooLongPkts_ref())});
        }
        out << table << std::endl;
      };

      if (!inStatsList.empty()) {
        out << "Ingress Flow Stats" << std::endl;
        out << std::string(20, '-') << std::endl;
        for (const auto& inStat : inStatsList) {
          printFlowStats(inStat, true);
        }
      }

      if (!outStatsList.empty()) {
        out << "Egress Flow Stats" << std::endl;
        out << std::string(20, '-') << std::endl;
        for (const auto& outStat : outStatsList) {
          printFlowStats(outStat, false);
        }
      }
    };

    auto printSaStatsMap = [&out](const auto& modelIntfMKAStats) {
      const auto& inStatsList =
          *modelIntfMKAStats.rxSecureAssociationStats_ref();
      const auto& outStatsList =
          *modelIntfMKAStats.txSecureAssociationStats_ref();

      auto printSaStats = [&out](const auto& saStat, bool ingress) {
        const auto& stats = saStat.get_saStats();
        const auto& sa = saStat.get_saId();
        const auto& sci = sa.get_sci();

        out << "SA INFO: " << std::endl;
        out << "MAC: " << sci.get_macAddress() << std::endl;
        out << "PORT: " << sci.get_port() << std::endl;
        out << "Association#: " << sa.get_assocNum() << std::endl;

        Table table;
        table.setHeader({"SA Counter Name", "Value"});
        table.addRow(
            {"OctetsEncrypted", std::to_string(*stats.octetsEncrypted_ref())});
        table.addRow(
            {"OctetsProtected", std::to_string(*stats.octetsProtected_ref())});
        if (ingress) {
          table.addRow(
              {"UncheckedPacktes",
               std::to_string(*stats.inUncheckedPkts_ref())});
          table.addRow(
              {"DelayedPacktes", std::to_string(*stats.inDelayedPkts_ref())});
          table.addRow(
              {"LatePacktes", std::to_string(*stats.inLatePkts_ref())});
          table.addRow(
              {"LatePacktes", std::to_string(*stats.inLatePkts_ref())});
          table.addRow(
              {"InvalidPacktes", std::to_string(*stats.inInvalidPkts_ref())});
          table.addRow(
              {"NotValidPacktes", std::to_string(*stats.inNotValidPkts_ref())});
          table.addRow(
              {"NoSaPacktes", std::to_string(*stats.inNoSaPkts_ref())});
          table.addRow(
              {"UnusedSaPacktes", std::to_string(*stats.inUnusedSaPkts_ref())});
          table.addRow({"OkPacktes", std::to_string(*stats.inOkPkts_ref())});
        } else {
          table.addRow(
              {"EncryptedPacktes",
               std::to_string(*stats.outEncryptedPkts_ref())});
          table.addRow(
              {"ProtectedPacktes",
               std::to_string(*stats.outProtectedPkts_ref())});
        }
        out << table << std::endl;
      };

      if (!inStatsList.empty()) {
        out << "Ingress SA Stats" << std::endl;
        out << std::string(20, '-') << std::endl;
        for (const auto& inStat : inStatsList) {
          printSaStats(inStat, true);
        }
      }

      if (!outStatsList.empty()) {
        out << "Egress SA Stats" << std::endl;
        out << std::string(20, '-') << std::endl;
        for (const auto& outStat : outStatsList) {
          printSaStats(outStat, false);
        }
      }
    };

    auto printAclStats = [&out](const auto& modelIntfMKAStats) {
      const auto& inStats = *modelIntfMKAStats.ingressAclStats_ref();

      Table table;
      table.setHeader({"Counter Name", "IN", "OUT"});
      table.addRow(
          {"DefaultAclPackets",
           std::to_string(*inStats.defaultAclStats_ref()),
           "0"});
      out << table << std::endl;
    };

    for (auto& m : model.get_intfMKAStats()) {
      out << "Port: " << m.first << std::endl;
      out << std::string(20, '=') << std::endl;

      // Print port stats
      printPortStats(m.second);
      // Print flow stats
      printFlowStatsMap(m.second);
      // Print SA stats
      printSaStatsMap(m.second);
      // Print ACL stats
      printAclStats(m.second);
    }
  }
};

} // namespace facebook::fboss
