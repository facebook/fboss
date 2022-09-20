// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include <fboss/agent/if/gen-cpp2/ctrl_types.h>
#include <folly/String.h>
#include <folly/gen/Base.h>
#include <cstdint>
#include "fboss/agent/if/gen-cpp2/common_types.h"
#include "fboss/cli/fboss2/CmdHandler.h"
#include "fboss/cli/fboss2/commands/show/interface/prbs/CmdShowInterfacePrbs.h"
#include "fboss/cli/fboss2/commands/show/interface/prbs/state/gen-cpp2/model_types.h"
#include "fboss/cli/fboss2/utils/CmdUtils.h"
#include "fboss/cli/fboss2/utils/PrbsUtils.h"
#include "fboss/cli/fboss2/utils/Table.h"
#include "fboss/lib/phy/gen-cpp2/phy_types.h"
#include "thrift/lib/cpp/util/EnumUtils.h"

namespace facebook::fboss {

struct CmdShowInterfacePrbsStateTraits : public BaseCommandTraits {
  using ParentCmd = CmdShowInterfacePrbs;
  static constexpr utils::ObjectArgTypeId ObjectArgTypeId =
      utils::ObjectArgTypeId::OBJECT_ARG_TYPE_ID_NONE;
  using ObjectArgType = std::monostate;
  using RetType = cli::ShowPrbsStateModel;
  static constexpr bool ALLOW_FILTERING = true;
  static constexpr bool ALLOW_AGGREGATION = true;
};

class CmdShowInterfacePrbsState : public CmdHandler<
                                      CmdShowInterfacePrbsState,
                                      CmdShowInterfacePrbsStateTraits> {
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
    Table table;
    table.setHeader(
        {"Interface", "Component", "Generator", "Checker", "Polynomial"});
    for (const auto& intfEntry : *model.interfaceEntries()) {
      for (const auto& entry : *intfEntry.componentEntries()) {
        auto state = *(entry.prbsState());
        bool generator =
            state.generatorEnabled().has_value() && *state.generatorEnabled();
        bool checker =
            state.checkerEnabled().has_value() && *state.checkerEnabled();
        table.addRow(
            {*entry.interfaceName(),
             apache::thrift::util::enumNameSafe(*entry.component()),
             generator ? "Enabled" : "Disabled",
             checker ? "Enabled" : "Disabled",
             apache::thrift::util::enumNameSafe(*state.polynomial())});
      }
    }
    out << table << std::endl;
  }

  RetType createModel(
      const HostInfo& hostInfo,
      const std::vector<std::string>& queriedIfs,
      const std::vector<phy::PortComponent>& components) {
    RetType model;
    for (const auto& intf : queriedIfs) {
      cli::PrbsStateInterfaceEntry intfEntry;
      for (const auto& component : components) {
        cli::PrbsStateComponentEntry entry;
        entry.interfaceName() = intf;
        entry.component() = component;
        entry.prbsState() = getPrbsState(hostInfo, intf, component);
        intfEntry.componentEntries()->push_back(entry);
      }
      if (intfEntry.componentEntries()->size()) {
        model.interfaceEntries()->push_back(intfEntry);
      }
    }
    return model;
  }

  prbs::InterfacePrbsState getPrbsState(
      const HostInfo& hostInfo,
      const std::string& interfaceName,
      const phy::PortComponent& component) {
    prbs::InterfacePrbsState prbsState;
    if (component == phy::PortComponent::TRANSCEIVER_LINE ||
        component == phy::PortComponent::TRANSCEIVER_SYSTEM ||
        component == phy::PortComponent::GB_LINE ||
        component == phy::PortComponent::GB_SYSTEM) {
      auto qsfpClient = utils::createClient<QsfpServiceAsyncClient>(hostInfo);
      qsfpClient->sync_getInterfacePrbsState(
          prbsState, interfaceName, component);
    } else {
      auto agentClient =
          utils::createClient<facebook::fboss::FbossCtrlAsyncClient>(hostInfo);
      agentClient->sync_getInterfacePrbsState(
          prbsState, interfaceName, component);
    }
    return prbsState;
  }
};

} // namespace facebook::fboss
