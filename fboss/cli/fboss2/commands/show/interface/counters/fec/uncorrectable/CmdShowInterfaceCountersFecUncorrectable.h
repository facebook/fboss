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

    std::map<std::string, int64_t> fb303CountersIPhyIngressUcSum;
    std::map<std::string, int64_t> fb303CountersXPhyIngressUcSum;
    std::map<std::string, int64_t> fb303CountersTcvrIngressUcSum;
    std::map<std::string, int64_t> fb303CountersXPhyEgressUcSum;
    std::map<std::string, int64_t> fb303CountersTcvrEgressUcSum;

#ifdef IS_OSS
    // TODO - figure out why getRegexCounters fails for OSS
    agentClient->sync_getCounters(fb303CountersIPhyIngressUcSum);
    qsfpClient->sync_getCounters(fb303CountersXPhyIngressUcSum);
    qsfpClient->sync_getCounters(fb303CountersTcvrIngressUcSum);
    qsfpClient->sync_getCounters(fb303CountersXPhyEgressUcSum);
    qsfpClient->sync_getCounters(fb303CountersTcvrEgressUcSum);
#else
    fb303CountersIPhyIngressUcSum = utils::getAgentFb303RegexCounters(
        hostInfo, "^(eth|fab).*fec_uncorrectable_errors.sum.*");
    qsfpClient->sync_getRegexCounters(
        fb303CountersXPhyIngressUcSum, ".*xphy.line.fec_uncorrectable.sum.*");
    qsfpClient->sync_getRegexCounters(
        fb303CountersTcvrIngressUcSum, "^qsfp.*media.uncorrectable.sum.*");
    qsfpClient->sync_getRegexCounters(
        fb303CountersXPhyEgressUcSum, ".*xphy.system.fec_uncorrectable.sum.*");
    qsfpClient->sync_getRegexCounters(
        fb303CountersTcvrEgressUcSum, "^qsfp.*host.uncorrectable.sum.*");
