// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include "fboss/cli/fboss2/CmdHandler.h"
#include "fboss/cli/fboss2/commands/show/interface/counters/fec/CmdShowInterfaceCountersFec.h"
#include "fboss/cli/fboss2/commands/show/interface/counters/fec/uncorrectable/gen-cpp2/model_types.h"
#include "fboss/cli/fboss2/utils/CmdUtils.h"
#include "fboss/cli/fboss2/utils/Table.h"

namespace facebook::fboss {

using utils::Table;

struct CmdShowInterfaceCountersFecUncorrectableTraits
    : public BaseCommandTraits {
  using ParentCmd = CmdShowInterfaceCountersFec;
  static constexpr utils::ObjectArgTypeId ObjectArgTypeId =
      utils::ObjectArgTypeId::OBJECT_ARG_TYPE_ID_NONE;
  using ObjectArgType = std::monostate;
  using RetType = cli::ShowInterfaceCountersFecUncorrectableModel;
  static constexpr bool ALLOW_FILTERING = true;
  static constexpr bool ALLOW_AGGREGATION = true;
};

class CmdShowInterfaceCountersFecUncorrectable
    : public CmdHandler<
          CmdShowInterfaceCountersFecUncorrectable,
          CmdShowInterfaceCountersFecUncorrectableTraits> {
 public:
  using ObjectArgType =
      CmdShowInterfaceCountersFecUncorrectableTraits::ObjectArgType;
  using RetType = CmdShowInterfaceCountersFecUncorrectableTraits::RetType;

  RetType queryClient(
      const HostInfo& hostInfo,
      const std::vector<std::string>& queriedIfs,
      const utils::LinkDirection& direction) {
    // Get all the counters from Fbagent/Fb303. We will use regex to filter out
    // our desired counters
    auto agentClient =
        utils::createClient<facebook::fboss::FbossCtrlAsyncClient>(hostInfo);
    auto qsfpClient =
        utils::createClient<facebook::fboss::QsfpServiceAsyncClient>(hostInfo);

    std::map<std::string, int64_t> fb303CountersIPhyUcSum;
    std::map<std::string, int64_t> fb303CountersXPhyUcSum;
    std::map<std::string, int64_t> fb303CountersTcvrUcSum;

#ifdef IS_OSS
    // TODO - figure out why getRegexCounters fails for OSS
    agentClient->sync_getCounters(fb303CountersIPhyUcSum);
    qsfpClient->sync_getCounters(fb303CountersTcvrUcSum);
#else
    agentClient->sync_getRegexCounters(
        fb303CountersIPhyUcSum, "^(eth|fab).*fec_uncorrectable_errors.sum.*");
    qsfpClient->sync_getRegexCounters(
        fb303CountersXPhyUcSum, ".*xphy.line.fec_uncorrectable.sum.*");
    qsfpClient->sync_getRegexCounters(
        fb303CountersTcvrUcSum, "^qsfp.*media.uncorrectable.sum.*");
#endif

    std::unordered_set<std::string> distinctInterfaceNames;
    for (const auto& counter : fb303CountersIPhyUcSum) {
      std::vector<std::string> result;
      boost::split(result, counter.first, boost::is_any_of("."));
      distinctInterfaceNames.insert(result[0]);
    }
    std::vector<std::string> ifNames(
        distinctInterfaceNames.begin(), distinctInterfaceNames.end());

    return createModel(
        ifNames,
        fb303CountersIPhyUcSum,
        fb303CountersXPhyUcSum,
        fb303CountersTcvrUcSum,
        queriedIfs,
        direction.direction);
  }

  RetType createModel(
      std::vector<std::string>& ifNames,
      const std::map<std::string, int64_t>& iPhyUcSum,
      const std::map<std::string, int64_t>& xPhyUcSum,
      const std::map<std::string, int64_t>& tcvrUcSum,
      const std::vector<std::string>& queriedIfs,
      const phy::Direction direction) {
    RetType model;
    model.direction() = direction;

    std::unordered_set<std::string> queriedSet(
        queriedIfs.begin(), queriedIfs.end());

    for (std::string interface : ifNames) {
      if (queriedIfs.size() == 0 || queriedSet.count(interface)) {
        std::string counterName;

        counterName = interface + ".fec_uncorrectable_errors.sum";
        if (iPhyUcSum.find(counterName) != iPhyUcSum.end()) {
          model.uncorrectableFrames()[interface][phy::PortComponent::ASIC]
              .totalCount() = iPhyUcSum.at(counterName);
        }

        counterName = interface + ".fec_uncorrectable_errors.sum.600";
        if (iPhyUcSum.find(counterName) != iPhyUcSum.end()) {
          model.uncorrectableFrames()[interface][phy::PortComponent::ASIC]
              .tenMinuteCount() = iPhyUcSum.at(counterName);
        }

        counterName = interface + ".xphy.line.fec_uncorrectable.sum";
        if (xPhyUcSum.find(counterName) != xPhyUcSum.end()) {
          model.uncorrectableFrames()[interface][phy::PortComponent::GB_LINE]
              .totalCount() = xPhyUcSum.at(counterName);
        }

        counterName = interface + ".xphy.line.fec_uncorrectable.sum.600";
        if (xPhyUcSum.find(counterName) != xPhyUcSum.end()) {
          model.uncorrectableFrames()[interface][phy::PortComponent::GB_LINE]
              .tenMinuteCount() = xPhyUcSum.at(counterName);
        }

        counterName =
            "qsfp.interface." + interface + ".media.uncorrectable.sum";
        if (tcvrUcSum.find(counterName) != tcvrUcSum.end()) {
          model
              .uncorrectableFrames()[interface]
                                    [phy::PortComponent::TRANSCEIVER_LINE]
              .totalCount() = tcvrUcSum.at(counterName);
        }

        counterName =
            "qsfp.interface." + interface + ".media.uncorrectable.sum.600";
        if (tcvrUcSum.find(counterName) != tcvrUcSum.end()) {
          model
              .uncorrectableFrames()[interface]
                                    [phy::PortComponent::TRANSCEIVER_LINE]
              .tenMinuteCount() = tcvrUcSum.at(counterName);
        }
      }
    }

    return model;
  }

  void printOutput(const RetType& model, std::ostream& out = std::cout) {
    Table table;

    if (model.direction() == phy::Direction::RECEIVE) {
      table.setHeader(
          {"Interface Name",
           "ASIC",
           "ASIC_10min",
           "XPHY_LINE",
           "XPHY_LINE_10min",
           "TRANSCEIVER_LINE",
           "TRANSCEIVER_LINE_10min"});
    }

    for (const auto& [interfaceName, ucCounters] :
         *model.uncorrectableFrames()) {
      std::optional<int64_t> iphyUc, iphyUc10m, xphyUc, xphyUc10m, tcvrUc,
          tcvrUc10m;

      if (model.direction() == phy::Direction::RECEIVE) {
        if (ucCounters.find(phy::PortComponent::ASIC) != ucCounters.end()) {
          iphyUc = ucCounters.at(phy::PortComponent::ASIC).totalCount().value();
          iphyUc10m =
              ucCounters.at(phy::PortComponent::ASIC).tenMinuteCount().value();
        }

        if (ucCounters.find(phy::PortComponent::GB_LINE) != ucCounters.end()) {
          xphyUc =
              ucCounters.at(phy::PortComponent::GB_LINE).totalCount().value();
          xphyUc10m = ucCounters.at(phy::PortComponent::GB_LINE)
                          .tenMinuteCount()
                          .value();
        }

        if (ucCounters.find(phy::PortComponent::TRANSCEIVER_LINE) !=
            ucCounters.end()) {
          tcvrUc = ucCounters.at(phy::PortComponent::TRANSCEIVER_LINE)
                       .totalCount()
                       .value();
          tcvrUc10m = ucCounters.at(phy::PortComponent::TRANSCEIVER_LINE)
                          .tenMinuteCount()
                          .value();
        }

        table.addRow({
            interfaceName,
            iphyUc.has_value() ? styledUc(*iphyUc)
                               : Table::StyledCell("-", Table::Style::NONE),
            iphyUc10m.has_value() ? styledUc(*iphyUc10m)
                                  : Table::StyledCell("-", Table::Style::NONE),
            xphyUc.has_value() ? styledUc(*xphyUc)
                               : Table::StyledCell("-", Table::Style::NONE),
            xphyUc10m.has_value() ? styledUc(*xphyUc10m)
                                  : Table::StyledCell("-", Table::Style::NONE),
            tcvrUc.has_value() ? styledUc(*tcvrUc)
                               : Table::StyledCell("-", Table::Style::NONE),
            tcvrUc10m.has_value() ? styledUc(*tcvrUc10m)
                                  : Table::StyledCell("-", Table::Style::NONE),
        });
      }
    }
    out << table << std::endl;
  }

  Table::StyledCell styledUc(int64_t uc) const {
    std::ostringstream outStringStream;
    outStringStream << std::to_string(uc);
    if (uc >= 100) {
      return Table::StyledCell(outStringStream.str(), Table::Style::ERROR);
    }
    if (uc >= 1) {
      return Table::StyledCell(outStringStream.str(), Table::Style::WARN);
    }
    return Table::StyledCell(outStringStream.str(), Table::Style::GOOD);
  }
};

} // namespace facebook::fboss
