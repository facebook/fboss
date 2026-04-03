// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/cli/fboss2/commands/show/interface/counters/fec/tail/CmdShowInterfaceCountersFecTail.h"
#include "fboss/cli/fboss2/CmdHandler.cpp"

#include <iostream>

#include "fboss/agent/if/gen-cpp2/FbossCtrl.h"
#include "fboss/cli/fboss2/utils/Table.h"
#include "fboss/lib/phy/gen-cpp2/phy_types.h"
#include "fboss/qsfp_service/if/gen-cpp2/QsfpService.h"

namespace facebook::fboss {

namespace {

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
    cli::ShowInterfaceCountersFecTailModel& model) {
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

cli::ShowInterfaceCountersFecTailModel createModel(
    const std::map<std::string, phy::PhyInfo>& iPhyInfo,
    const std::map<std::string, phy::PhyInfo>& xPhyInfo,
    const std::map<int, TransceiverInfo>& transceiverInfo,
    const phy::Direction direction) {
  cli::ShowInterfaceCountersFecTailModel model;
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
        model.fecTail()[interfaceName][phy::PortComponent::GB_LINE] = *fecTail;
      }
    }
  }
  // Get Transceiver fec Tail
  getPreFecTailFromTransceiverInfo(transceiverInfo, model);

  model.hasXphy() = xPhyInfo.size() > 0;
  return model;
}

} // namespace

CmdShowInterfaceCountersFecTail::RetType
CmdShowInterfaceCountersFecTail::queryClient(
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

void CmdShowInterfaceCountersFecTail::printOutput(
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
    if (fecTail.find(phy::PortComponent::TRANSCEIVER_SYSTEM) != fecTail.end()) {
      tcvrSystemFecTail = fecTail.at(phy::PortComponent::TRANSCEIVER_SYSTEM);
    }
    if (fecTail.find(phy::PortComponent::TRANSCEIVER_LINE) != fecTail.end()) {
      tcvrLineFecTail = fecTail.at(phy::PortComponent::TRANSCEIVER_LINE);
    }
    std::vector<facebook::fboss::utils::Table::RowData> rowData = {
        interfaceName};
    if (model.direction() == phy::Direction::RECEIVE) {
      rowData.emplace_back(
          iphyFecTail.has_value()
              ? utils::styledFecTail(*iphyFecTail)
              : utils::Table::StyledCell("-", utils::Table::Style::NONE));
      if (hasXphy) {
        rowData.emplace_back(
            xphyLineFecTail.has_value()
                ? utils::styledFecTail(*xphyLineFecTail)
                : utils::Table::StyledCell("-", utils::Table::Style::NONE));
      }
      rowData.emplace_back(
          tcvrLineFecTail.has_value()
              ? utils::styledFecTail(*tcvrLineFecTail)
              : utils::Table::StyledCell("-", utils::Table::Style::NONE));
    } else {
      if (hasXphy) {
        rowData.emplace_back(
            xphySystemFecTail.has_value()
                ? utils::styledFecTail(*xphySystemFecTail)
                : utils::Table::StyledCell("-", utils::Table::Style::NONE));
      }
      rowData.emplace_back(
          tcvrSystemFecTail.has_value()
              ? utils::styledFecTail(*tcvrSystemFecTail)
              : utils::Table::StyledCell("-", utils::Table::Style::NONE));
    }
    table.addRow(rowData);
  }
  out << table << std::endl;
}

// Template instantiations
template void CmdHandler<
    CmdShowInterfaceCountersFecTail,
    CmdShowInterfaceCountersFecTailTraits>::run();
template const ValidFilterMapType CmdHandler<
    CmdShowInterfaceCountersFecTail,
    CmdShowInterfaceCountersFecTailTraits>::getValidFilters();

} // namespace facebook::fboss
