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
    model.intfMKAStats() = intfsMKAStatsMap;

    return model;
  }

  void printOutput(const RetType& model, std::ostream& out = std::cout) {
    Table table;

    auto printPortStats = [&out](const auto& modelIntfMKAStats) {
      const auto& inStats = *modelIntfMKAStats.ingressPortStats();
      const auto& outStats = *modelIntfMKAStats.egressPortStats();

      Table table;
      table.setHeader({"Counter Name", "IN", "OUT"});
      table.addRow(
          {"PreMacsecDropPackets",
           std::to_string(*inStats.preMacsecDropPkts()),
           std::to_string(*outStats.preMacsecDropPkts())});
      table.addRow(
          {"ControlPackets",
           std::to_string(*inStats.controlPkts()),
           std::to_string(*outStats.controlPkts())});
      table.addRow(
          {"DataPackets",
           std::to_string(*inStats.dataPkts()),
           std::to_string(*outStats.dataPkts())});
      table.addRow(
          {"(En/De)cryptedOctets",
           std::to_string(*inStats.octetsEncrypted()),
           std::to_string(*outStats.octetsEncrypted())});
      table.addRow(
          {"MacsecTagPkts",
           std::to_string(*inStats.noMacsecTagPkts()),
           std::to_string(*outStats.noMacsecTagPkts())});
      table.addRow(
          {"BadOrNoMacsecTagDroppedPkts",
           std::to_string(*outStats.inBadOrNoMacsecTagDroppedPkts()),
           "--"});
      table.addRow(
          {"NoSciDroppedPkts",
           std::to_string(*inStats.inNoSciDroppedPkts()),
           "--"});
      table.addRow(
          {"UnknownSciPkts",
           std::to_string(*inStats.inUnknownSciPkts()),
           "--"});
      table.addRow(
          {"OverrunDroppedPkts",
           std::to_string(*inStats.inOverrunDroppedPkts()),
           "--"});
      table.addRow(
          {"DelayedPkts", std::to_string(*inStats.inDelayedPkts()), "--"});
      table.addRow(
          {"LateDroppedPkts",
           std::to_string(*inStats.inLateDroppedPkts()),
           "--"});
      table.addRow(
          {"NotValidDroppedPkts",
           std::to_string(*inStats.inNotValidDroppedPkts()),
           "--"});
      table.addRow(
          {"InvalidPkts", std::to_string(*inStats.inInvalidPkts()), "--"});
      table.addRow(
          {"NoSaDroppedPkts",
           std::to_string(*inStats.inNoSaDroppedPkts()),
           "--"});
      table.addRow(
          {"UnusedSaPkts", std::to_string(*inStats.inUnusedSaPkts()), "--"});
      table.addRow(
          {"TooLongDroppedPkts",
           "--",
           std::to_string(*outStats.outTooLongDroppedPkts())});
      table.addRow(
          {"CurrentXPN",
           std::to_string(*inStats.inCurrentXpn()),
           std::to_string(*outStats.outCurrentXpn())});

      out << table << std::endl;
    };

    auto printFlowStatsMap = [&out](const auto& modelIntfMKAStats) {
      const auto& inStatsList = *modelIntfMKAStats.ingressFlowStats();
      const auto& outStatsList = *modelIntfMKAStats.egressFlowStats();

      auto printFlowStats = [&out](const auto& flowStat, bool ingress) {
        const auto& sci = *flowStat.sci();
        const auto& stats = *flowStat.flowStats();

        out << "SCI INFO: " << std::endl;
        out << "MAC: " << sci.get_macAddress() << std::endl;
        out << "PORT: " << sci.get_port() << std::endl;
        Table table;
        table.setHeader({"Flow Counter Name", "Value"});
        table.addRow(
            {"UnicastUncontrolledPackets",
             std::to_string(*stats.ucastUncontrolledPkts())});
        table.addRow(
            {"UnicastControlledPackets",
             std::to_string(*stats.ucastControlledPkts())});
        table.addRow(
            {"MulticastUncontrolledPackets",
             std::to_string(*stats.mcastUncontrolledPkts())});
        table.addRow(
            {"MulticastControlledPackets",
             std::to_string(*stats.mcastControlledPkts())});
        table.addRow(
            {"BroadcastUncontrolledPackets",
             std::to_string(*stats.bcastUncontrolledPkts())});
        table.addRow(
            {"BroadcastControlledPackets",
             std::to_string(*stats.bcastControlledPkts())});
        table.addRow({"ControlPackets", std::to_string(*stats.controlPkts())});
        table.addRow(
            {"OtherErroredPackets", std::to_string(*stats.otherErrPkts())});
        table.addRow(
            {"OctetsUncontrolled",
             std::to_string(*stats.octetsUncontrolled())});
        table.addRow(
            {"OctetsControlled", std::to_string(*stats.octetsControlled())});

        if (ingress) {
          table.addRow(
              {"UntaggedPackets", std::to_string(*stats.untaggedPkts())});
          table.addRow(
              {"BadTagPackets", std::to_string(*stats.inBadTagPkts())});
          table.addRow({"NoSciPackets", std::to_string(*stats.noSciPkts())});
          table.addRow(
              {"UnknownSciPackets", std::to_string(*stats.unknownSciPkts())});
        } else {
          table.addRow(
              {"CommonOctets", std::to_string(*stats.outCommonOctets())});
          table.addRow(
              {"TooLongPackets", std::to_string(*stats.outTooLongPkts())});
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
      const auto& inStatsList = *modelIntfMKAStats.rxSecureAssociationStats();
      const auto& outStatsList = *modelIntfMKAStats.txSecureAssociationStats();

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
            {"OctetsEncrypted", std::to_string(*stats.octetsEncrypted())});
        table.addRow(
            {"OctetsProtected", std::to_string(*stats.octetsProtected())});
        if (ingress) {
          table.addRow(
              {"UncheckedPackets", std::to_string(*stats.inUncheckedPkts())});
          table.addRow(
              {"DelayedPackets", std::to_string(*stats.inDelayedPkts())});
          table.addRow({"LatePackets", std::to_string(*stats.inLatePkts())});
          table.addRow(
              {"InvalidPackets", std::to_string(*stats.inInvalidPkts())});
          table.addRow(
              {"NotValidPackets", std::to_string(*stats.inNotValidPkts())});
          table.addRow({"NoSaPackets", std::to_string(*stats.inNoSaPkts())});
          table.addRow(
              {"UnusedSaPackets", std::to_string(*stats.inUnusedSaPkts())});
          table.addRow({"InCurrentXpn", std::to_string(*stats.inCurrentXpn())});
          table.addRow({"OkPackets", std::to_string(*stats.inOkPkts())});
        } else {
          table.addRow(
              {"EncryptedPackets", std::to_string(*stats.outEncryptedPkts())});
          table.addRow(
              {"ProtectedPackets", std::to_string(*stats.outProtectedPkts())});
          table.addRow(
              {"OutCurentXpn", std::to_string(*stats.outCurrentXpn())});
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
      const auto& inStats = *modelIntfMKAStats.ingressAclStats();
      const auto& outStats = *modelIntfMKAStats.egressAclStats();

      Table table;
      table.setHeader({"Counter Name", "IN", "OUT"});
      table.addRow(
          {"DefaultAclPackets",
           std::to_string(*inStats.defaultAclStats()),
           std::to_string(*outStats.defaultAclStats())});
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
