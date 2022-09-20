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
    return createModel(
        hostInfo,
        queriedIfs,
        prbsComponents(components, true /* returnAllIfEmpty */));
  }

  void printOutput(const RetType& model, std::ostream& out = std::cout) {
    auto now = duration_cast<seconds>(system_clock::now().time_since_epoch());
    for (const auto& intfEntry : *model.interfaceEntries()) {
      for (const auto& entry : *intfEntry.componentEntries()) {
        auto stats = *(entry.prbsStats());
        out << "Interface: " << *entry.interfaceName() << std::endl;
        out << "Component: "
            << apache::thrift::util::enumNameSafe(*entry.component())
            << std::endl;
        out << "Time Since Last Update: "
            << utils::getPrettyElapsedTime(*(stats.timeCollected()))
            << std::endl;
        if (stats.laneStats()->empty()) {
          continue;
        }
        std::vector<std::string> header = {"Field Name"};
        std::unordered_map<int, phy::PrbsLaneStats&> statsPerLane;
        Table table;
        table.setHeader(
            {"Lane",
             "Locked",
             "BER",
             "Max BER",
             "Num Loss Of Lock",
             "Time Since Last Lock",
             "Time Since Last Clear"});
        for (auto laneStat : *stats.laneStats()) {
          std::ostringstream ber, maxBer;
          ber << *laneStat.ber();
          maxBer << *laneStat.maxBer();
          std::string lastClear = *laneStat.timeSinceLastClear()
              ? utils::getPrettyElapsedTime(
                    now.count() - *laneStat.timeSinceLastClear())
              : "N/A";
          table.addRow(
              {std::to_string(*laneStat.laneId()),
               *laneStat.locked() ? "True" : "False",
               ber.str(),
               maxBer.str(),
               std::to_string(*laneStat.numLossOfLock()),
               utils::getPrettyElapsedTime(
                   now.count() - *laneStat.timeSinceLastLocked()),
               lastClear});
        }
        out << table << std::endl;
      }
    }
  }

  RetType createModel(
      const HostInfo& hostInfo,
      const std::vector<std::string>& queriedIfs,
      const std::vector<phy::PortComponent>& components) {
    RetType model;
    for (const auto& intf : queriedIfs) {
      cli::PrbsStatsInterfaceEntry intfEntry;
      for (const auto& component : components) {
        cli::PrbsStatsComponentEntry entry;
        entry.interfaceName() = intf;
        entry.component() = component;
        entry.prbsStats() = getPrbsStats(hostInfo, intf, component);
        intfEntry.componentEntries()->push_back(entry);
      }
      if (intfEntry.componentEntries()->size()) {
        model.interfaceEntries()->push_back(intfEntry);
      }
    }
    return model;
  }

  phy::PrbsStats getPrbsStats(
      const HostInfo& hostInfo,
      const std::string& interfaceName,
      const phy::PortComponent& component) {
    phy::PrbsStats prbsStats;
    try {
      if (component == phy::PortComponent::TRANSCEIVER_LINE ||
          component == phy::PortComponent::TRANSCEIVER_SYSTEM ||
          component == phy::PortComponent::GB_LINE ||
          component == phy::PortComponent::GB_SYSTEM) {
        auto qsfpClient = utils::createClient<QsfpServiceAsyncClient>(hostInfo);
        qsfpClient->sync_getInterfacePrbsStats(
            prbsStats, interfaceName, component);
      } else if (component == phy::PortComponent::ASIC) {
        auto agentClient =
            utils::createClient<facebook::fboss::FbossCtrlAsyncClient>(
                hostInfo);
        if (portEntries_.empty()) {
          // Fetch all the port info once
          agentClient->sync_getAllPortInfo(portEntries_);
        }
        agentClient->sync_getPortPrbsStats(
            prbsStats,
            utils::getPortIDList({interfaceName}, portEntries_)[0],
            component);
      } else {
        std::runtime_error(
            "Unsupported component " +
            apache::thrift::util::enumNameSafe(component));
      }
    } catch (const std::exception& e) {
      std::cerr << e.what();
    }
    return prbsStats;
  }

 private:
  std::map<int32_t, facebook::fboss::PortInfoThrift> portEntries_;
};

} // namespace facebook::fboss
