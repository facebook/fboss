// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/cli/fboss2/commands/show/interface/counters/fec/histogram/CmdShowInterfaceCountersFecHistogram.h"
#include "fboss/cli/fboss2/CmdHandler.cpp"

#include <iomanip>
#include <iostream>
#include <sstream>

#include "fboss/cli/fboss2/utils/Table.h"
#include "fboss/lib/phy/gen-cpp2/phy_types.h"
#include "fboss/qsfp_service/if/gen-cpp2/QsfpService.h"
#include "fboss/qsfp_service/if/gen-cpp2/transceiver_types.h"

namespace facebook::fboss {

namespace {

constexpr int kPercentageGreenBins = 40;
constexpr int kPercentageYellowBins = 80;

utils::Table::StyledCell styledCw(int64_t cw, int binId, int numBins) {
  int maxGreenBins = (numBins * kPercentageGreenBins) / 100;
  int maxYellowBins = (numBins * kPercentageYellowBins) / 100;

  std::ostringstream outStringStream;
  outStringStream << std::setprecision(3) << cw;
  if (!cw) {
    return utils::Table::StyledCell(
        outStringStream.str(), utils::Table::Style::GOOD);
  } else {
    if (binId < maxGreenBins) {
      return utils::Table::StyledCell(
          outStringStream.str(), utils::Table::Style::GOOD);
    } else if (binId < maxYellowBins) {
      return utils::Table::StyledCell(
          outStringStream.str(), utils::Table::Style::WARN);
    } else {
      return utils::Table::StyledCell(
          outStringStream.str(), utils::Table::Style::ERROR);
    }
  }
}

cli::ShowInterfaceCountersFecHistogramModel createModel(
    std::map<std::string, CdbDatapathSymErrHistogram>& portSymErr,
    const phy::Direction direction) {
  cli::ShowInterfaceCountersFecHistogramModel model;
  model.direction() = direction;

  for (auto& [interface, symErr] : portSymErr) {
    auto& sideSymErrHist = (direction == phy::Direction::RECEIVE)
        ? symErr.media().value()
        : symErr.host().value();
    for (auto& [bin, symErrHist] : sideSymErrHist) {
      model.nBitCorrectedWords()[interface].nBitCorrectedMax()[bin] =
          int64_t(symErrHist.nbitSymbolErrorMax().value());
      model.nBitCorrectedWords()[interface].nBitCorrectedAvg()[bin] =
          int64_t(symErrHist.nbitSymbolErrorAvg().value());
      model.nBitCorrectedWords()[interface].nBitCorrectedCur()[bin] =
          int64_t(symErrHist.nbitSymbolErrorCur().value());
    }
  }
  return model;
}

} // namespace

CmdShowInterfaceCountersFecHistogram::RetType
CmdShowInterfaceCountersFecHistogram::queryClient(
    const HostInfo& hostInfo,
    const std::vector<std::string>& queriedIfs,
    const utils::LinkDirection& direction) {
  std::map<std::string, CdbDatapathSymErrHistogram> portSymErr;
  try {
    auto qsfpClient =
        utils::createClient<apache::thrift::Client<QsfpService>>(hostInfo);

    bool showAllInterfaces = queriedIfs.empty();
    std::vector<std::string> interfaces = queriedIfs;
    if (showAllInterfaces) {
      // getPortMediaInterface only reports ports with a transceiver
      // present, unlike wedge_agent's getAllPortInfo.
      try {
        std::map<std::string, MediaInterfaceCode> mediaInterfaces;
        qsfpClient->sync_getPortMediaInterface(mediaInterfaces);
        for (const auto& [portName, unused] : mediaInterfaces) {
          interfaces.push_back(portName);
        }
      } catch (const thrift::FbossBaseError&) {
      }
    }

    for (const auto& interface : interfaces) {
      CdbDatapathSymErrHistogram symErr;
      try {
        qsfpClient->sync_getSymbolErrorHistogram(symErr, interface);
      } catch (const thrift::FbossBaseError& ex) {
        if (!showAllInterfaces) {
          std::cerr << "Skipping " << interface << ": " << ex.what() << "\n";
        }
        continue;
      }
      portSymErr[interface] = symErr;
    }
  } catch (const apache::thrift::transport::TTransportException&) {
    std::cerr << "Cannot connect to qsfp_service\n";
  }

  return createModel(portSymErr, direction.direction);
}

void CmdShowInterfaceCountersFecHistogram::printOutput(
    const RetType& model,
    std::ostream& out) {
  utils::Table table;

  table.setHeader(
      {(model.direction() == phy::Direction::RECEIVE) ? "Interface Rx"
                                                      : "Interface Tx",
       "Bin0",
       "Bin1",
       "Bin2",
       "Bin3",
       "Bin4",
       "Bin5",
       "Bin6",
       "Bin7",
       "Bin8",
       "Bin9",
       "Bin10",
       "Bin11",
       "Bin12",
       "Bin13",
       "Bin14",
       "Bin15"});

  for (const auto& [interfaceName, nBitValues] : *model.nBitCorrectedWords()) {
    int numBins = static_cast<int>(nBitValues.nBitCorrectedMax()->size());
    std::vector<utils::Table::RowData> rowData;

    rowData.emplace_back(interfaceName);
    for (int binId = 0; binId < numBins; binId++) {
      rowData.emplace_back(
          nBitValues.nBitCorrectedMax()->find(binId) !=
                  nBitValues.nBitCorrectedMax()->end()
              ? styledCw(
                    nBitValues.nBitCorrectedMax()->at(binId), binId, numBins)
              : utils::Table::StyledCell("-", utils::Table::Style::NONE));
    }
    table.addRow(rowData);
  }
  out << table << std::endl;
}

// Template instantiations
template void CmdHandler<
    CmdShowInterfaceCountersFecHistogram,
    CmdShowInterfaceCountersFecHistogramTraits>::run();
template const ValidFilterMapType CmdHandler<
    CmdShowInterfaceCountersFecHistogram,
    CmdShowInterfaceCountersFecHistogramTraits>::getValidFilters();

} // namespace facebook::fboss
