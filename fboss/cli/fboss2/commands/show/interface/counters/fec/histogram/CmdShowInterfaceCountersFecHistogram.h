// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include "fboss/cli/fboss2/CmdHandler.h"
#include "fboss/cli/fboss2/commands/show/interface/counters/fec/CmdShowInterfaceCountersFec.h"
#include "fboss/cli/fboss2/commands/show/interface/counters/fec/histogram/gen-cpp2/model_types.h"
#include "fboss/cli/fboss2/utils/CmdUtils.h"
#include "fboss/cli/fboss2/utils/Table.h"

namespace facebook::fboss {

namespace {
constexpr int kPercentageGreenBins = 40;
constexpr int kPercentageYellowBins = 80;
constexpr int kNumFecBins = 16;
} // namespace

using utils::Table;

struct CmdShowInterfaceCountersFecHistogramTraits : public BaseCommandTraits {
  using ParentCmd = CmdShowInterfaceCountersFec;
  static constexpr utils::ObjectArgTypeId ObjectArgTypeId =
      utils::ObjectArgTypeId::OBJECT_ARG_TYPE_ID_NONE;
  using ObjectArgType = std::monostate;
  using RetType = cli::ShowInterfaceCountersFecHistogramModel;
  static constexpr bool ALLOW_FILTERING = true;
  static constexpr bool ALLOW_AGGREGATION = true;
};

class CmdShowInterfaceCountersFecHistogram
    : public CmdHandler<
          CmdShowInterfaceCountersFecHistogram,
          CmdShowInterfaceCountersFecHistogramTraits> {
 public:
  using ObjectArgType = CmdShowInterfaceCountersFecHistogram::ObjectArgType;
  using RetType = CmdShowInterfaceCountersFecHistogramTraits::RetType;

  RetType queryClient(
      const HostInfo& hostInfo,
      const std::vector<std::string>& queriedIfs,
      const utils::LinkDirection& direction) {
    auto qsfpClient =
        utils::createClient<facebook::fboss::QsfpServiceAsyncClient>(hostInfo);

    std::map<std::string, CdbDatapathSymErrHistogram> portSymErr;
    CdbDatapathSymErrHistogram symErr;

    if (queriedIfs.empty()) {
      return RetType{};
    }

    for (const auto& interface : queriedIfs) {
      qsfpClient->sync_getSymbolErrorHistogram(symErr, interface);
      portSymErr[interface] = symErr;
    }

    return createModel(portSymErr, direction.direction);
  }

  RetType createModel(
      std::map<std::string, CdbDatapathSymErrHistogram>& portSymErr,
      const phy::Direction direction) {
    RetType model;
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

  void printOutput(const RetType& model, std::ostream& out = std::cout) {
    Table table;

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

    for (const auto& [interfaceName, nBitValues] :
         *model.nBitCorrectedWords()) {
      int numBins = nBitValues.nBitCorrectedMax()->size();
      std::vector<Table::RowData> rowData;

      rowData.push_back(interfaceName);
      for (int binId = 0; binId < numBins; binId++) {
        rowData.push_back(
            nBitValues.nBitCorrectedMax()->find(binId) !=
                    nBitValues.nBitCorrectedMax()->end()
                ? styledCw(
                      nBitValues.nBitCorrectedMax()->at(binId), binId, numBins)
                : Table::StyledCell("-", Table::Style::NONE));
      }
      table.addRow(rowData);
    }
    out << table << std::endl;
  }

  Table::StyledCell styledCw(int64_t cw, int binId, int numBins) const {
    int maxGreenBins = (numBins * kPercentageGreenBins) / 100;
    int maxYellowBins = (numBins * kPercentageYellowBins) / 100;

    std::ostringstream outStringStream;
    outStringStream << std::setprecision(3) << cw;
    if (!cw) {
      return Table::StyledCell(outStringStream.str(), Table::Style::GOOD);
    } else {
      if (binId < maxGreenBins) {
        return Table::StyledCell(outStringStream.str(), Table::Style::GOOD);
      } else if (binId < maxYellowBins) {
        return Table::StyledCell(outStringStream.str(), Table::Style::WARN);
      } else {
        return Table::StyledCell(outStringStream.str(), Table::Style::ERROR);
      }
    }
  }
};

} // namespace facebook::fboss
