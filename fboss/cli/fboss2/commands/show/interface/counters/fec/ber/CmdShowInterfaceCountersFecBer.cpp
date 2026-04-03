// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/cli/fboss2/commands/show/interface/counters/fec/ber/CmdShowInterfaceCountersFecBer.h"
#include "fboss/cli/fboss2/CmdHandler.cpp"

#include <iostream>

#include "fboss/agent/if/gen-cpp2/FbossCtrl.h"
#include "fboss/cli/fboss2/utils/Table.h"
#include "fboss/lib/phy/gen-cpp2/phy_types.h"
#include "fboss/qsfp_service/if/gen-cpp2/QsfpService.h"

namespace facebook::fboss {

namespace {

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
      return folly::copy(fecStats->preFECBer().value());
    }
  }
  return std::nullopt;
}

void getPreFecBerFromTransceiverInfo(
    const std::map<int, TransceiverInfo>& transceiverInfo,
    cli::ShowInterfaceCountersFecBerModel& model) {
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

cli::ShowInterfaceCountersFecBerModel createModel(
    const std::map<std::string, phy::PhyInfo>& iPhyInfo,
    const std::map<std::string, phy::PhyInfo>& xPhyInfo,
    const std::map<int, TransceiverInfo>& transceiverInfo,
    const phy::Direction direction) {
  cli::ShowInterfaceCountersFecBerModel model;
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

  model.hasXphy() = xPhyInfo.size() > 0;
  return model;
}

} // namespace

CmdShowInterfaceCountersFecBer::RetType
CmdShowInterfaceCountersFecBer::queryClient(
    const HostInfo& hostInfo,
    const utils::PortList& queriedIfs,
    const utils::LinkDirection& direction) {
  std::map<std::string, phy::PhyInfo> iPhyInfo;
  std::map<std::string, phy::PhyInfo> xPhyInfo;
  std::map<int, TransceiverInfo> transceiverInfo;

  try {
    auto agentClient =
        utils::createClient<apache::thrift::Client<FbossCtrl>>(hostInfo);
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
        utils::createClient<apache::thrift::Client<QsfpService>>(hostInfo);
    if (queriedIfs.empty()) {
      qsfpClient->sync_getAllInterfacePhyInfo(xPhyInfo);
    } else {
      qsfpClient->sync_getInterfacePhyInfo(xPhyInfo, queriedIfs.data());
    }
    qsfpClient->sync_getTransceiverInfo(transceiverInfo, {});
  } catch (apache::thrift::transport::TTransportException&) {
    std::cerr << "Cannot connect to qsfp_service\n";
  }
  return createModel(iPhyInfo, xPhyInfo, transceiverInfo, direction.direction);
}

void CmdShowInterfaceCountersFecBer::printOutput(
    const RetType& model,
    std::ostream& out) {
  utils::Table table;
  bool hasXphy = model.hasXphy().value();

  std::vector<facebook::fboss::utils::Table::RowData> header = {
      "Interface Name"};
  if (model.direction() == phy::Direction::RECEIVE) {
    header.emplace_back("ASIC");
    if (hasXphy) {
      header.emplace_back("XPHY_LINE");
    }
    header.emplace_back("TRANSCEIVER_LINE");
  } else {
    if (hasXphy) {
      header.emplace_back("XPHY_SYSTEM");
    }
    header.emplace_back("TRANSCEIVER_SYSTEM");
  }
  table.setHeader(header);
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
    std::vector<facebook::fboss::utils::Table::RowData> rowData = {
        interfaceName};
    if (model.direction() == phy::Direction::RECEIVE) {
      rowData.emplace_back(
          iphyBer.has_value()
              ? utils::styledBer(*iphyBer)
              : utils::Table::StyledCell("-", utils::Table::Style::NONE));
      if (hasXphy) {
        rowData.emplace_back(
            xphyLineBer.has_value()
                ? utils::styledBer(*xphyLineBer)
                : utils::Table::StyledCell("-", utils::Table::Style::NONE));
      }
      rowData.emplace_back(
          tcvrLineBer.has_value()
              ? utils::styledBer(*tcvrLineBer)
              : utils::Table::StyledCell("-", utils::Table::Style::NONE));
    } else {
      if (hasXphy) {
        rowData.emplace_back(
            xphySystemBer.has_value()
                ? utils::styledBer(*xphySystemBer)
                : utils::Table::StyledCell("-", utils::Table::Style::NONE));
      }
      rowData.emplace_back(
          tcvrSystemBer.has_value()
              ? utils::styledBer(*tcvrSystemBer)
              : utils::Table::StyledCell("-", utils::Table::Style::NONE));
    }
    table.addRow(rowData);
  }
  out << table << std::endl;
}

// Template instantiations
template void CmdHandler<
    CmdShowInterfaceCountersFecBer,
    CmdShowInterfaceCountersFecBerTraits>::run();
template const ValidFilterMapType CmdHandler<
    CmdShowInterfaceCountersFecBer,
    CmdShowInterfaceCountersFecBerTraits>::getValidFilters();

} // namespace facebook::fboss