#endif

    std::unordered_set<std::string> distinctInterfaceNames;
    for (const auto& counter : fb303CountersIPhyIngressUcSum) {
      std::vector<std::string> result;
      boost::split(result, counter.first, boost::is_any_of("."));
      distinctInterfaceNames.insert(result[0]);
    }
    std::vector<std::string> ifNames(
        distinctInterfaceNames.begin(), distinctInterfaceNames.end());

    return createModel(
        ifNames,
        fb303CountersIPhyIngressUcSum,
        fb303CountersXPhyIngressUcSum,
        fb303CountersTcvrIngressUcSum,
        fb303CountersXPhyEgressUcSum,
        fb303CountersTcvrEgressUcSum,
        queriedIfs,
        direction.direction);
  }

  RetType createModel(
      std::vector<std::string>& ifNames,
      const std::map<std::string, int64_t>& iPhyIngressUcSum,
      const std::map<std::string, int64_t>& xPhyIngressUcSum,
      const std::map<std::string, int64_t>& tcvrIngressUcSum,
      const std::map<std::string, int64_t>& xPhyEgressUcSum,
      const std::map<std::string, int64_t>& tcvrEgressUcSum,
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
        if (iPhyIngressUcSum.find(counterName) != iPhyIngressUcSum.end()) {
          model.uncorrectableFrames()[interface][phy::PortComponent::ASIC]
              .totalCount() = iPhyIngressUcSum.at(counterName);
        }

        counterName = interface + ".fec_uncorrectable_errors.sum.600";
        if (iPhyIngressUcSum.find(counterName) != iPhyIngressUcSum.end()) {
          model.uncorrectableFrames()[interface][phy::PortComponent::ASIC]
              .tenMinuteCount() = iPhyIngressUcSum.at(counterName);
        }

        counterName = interface + ".xphy.line.fec_uncorrectable.sum";
        if (xPhyIngressUcSum.find(counterName) != xPhyIngressUcSum.end()) {
          model.uncorrectableFrames()[interface][phy::PortComponent::GB_LINE]
              .totalCount() = xPhyIngressUcSum.at(counterName);
        }

        counterName = interface + ".xphy.line.fec_uncorrectable.sum.600";
        if (xPhyIngressUcSum.find(counterName) != xPhyIngressUcSum.end()) {
          model.uncorrectableFrames()[interface][phy::PortComponent::GB_LINE]
              .tenMinuteCount() = xPhyIngressUcSum.at(counterName);
        }

        counterName =
            "qsfp.interface." + interface + ".media.uncorrectable.sum";
        if (tcvrIngressUcSum.find(counterName) != tcvrIngressUcSum.end()) {
          model
              .uncorrectableFrames()[interface]
                                    [phy::PortComponent::TRANSCEIVER_LINE]
              .totalCount() = tcvrIngressUcSum.at(counterName);
        }

        counterName =
            "qsfp.interface." + interface + ".media.uncorrectable.sum.600";
        if (tcvrIngressUcSum.find(counterName) != tcvrIngressUcSum.end()) {
          model
              .uncorrectableFrames()[interface]
                                    [phy::PortComponent::TRANSCEIVER_LINE]
              .tenMinuteCount() = tcvrIngressUcSum.at(counterName);
        }

        counterName = interface + ".xphy.system.fec_uncorrectable.sum";
        if (xPhyEgressUcSum.find(counterName) != xPhyEgressUcSum.end()) {
          model.uncorrectableFrames()[interface][phy::PortComponent::GB_SYSTEM]
              .totalCount() = xPhyEgressUcSum.at(counterName);
        }

        counterName = interface + ".xphy.system.fec_uncorrectable.sum.600";
        if (xPhyEgressUcSum.find(counterName) != xPhyEgressUcSum.end()) {
          model.uncorrectableFrames()[interface][phy::PortComponent::GB_SYSTEM]
              .tenMinuteCount() = xPhyEgressUcSum.at(counterName);
        }

        counterName = "qsfp.interface." + interface + ".host.uncorrectable.sum";
        if (tcvrEgressUcSum.find(counterName) != tcvrEgressUcSum.end()) {
          model
              .uncorrectableFrames()[interface]
                                    [phy::PortComponent::TRANSCEIVER_SYSTEM]
              .totalCount() = tcvrEgressUcSum.at(counterName);
        }

        counterName =
            "qsfp.interface." + interface + ".host.uncorrectable.sum.600";
        if (tcvrEgressUcSum.find(counterName) != tcvrEgressUcSum.end()) {
          model
              .uncorrectableFrames()[interface]
                                    [phy::PortComponent::TRANSCEIVER_SYSTEM]
              .tenMinuteCount() = tcvrEgressUcSum.at(counterName);
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
    } else {
      table.setHeader(
          {"Interface Name",
           "XPHY_SYSTEM",
           "XPHY_SYSTEM_10min",
           "TRANSCEIVER_SYSTEM",
           "TRANSCEIVER_SYSTEM_10min"});
    }

    for (const auto& [interfaceName, ucCounters] :
         *model.uncorrectableFrames()) {
      if (model.direction() == phy::Direction::RECEIVE) {
        std::optional<int64_t> iphyUc, iphyUc10m, xphyUc, xphyUc10m, tcvrUc,
            tcvrUc10m;

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
      } else {
        std::optional<int64_t> xphyUc, xphyUc10m, tcvrUc, tcvrUc10m;

        if (ucCounters.find(phy::PortComponent::GB_SYSTEM) !=
            ucCounters.end()) {
          xphyUc =
              ucCounters.at(phy::PortComponent::GB_SYSTEM).totalCount().value();
          xphyUc10m = ucCounters.at(phy::PortComponent::GB_SYSTEM)
                          .tenMinuteCount()
                          .value();
        }

        if (ucCounters.find(phy::PortComponent::TRANSCEIVER_SYSTEM) !=
            ucCounters.end()) {
          tcvrUc = ucCounters.at(phy::PortComponent::TRANSCEIVER_SYSTEM)
                       .totalCount()
                       .value();
          tcvrUc10m = ucCounters.at(phy::PortComponent::TRANSCEIVER_SYSTEM)
                          .tenMinuteCount()
                          .value();
        }

        table.addRow({
            interfaceName,
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
