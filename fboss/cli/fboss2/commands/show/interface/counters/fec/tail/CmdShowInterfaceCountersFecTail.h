// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include "fboss/cli/fboss2/CmdHandler.h"
#include "fboss/cli/fboss2/commands/show/interface/counters/fec/CmdShowInterfaceCountersFec.h"
#include "fboss/cli/fboss2/commands/show/interface/counters/fec/tail/gen-cpp2/model_types.h"
#include "fboss/cli/fboss2/utils/CmdUtils.h"
#include "fboss/cli/fboss2/utils/Table.h"

namespace facebook::fboss {

using utils::Table;

struct CmdShowInterfaceCountersFecTailTraits : public BaseCommandTraits {
  using ParentCmd = CmdShowInterfaceCountersFec;
  static constexpr utils::ObjectArgTypeId ObjectArgTypeId =
      utils::ObjectArgTypeId::OBJECT_ARG_TYPE_ID_NONE;
  using ObjectArgType = std::monostate;
  using RetType = cli::ShowInterfaceCountersFecTailModel;
};

class CmdShowInterfaceCountersFecTail
    : public CmdHandler<
          CmdShowInterfaceCountersFecTail,
          CmdShowInterfaceCountersFecTailTraits> {
 public:
  using ObjectArgType = CmdShowInterfaceCountersFecTailTraits::ObjectArgType;
  using RetType = CmdShowInterfaceCountersFecTailTraits::RetType;

  RetType queryClient(
      const HostInfo& hostInfo,
      const utils::PortList& queriedIfs,
      const utils::LinkDirection& direction) {
    std::map<std::string, phy::PhyInfo> iPhyInfo;
    std::map<std::string, phy::PhyInfo> xPhyInfo;
    std::map<int, TransceiverInfo> transceiverInfo;

    try {
      auto agentClient =
          utils::createClient<facebook::fboss::FbossCtrlAsyncClient>(hostInfo);
      if (queriedIfs.empty()) {
        agentClient->sync_getAllInterfacePhyInfo(iPhyInfo);
      } else {
        agentClient->sync_getInterfacePhyInfo(iPhyInfo, queriedIfs.data());
      }
    } catch (apache::thrift::transport::TTransportException&) {
      std::cerr << "Cannot connect to wedge_agent\n";
    }

    try {
      auto qsfpClient =
          utils::createClient<facebook::fboss::QsfpServiceAsyncClient>(
              hostInfo);
      if (queriedIfs.empty()) {
        qsfpClient->sync_getAllInterfacePhyInfo(xPhyInfo);
      } else {
        qsfpClient->sync_getInterfacePhyInfo(xPhyInfo, queriedIfs.data());
      }
      qsfpClient->sync_getTransceiverInfo(transceiverInfo, {});
    } catch (apache::thrift::transport::TTransportException&) {
      std::cerr << "Cannot connect to qsfp_service\n";
    }
    return createModel(
        iPhyInfo, xPhyInfo, transceiverInfo, direction.direction);
  }

  RetType createModel(
      const std::map<std::string, phy::PhyInfo>& iPhyInfo,
      const std::map<std::string, phy::PhyInfo>& xPhyInfo,
      const std::map<int, TransceiverInfo>& transceiverInfo,
      const phy::Direction direction) {
    RetType model;
    model.direction() = direction;
    // Use interfaces in iPhyInfo as the SOT for the ports in the system
    for (auto& [interfaceName, phyInfo] : iPhyInfo) {
      if (auto fecTail = getPreFecTailFromPhyInfo(phyInfo, phy::Side::LINE)) {
        model.fecTail()[interfaceName][phy::PortComponent::ASIC] = *fecTail;
      }
      if (xPhyInfo.find(interfaceName) != xPhyInfo.end()) {
        if (auto fecTail = getPreFecTailFromPhyInfo(
                xPhyInfo.at(interfaceName), phy::Side::SYSTEM)) {
          model.fecTail()[interfaceName][phy::PortComponent::GB_SYSTEM] =
              *fecTail;
        }
        if (auto fecTail = getPreFecTailFromPhyInfo(
                xPhyInfo.at(interfaceName), phy::Side::LINE)) {
          model.fecTail()[interfaceName][phy::PortComponent::GB_LINE] =
              *fecTail;
        }
      }
    }
    // Get Transceiver fec Tail
    getPreFecTailFromTransceiverInfo(transceiverInfo, model);

    model.hasXphy() = xPhyInfo.size() > 0;
    return model;
  }

  std::optional<double> getPreFecTailFromPhyInfo(
      const phy::PhyInfo& phyInfo,
      phy::Side side) {
    auto& stats = *phyInfo.stats();
    std::optional<phy::PhySideStats> sideStats;
    if (side == phy::Side::LINE) {
      sideStats = *stats.line();
    } else if (side == phy::Side::SYSTEM && stats.system()) {
      sideStats = *stats.system();
    }
    if (sideStats && sideStats->pcs()) {
      auto& pcsStats = *sideStats->pcs();
      if (auto fecStats = pcsStats.rsFec()) {
        if (fecStats->fecTail().has_value()) {
          return *fecStats->fecTail();
        }
      }
    }
    return std::nullopt;
  }

  void getPreFecTailFromTransceiverInfo(
      const std::map<int, TransceiverInfo>& transceiverInfo,
      RetType& model) {
    for (auto& [tcvrId, tcvrInfo] : transceiverInfo) {
      if (tcvrInfo.tcvrStats()->vdmPerfMonitorStats().has_value()) {
        auto& vdmStats = tcvrInfo.tcvrStats()->vdmPerfMonitorStats().value();
        for (auto& [portName, portSideStats] :
             vdmStats.mediaPortVdmStats().value()) {
          if (portSideStats.fecTailMax().has_value()) {
            model.fecTail()[portName][phy::PortComponent::TRANSCEIVER_LINE] =
                portSideStats.fecTailMax().value();
          }
        }
        for (auto& [portName, portSideStats] :
             vdmStats.hostPortVdmStats().value()) {
          if (portSideStats.fecTailMax().has_value()) {
            model.fecTail()[portName][phy::PortComponent::TRANSCEIVER_SYSTEM] =
                portSideStats.fecTailMax().value();
          }
        }
      }
    }
  }

  void printOutput(const RetType& model, std::ostream& out = std::cout) {
    Table table;
    bool hasXphy = model.hasXphy().value();

    std::vector<facebook::fboss::utils::Table::RowData> header = {
        "Interface Name"};
    if (model.direction() == phy::Direction::RECEIVE) {
      header.push_back("ASIC");
      if (hasXphy) {
        header.push_back("XPHY_LINE");
      }
      header.push_back("TRANSCEIVER_LINE");
    } else {
      if (hasXphy) {
        header.push_back("XPHY_SYSTEM");
      }
      header.push_back("TRANSCEIVER_SYSTEM");
    }
    table.setHeader(header);
    for (const auto& [interfaceName, fecTail] : *model.fecTail()) {
      std::optional<double> iphyFecTail, xphySystemFecTail, xphyLineFecTail,
          tcvrSystemFecTail, tcvrLineFecTail;
      if (fecTail.find(phy::PortComponent::ASIC) != fecTail.end()) {
        iphyFecTail = fecTail.at(phy::PortComponent::ASIC);
      }
      if (fecTail.find(phy::PortComponent::GB_SYSTEM) != fecTail.end()) {
        xphySystemFecTail = fecTail.at(phy::PortComponent::GB_SYSTEM);
      }
      if (fecTail.find(phy::PortComponent::GB_LINE) != fecTail.end()) {
        xphyLineFecTail = fecTail.at(phy::PortComponent::GB_LINE);
      }
      if (fecTail.find(phy::PortComponent::TRANSCEIVER_SYSTEM) !=
          fecTail.end()) {
        tcvrSystemFecTail = fecTail.at(phy::PortComponent::TRANSCEIVER_SYSTEM);
      }
      if (fecTail.find(phy::PortComponent::TRANSCEIVER_LINE) != fecTail.end()) {
        tcvrLineFecTail = fecTail.at(phy::PortComponent::TRANSCEIVER_LINE);
      }
      std::vector<facebook::fboss::utils::Table::RowData> rowData = {
          interfaceName};
      if (model.direction() == phy::Direction::RECEIVE) {
        rowData.push_back({
            iphyFecTail.has_value()
                ? utils::styledFecTail(*iphyFecTail)
                : Table::StyledCell("-", Table::Style::NONE),
        });
        if (hasXphy) {
          rowData.push_back(
              xphyLineFecTail.has_value()
                  ? utils::styledFecTail(*xphyLineFecTail)
                  : Table::StyledCell("-", Table::Style::NONE));
        }
        rowData.push_back(
            tcvrLineFecTail.has_value()
                ? utils::styledFecTail(*tcvrLineFecTail)
                : Table::StyledCell("-", Table::Style::NONE));
      } else {
        if (hasXphy) {
          rowData.push_back(
              xphySystemFecTail.has_value()
                  ? utils::styledFecTail(*xphySystemFecTail)
                  : Table::StyledCell("-", Table::Style::NONE));
        }
        rowData.push_back(
            tcvrSystemFecTail.has_value()
                ? utils::styledFecTail(*tcvrSystemFecTail)
                : Table::StyledCell("-", Table::Style::NONE));
      }
      table.addRow(rowData);
    }
    out << table << std::endl;
  }
};

} // namespace facebook::fboss
