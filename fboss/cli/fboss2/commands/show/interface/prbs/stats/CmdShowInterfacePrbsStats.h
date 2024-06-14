// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include <fboss/agent/if/gen-cpp2/ctrl_types.h>
#include <folly/String.h>
#include <folly/gen/Base.h>
#include <cstdint>
#include "fboss/agent/if/gen-cpp2/common_types.h"
#include "fboss/cli/fboss2/CmdHandler.h"
#include "fboss/cli/fboss2/commands/show/interface/prbs/CmdShowInterfacePrbs.h"
#include "fboss/cli/fboss2/commands/show/interface/prbs/stats/gen-cpp2/model_types.h"
#include "fboss/cli/fboss2/utils/CmdUtils.h"
#include "fboss/cli/fboss2/utils/PrbsUtils.h"
#include "fboss/cli/fboss2/utils/Table.h"
#include "fboss/lib/phy/gen-cpp2/phy_types.h"
#include "thrift/lib/cpp/util/EnumUtils.h"

using std::chrono::duration_cast;
using std::chrono::seconds;
using std::chrono::system_clock;

namespace facebook::fboss {

struct CmdShowInterfacePrbsStatsTraits : public BaseCommandTraits {
  using ParentCmd = CmdShowInterfacePrbs;
  static constexpr utils::ObjectArgTypeId ObjectArgTypeId =
      utils::ObjectArgTypeId::OBJECT_ARG_TYPE_ID_NONE;
  using ObjectArgType = std::monostate;
  using RetType = cli::ShowPrbsStatsModel;
};

class CmdShowInterfacePrbsStats : public CmdHandler<
                                      CmdShowInterfacePrbsStats,
                                      CmdShowInterfacePrbsStatsTraits> {
 public:
  RetType queryClient(
      const HostInfo& hostInfo,
      const std::vector<std::string>& queriedIfs,
      const std::vector<std::string>& components) {
    std::map<phy::PortComponent, std::map<std::string, phy::PrbsStats>>
        allPrbsStats;
    auto queriedPrbsComponents =
        prbsComponents(components, true /* returnAllIfEmpty */);
    for (const auto& component : queriedPrbsComponents) {
      auto prbsStats = getComponentPrbsStats(hostInfo, component);
      allPrbsStats[component] = prbsStats;
    }
    if (queriedIfs.empty()) {
      std::vector<std::string> allIfs;
      auto agentClient =
          utils::createClient<apache::thrift::Client<FbossCtrl>>(hostInfo);
      agentClient->sync_getAllPortInfo(portEntries_);
      for (const auto& port : portEntries_) {
        allIfs.push_back(port.second.get_name());
      }
      return createModel(hostInfo, allIfs, queriedPrbsComponents, allPrbsStats);
    } else {
      return createModel(
          hostInfo, queriedIfs, queriedPrbsComponents, allPrbsStats);
    }
  }

  void printOutput(const RetType& model, std::ostream& out = std::cout) {
    Table table;
    table.setHeader(
        {"Interface",
         "Component",
         "Lane",
         "Locked",
         "BER",
         "Max BER",
         "SNR",
         "Max SNR",
         "Num Loss Of Lock",
         "Time Since Last Lock",
         "Time Since Last Clear",
         "Time Since Last Update"});
    auto now = duration_cast<seconds>(system_clock::now().time_since_epoch());
    for (const auto& intfEntry : *model.interfaceEntries()) {
      for (const auto& entry : *intfEntry.componentEntries()) {
        auto stats = *(entry.prbsStats());
        auto interface = *entry.interfaceName();
        auto component = apache::thrift::util::enumNameSafe(*entry.component());
        if (stats.laneStats()->empty()) {
          continue;
        }
        for (auto laneStat : *stats.laneStats()) {
          std::ostringstream ber, maxBer, snr, maxSnr;
          ber << *laneStat.ber();
          maxBer << *laneStat.maxBer();
          snr << (laneStat.snr().has_value() ? *laneStat.snr() : 0);
          maxSnr << (laneStat.maxSnr().has_value() ? *laneStat.maxSnr() : 0);
          std::string lastClear = *laneStat.timeSinceLastClear()
              ? utils::getPrettyElapsedTime(
                    now.count() - *laneStat.timeSinceLastClear())
              : "N/A";
          std::string lastUpdate = *laneStat.timeCollected()
              ? utils::getPrettyElapsedTime(*laneStat.timeCollected())
              : "N/A";
          table.addRow(
              {interface,
               component,
               (*laneStat.laneId() == -1) ? "-"
                                          : std::to_string(*laneStat.laneId()),
               styledLocked(*laneStat.locked()),
               utils::styledBer(*laneStat.ber()),
               utils::styledBer(*laneStat.maxBer()),
               snr.str(),
               maxSnr.str(),
               styledNumLossOfLock(*laneStat.numLossOfLock()),
               utils::getPrettyElapsedTime(
                   now.count() - *laneStat.timeSinceLastLocked()),
               lastClear,
               lastUpdate});
        }
      }
    }
    out << table << std::endl;
  }

  RetType createModel(
      const HostInfo& hostInfo,
      const std::vector<std::string>& queriedIfs,
      const std::vector<phy::PortComponent>& components,
      const std::map<phy::PortComponent, std::map<std::string, phy::PrbsStats>>&
          allPrbsStats) {
    RetType model;
    for (const auto& intf : queriedIfs) {
      cli::PrbsStatsInterfaceEntry intfEntry;
      for (const auto& component : components) {
        cli::PrbsStatsComponentEntry entry;
        entry.interfaceName() = intf;
        entry.component() = component;
        entry.prbsStats() = getPrbsStats(allPrbsStats, intf, component);
        intfEntry.componentEntries()->push_back(entry);
      }
      if (intfEntry.componentEntries()->size()) {
        model.interfaceEntries()->push_back(intfEntry);
      }
    }
    return model;
  }

  std::map<std::string, phy::PrbsStats> getComponentPrbsStats(
      const HostInfo& hostInfo,
      const phy::PortComponent& component) {
    std::map<std::string, phy::PrbsStats> prbsStats;
    if (component == phy::PortComponent::TRANSCEIVER_LINE ||
        component == phy::PortComponent::TRANSCEIVER_SYSTEM ||
        component == phy::PortComponent::GB_LINE ||
        component == phy::PortComponent::GB_SYSTEM) {
      auto qsfpClient =
          utils::createClient<apache::thrift::Client<QsfpService>>(hostInfo);
      queryPrbsStats<apache::thrift::Client<QsfpService>>(
          std::move(qsfpClient), prbsStats, component);
    } else {
      auto numHwSwitches = utils::getNumHwSwitches(hostInfo);
      for (int i = 0; i < numHwSwitches; i++) {
        std::map<std::string, phy::PrbsStats> hwAgentPrbsStats;
        auto hwAgentClient =
            utils::createClient<apache::thrift::Client<FbossHwCtrl>>(
                hostInfo, i);
        queryPrbsStats<apache::thrift::Client<FbossHwCtrl>>(
            std::move(hwAgentClient), hwAgentPrbsStats, component);
        for (const auto& [interface, prbsStat] : hwAgentPrbsStats) {
          prbsStats[interface] = prbsStat;
        }
      }
    }
    return prbsStats;
  }

  template <class Client>
  void queryPrbsStats(
      std::unique_ptr<Client> client,
      std::map<std::string, phy::PrbsStats>& prbsStats,
      const phy::PortComponent& component) {
    try {
      client->sync_getAllInterfacePrbsStats(prbsStats, component);
    } catch (const std::exception& e) {
      std::cerr << e.what();
    }
  }

  phy::PrbsStats getPrbsStats(
      const std::map<phy::PortComponent, std::map<std::string, phy::PrbsStats>>&
          allPrbsStats,
      const std::string& interface,
      const phy::PortComponent& component) {
    auto allPrbsStatsItr = allPrbsStats.find(component);
    if (allPrbsStatsItr == allPrbsStats.end()) {
      throw std::runtime_error(
          "Could not find component in PRBS stats: " +
          std::to_string((int)component));
    }
    auto componentPrbsStats = allPrbsStatsItr->second;
    auto componentPrbsStatsItr = componentPrbsStats.find(interface);
    if (componentPrbsStatsItr == componentPrbsStats.end()) {
      throw std::runtime_error(
          "Could not find interface in PRBS stats: " + interface);
    }
    return componentPrbsStatsItr->second;
  }

  Table::StyledCell styledLocked(bool locked) {
    if (locked) {
      return Table::StyledCell("True", Table::Style::GOOD);
    } else {
      return Table::StyledCell("False", Table::Style::ERROR);
    }
  }

  Table::StyledCell styledNumLossOfLock(int numLossOfLock) {
    std::ostringstream outStringStream;
    outStringStream << numLossOfLock;
    if (numLossOfLock > 0) {
      return Table::StyledCell(outStringStream.str(), Table::Style::ERROR);
    } else {
      return Table::StyledCell(outStringStream.str(), Table::Style::GOOD);
    }
  }

 private:
  std::map<int32_t, facebook::fboss::PortInfoThrift> portEntries_;
};

} // namespace facebook::fboss
