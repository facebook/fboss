// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include <fboss/agent/if/gen-cpp2/ctrl_types.h>
#include <folly/String.h>
#include <folly/gen/Base.h>
#include <cstdint>
#include "fboss/agent/if/gen-cpp2/common_types.h"
#include "fboss/cli/fboss2/CmdHandler.h"
#include "fboss/cli/fboss2/commands/show/interface/CmdShowInterface.h"
#include "fboss/cli/fboss2/commands/show/interface/phy/gen-cpp2/model_types.h"
#include "fboss/cli/fboss2/utils/CmdUtils.h"
#include "fboss/cli/fboss2/utils/Table.h"
#include "fboss/lib/phy/gen-cpp2/phy_types.h"
#include "thrift/lib/cpp/util/EnumUtils.h"

namespace facebook::fboss {

struct CmdShowInterfacePhyTraits : public BaseCommandTraits {
  using ParentCmd = CmdShowInterface;
  static constexpr utils::ObjectArgTypeId ObjectArgTypeId =
      utils::ObjectArgTypeId::OBJECT_ARG_TYPE_PHY_CHIP_TYPE;
  using ObjectArgType = utils::PhyChipType;
  using RetType = cli::ShowInterfacePhyModel;
};

class CmdShowInterfacePhy
    : public CmdHandler<CmdShowInterfacePhy, CmdShowInterfacePhyTraits> {
 public:
  RetType queryClient(
      const HostInfo& hostInfo,
      const utils::PortList& queriedIfs,
      const utils::PhyChipType& phyChipType) {
    return createModel(hostInfo, queriedIfs, phyChipType);
  }

  void printOutput(const RetType& model, std::ostream& out = std::cout) {
    for (const auto& info : *model.phyInfo_ref()) {
      auto ifName = info.first;
      auto phyInfo = info.second;
      if (phyInfo.find(phy::DataPlanePhyChipType::IPHY) != phyInfo.end()) {
        printPhyInfo(out, phyInfo[phy::DataPlanePhyChipType::IPHY], ifName);
      }
      if (phyInfo.find(phy::DataPlanePhyChipType::XPHY) != phyInfo.end()) {
        printPhyInfo(out, phyInfo[phy::DataPlanePhyChipType::XPHY], ifName);
      }
      out << std::endl;
    }
  }

  void printPhyInfo(
      std::ostream& out,
      phy::PhyInfo& phyInfo,
      const std::string& ifName) {
    auto phyType =
        apache::thrift::util::enumNameSafe(*phyInfo.phyChip()->type());
    std::string prefix = phyType + "-";
    Table table;
    table.setHeader({"Interface", ifName});
    table.addRow({"PhyChipType", phyType});
    if (auto linkState = phyInfo.linkState()) {
      if (*linkState) {
        table.addRow(
            {"Link State", Table::StyledCell("UP", Table::Style::GOOD)});
      } else {
        table.addRow(
            {"Link State", Table::StyledCell("DOWN", Table::Style::ERROR)});
      }
    }
    if (auto linkFlapCount = phyInfo.linkFlapCount()) {
      table.addRow({"Link Flap Count", std::to_string(*linkFlapCount)});
    }
    table.addRow(
        {"Speed", apache::thrift::util::enumNameSafe(*phyInfo.speed())});
    table.addRow(
        {phyType + " Data Collected",
         utils::getPrettyElapsedTime(*phyInfo.timeCollected()) + " ago"});
    out << table;
    if (auto systemSide = phyInfo.system()) {
      printSideInfo(out, *systemSide, prefix + "System ");
    }
    printSideInfo(out, *phyInfo.line(), prefix + "Line ");
  }

  void printSideInfo(
      std::ostream& out,
      phy::PhySideInfo& sideInfo,
      const std::string& prefix) {
    if (auto rs = sideInfo.rs()) {
      Table rsTable;
      rsTable.setHeader({prefix + "Reconciliation Sublayer", ""});
      rsTable.addRow(
          {prefix + "Local Fault Live",
           std::to_string(*(*rs).faultStatus()->localFault())});
      rsTable.addRow(
          {prefix + "Remote Fault Live",
           std::to_string(*(*rs).faultStatus()->remoteFault())});
      out << rsTable;
    }
    if (auto pcs = sideInfo.pcs()) {
      Table pcsTable;
      Table rsFecTable;
      bool hasPcsData{false}, hasRsFecData{false};
      if (auto pcsRxStatusLive = (*pcs).pcsRxStatusLive()) {
        pcsTable.addRow(
            {prefix + "PCS RX Link Status Live",
             makeColorCellForLiveFlag(std::to_string(*pcsRxStatusLive))});
        hasPcsData = true;
      }
      if (auto rsFec = (*pcs).rsFec()) {
        rsFecTable.setHeader({prefix + "RS FEC", ""});
        rsFecTable.addRow(
            {prefix + "Corrected codewords",
             std::to_string(*(rsFec->correctedCodewords()))});
        rsFecTable.addRow(
            {prefix + "Uncorrected codewords",
             std::to_string(*(rsFec->uncorrectedCodewords()))});
        std::ostringstream outStringStream;
        outStringStream << *rsFec->preFECBer();
        rsFecTable.addRow({prefix + "Pre-FEC BER", outStringStream.str()});
        hasRsFecData = true;
      }
      if (hasPcsData) {
        pcsTable.setHeader({prefix + "PCS", ""});
        out << pcsTable;
      }
      if (hasRsFecData) {
        out << rsFecTable;
      }
    }

    if (!sideInfo.pmd()->lanes()->empty()) {
      Table pmdTable;
      pmdTable.setHeader(
          {prefix + "PMD",
           "Lane",
           "RX Signal Detect Live",
           "RX CDR Lock Live",
           "Eye Heights",
           "Eye Widths"});
      for (const auto& laneInfo : *sideInfo.pmd()->lanes()) {
        auto lane = laneInfo.second;
        std::string sigDetLive = "N/A";
        std::string cdrLockLive = "N/A";
        std::vector<float> eyeHeights = {};
        std::vector<float> eyeWidths = {};
        if (auto rxSigDetLive = lane.signalDetectLive()) {
          sigDetLive = std::to_string(*rxSigDetLive);
        }
        if (auto rxCdrLockLive = lane.cdrLockLive()) {
          cdrLockLive = std::to_string(*rxCdrLockLive);
        }
        if (auto eyes = lane.eyes()) {
          for (const auto& eye : *eyes) {
            if (auto eyeW = eye.width()) {
              eyeWidths.push_back(*eyeW);
            }
            if (auto eyeH = eye.height()) {
              eyeHeights.push_back(*eyeH);
            }
          }
        }
        pmdTable.addRow(
            {"",
             std::to_string(*lane.lane()),
             makeColorCellForLiveFlag(sigDetLive),
             makeColorCellForLiveFlag(cdrLockLive),
             folly::join(",", eyeHeights),
             folly::join(",", eyeWidths)});
      }
      out << pmdTable;
    }
  }

  Table::StyledCell makeColorCellForLiveFlag(const std::string& flag) {
    if (flag == "0") {
      return Table::StyledCell("False", Table::Style::ERROR);
    } else if (flag == "1") {
      return Table::StyledCell("True", Table::Style::GOOD);
    }
    return Table::StyledCell(flag, Table::Style::INFO);
  }

  RetType createModel(
      const HostInfo& hostInfo,
      const utils::PortList& queriedIfs,
      const utils::PhyChipType& phyChipType) {
    RetType model;
    try {
      if (phyChipType.iphyIncluded) {
        auto agentClient =
            utils::createClient<facebook::fboss::FbossCtrlAsyncClient>(
                hostInfo);
        std::map<std::string, phy::PhyInfo> phyInfo;
        agentClient->sync_getInterfacePhyInfo(phyInfo, queriedIfs.data());
        for (auto& interfacePhyInfo : phyInfo) {
          model.phyInfo_ref()[interfacePhyInfo.first].insert(
              {phy::DataPlanePhyChipType::IPHY, interfacePhyInfo.second});
        }
      }
    } catch (apache::thrift::transport::TTransportException& e) {
      std::cerr << "Cannot connect to wedge_agent\n";
    }
    try {
      if (phyChipType.xphyIncluded) {
        auto qsfpClient = utils::createClient<QsfpServiceAsyncClient>(hostInfo);
        std::map<std::string, phy::PhyInfo> phyInfo;
        qsfpClient->sync_getInterfacePhyInfo(phyInfo, queriedIfs.data());
        for (auto& interfacePhyInfo : phyInfo) {
          model.phyInfo_ref()[interfacePhyInfo.first].insert(
              {phy::DataPlanePhyChipType::XPHY, interfacePhyInfo.second});
        }
      }
    } catch (apache::thrift::transport::TTransportException& e) {
      std::cerr << "Cannot connect to qsfp_service\n";
    }
    return model;
  }
};

} // namespace facebook::fboss
