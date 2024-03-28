// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include "fboss/cli/fboss2/CmdHandler.h"
#include "fboss/cli/fboss2/commands/show/interface/counters/fec/CmdShowInterfaceCountersFec.h"
#include "fboss/cli/fboss2/commands/show/interface/counters/fec/ber/gen-cpp2/model_types.h"
#include "fboss/cli/fboss2/utils/CmdUtils.h"
#include "fboss/cli/fboss2/utils/Table.h"

namespace facebook::fboss {

using utils::Table;

struct CmdShowInterfaceCountersFecBerTraits : public BaseCommandTraits {
  using ParentCmd = CmdShowInterfaceCountersFec;
  static constexpr utils::ObjectArgTypeId ObjectArgTypeId =
      utils::ObjectArgTypeId::OBJECT_ARG_TYPE_ID_NONE;
  using ObjectArgType = std::monostate;
  using RetType = cli::ShowInterfaceCountersFecBerModel;
};

class CmdShowInterfaceCountersFecBer
    : public CmdHandler<
          CmdShowInterfaceCountersFecBer,
          CmdShowInterfaceCountersFecBerTraits> {
 public:
  using ObjectArgType = CmdShowInterfaceCountersFecBerTraits::ObjectArgType;
  using RetType = CmdShowInterfaceCountersFecBerTraits::RetType;

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
    } catch (apache::thrift::transport::TTransportException& e) {
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
    } catch (apache::thrift::transport::TTransportException& e) {
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
      if (auto ber = getPreFecBerFromPhyInfo(phyInfo, phy::Side::LINE)) {
        model.fecBer()[interfaceName][phy::PortComponent::ASIC] = *ber;
      }
      if (xPhyInfo.find(interfaceName) != xPhyInfo.end()) {
        if (auto ber = getPreFecBerFromPhyInfo(
                xPhyInfo.at(interfaceName), phy::Side::SYSTEM)) {
          model.fecBer()[interfaceName][phy::PortComponent::GB_SYSTEM] = *ber;
        }
        if (auto ber = getPreFecBerFromPhyInfo(
                xPhyInfo.at(interfaceName), phy::Side::LINE)) {
          model.fecBer()[interfaceName][phy::PortComponent::GB_LINE] = *ber;
        }
      }
    }
    // Get Transceiver BER
    getPreFecBerFromTransceiverInfo(transceiverInfo, model);

    return model;
  }

  std::optional<double> getPreFecBerFromPhyInfo(
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
        return fecStats->get_preFECBer();
      }
    }
    return std::nullopt;
  }

  void getPreFecBerFromTransceiverInfo(
      const std::map<int, TransceiverInfo>& transceiverInfo,
      RetType& model) {
    for (auto& [tcvrId, tcvrInfo] : transceiverInfo) {
      if (tcvrInfo.tcvrStats()->vdmPerfMonitorStats().has_value()) {
        auto& vdmStats = tcvrInfo.tcvrStats()->vdmPerfMonitorStats().value();
        for (auto& [portName, portSideStats] :
             vdmStats.mediaPortVdmStats().value()) {
          model.fecBer()[portName][phy::PortComponent::TRANSCEIVER_LINE] =
              portSideStats.datapathBER().value().max().value();
        }
        for (auto& [portName, portSideStats] :
             vdmStats.hostPortVdmStats().value()) {
          model.fecBer()[portName][phy::PortComponent::TRANSCEIVER_SYSTEM] =
              portSideStats.datapathBER().value().max().value();
        }
      }
    }
  }

  void printOutput(const RetType& model, std::ostream& out = std::cout) {
    Table table;

    if (model.direction() == phy::Direction::RECEIVE) {
      table.setHeader(
          {"Interface Name", "ASIC", "XPHY_LINE", "TRANSCEIVER_LINE"});
    } else {
      table.setHeader({"Interface Name", "XPHY_SYSTEM", "TRANSCEIVER_SYSTEM"});
    }

    for (const auto& [interfaceName, fecBer] : *model.fecBer()) {
      std::optional<double> iphyBer, xphySystemBer, xphyLineBer, tcvrSystemBer,
          tcvrLineBer;
      if (fecBer.find(phy::PortComponent::ASIC) != fecBer.end()) {
        iphyBer = fecBer.at(phy::PortComponent::ASIC);
      }
      if (fecBer.find(phy::PortComponent::GB_SYSTEM) != fecBer.end()) {
        xphySystemBer = fecBer.at(phy::PortComponent::GB_SYSTEM);
      }
      if (fecBer.find(phy::PortComponent::GB_LINE) != fecBer.end()) {
        xphyLineBer = fecBer.at(phy::PortComponent::GB_LINE);
      }
      if (fecBer.find(phy::PortComponent::TRANSCEIVER_SYSTEM) != fecBer.end()) {
        tcvrSystemBer = fecBer.at(phy::PortComponent::TRANSCEIVER_SYSTEM);
      }
      if (fecBer.find(phy::PortComponent::TRANSCEIVER_LINE) != fecBer.end()) {
        tcvrLineBer = fecBer.at(phy::PortComponent::TRANSCEIVER_LINE);
      }
      if (model.direction() == phy::Direction::RECEIVE) {
        table.addRow({
            interfaceName,
            iphyBer.has_value() ? styledBer(*iphyBer)
                                : Table::StyledCell("-", Table::Style::NONE),
            xphyLineBer.has_value()
                ? styledBer(*xphyLineBer)
                : Table::StyledCell("-", Table::Style::NONE),
            tcvrLineBer.has_value()
                ? styledBer(*tcvrLineBer)
                : Table::StyledCell("-", Table::Style::NONE),
        });
      } else {
        table.addRow({
            interfaceName,
            xphySystemBer.has_value()
                ? styledBer(*xphySystemBer)
                : Table::StyledCell("-", Table::Style::NONE),
            tcvrSystemBer.has_value()
                ? styledBer(*tcvrSystemBer)
                : Table::StyledCell("-", Table::Style::NONE),
        });
      }
    }
    out << table << std::endl;
  }

  Table::StyledCell styledBer(double ber) const {
    std::ostringstream outStringStream;
    outStringStream << std::setprecision(3) << ber;
    if (ber > 1e-5) {
      return Table::StyledCell(outStringStream.str(), Table::Style::ERROR);
    }
    if (ber > 1e-7) {
      return Table::StyledCell(outStringStream.str(), Table::Style::WARN);
    }
    return Table::StyledCell(outStringStream.str(), Table::Style::GOOD);
  }
};

} // namespace facebook::fboss
