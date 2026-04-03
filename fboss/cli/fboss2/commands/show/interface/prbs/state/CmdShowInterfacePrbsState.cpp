// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/cli/fboss2/commands/show/interface/prbs/state/CmdShowInterfacePrbsState.h"
#include "fboss/cli/fboss2/CmdHandler.cpp"

#include "fboss/agent/if/gen-cpp2/FbossCtrl.h"
#include "fboss/agent/if/gen-cpp2/FbossHwCtrl.h"
#include "fboss/cli/fboss2/utils/CmdClientUtils.h"
#include "fboss/cli/fboss2/utils/Table.h"
#include "fboss/qsfp_service/if/gen-cpp2/QsfpService.h"

#include "fboss/lib/phy/gen-cpp2/phy_types.h"

#include <fboss/agent/if/gen-cpp2/ctrl_types.h>
#include <cstdint>
#include "fboss/agent/if/gen-cpp2/common_types.h"
#include "thrift/lib/cpp/util/EnumUtils.h"

namespace facebook::fboss {

namespace {

CmdShowInterfacePrbsStateTraits::RetType createModel(
    const HostInfo& hostInfo,
    const std::vector<std::string>& queriedIfs,
    const std::vector<phy::PortComponent>& components,
    const std::map<
        phy::PortComponent,
        std::map<std::string, prbs::InterfacePrbsState>>& allPrbsStates);

prbs::InterfacePrbsState getPrbsState(
    const std::map<
        phy::PortComponent,
        std::map<std::string, prbs::InterfacePrbsState>>& allPrbsStates,
    const std::string& interface,
    const phy::PortComponent& component);

} // namespace

CmdShowInterfacePrbsStateTraits::RetType CmdShowInterfacePrbsState::queryClient(
    const HostInfo& hostInfo,
    const std::vector<std::string>& queriedIfs,
    const std::vector<std::string>& components) {
  std::map<phy::PortComponent, std::map<std::string, prbs::InterfacePrbsState>>
      allPrbsStates;
  auto queriedPrbsComponents =
      prbsComponents(components, true /* returnAllIfEmpty */);
  for (const auto& component : queriedPrbsComponents) {
    auto prbsStates = getComponentPrbsStates(hostInfo, component);
    allPrbsStates[component] = prbsStates;
  }
  if (queriedIfs.empty()) {
    std::map<int32_t, facebook::fboss::PortInfoThrift> portEntries_;
    auto agentClient =
        utils::createClient<apache::thrift::Client<FbossCtrl>>(hostInfo);
    agentClient->sync_getAllPortInfo(portEntries_);
    std::vector<std::string> allIfs;
    allIfs.reserve(portEntries_.size());
    for (const auto& port : portEntries_) {
      allIfs.push_back(port.second.name().value());
    }
    return createModel(hostInfo, allIfs, queriedPrbsComponents, allPrbsStates);
  } else {
    return createModel(
        hostInfo, queriedIfs, queriedPrbsComponents, allPrbsStates);
  }
}

void CmdShowInterfacePrbsState::printOutput(
    const RetType& model,
    std::ostream& out) {
  utils::Table table;
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

std::map<std::string, prbs::InterfacePrbsState>
CmdShowInterfacePrbsState::getComponentPrbsStates(
    const HostInfo& hostInfo,
    const phy::PortComponent& component) {
  std::map<std::string, prbs::InterfacePrbsState> prbsStates;
  if (component == phy::PortComponent::TRANSCEIVER_LINE ||
      component == phy::PortComponent::TRANSCEIVER_SYSTEM ||
      component == phy::PortComponent::GB_LINE ||
      component == phy::PortComponent::GB_SYSTEM) {
    auto qsfpClient =
        utils::createClient<apache::thrift::Client<QsfpService>>(hostInfo);
    queryPrbsStates<apache::thrift::Client<QsfpService>>(
        std::move(qsfpClient), prbsStates, component);
  } else {
    auto numHwSwitches = utils::getNumHwSwitches(hostInfo);
    for (int i = 0; i < numHwSwitches; i++) {
      std::map<std::string, prbs::InterfacePrbsState> hwAgentPrbsStates;
      auto hwAgentClient =
          utils::createClient<apache::thrift::Client<FbossHwCtrl>>(hostInfo, i);
      queryPrbsStates<apache::thrift::Client<FbossHwCtrl>>(
          std::move(hwAgentClient), hwAgentPrbsStates, component);
      for (const auto& [interface, prbsState] : hwAgentPrbsStates) {
        prbsStates[interface] = prbsState;
      }
    }
  }
  return prbsStates;
}

namespace {

CmdShowInterfacePrbsStateTraits::RetType createModel(
    const HostInfo& /* hostInfo */,
    const std::vector<std::string>& queriedIfs,
    const std::vector<phy::PortComponent>& components,
    const std::map<
        phy::PortComponent,
        std::map<std::string, prbs::InterfacePrbsState>>& allPrbsStates) {
  CmdShowInterfacePrbsStateTraits::RetType model;
  for (const auto& intf : queriedIfs) {
    cli::PrbsStateInterfaceEntry intfEntry;
    for (const auto& component : components) {
      cli::PrbsStateComponentEntry entry;
      entry.interfaceName() = intf;
      entry.component() = component;
      entry.prbsState() = getPrbsState(allPrbsStates, intf, component);
      intfEntry.componentEntries()->push_back(entry);
    }
    if (intfEntry.componentEntries()->size()) {
      model.interfaceEntries()->push_back(intfEntry);
    }
  }
  return model;
}

prbs::InterfacePrbsState getPrbsState(
    const std::map<
        phy::PortComponent,
        std::map<std::string, prbs::InterfacePrbsState>>& allPrbsStates,
    const std::string& interface,
    const phy::PortComponent& component) {
  auto allPrbsStatesItr = allPrbsStates.find(component);
  if (allPrbsStatesItr == allPrbsStates.end()) {
    throw std::runtime_error(
        "Could not find component in PRBS states: " +
        std::to_string((int)component));
  }
  auto componentPrbsStates = allPrbsStatesItr->second;
  auto componentPrbsStatesItr = componentPrbsStates.find(interface);
  if (componentPrbsStatesItr == componentPrbsStates.end()) {
    throw std::runtime_error(
        "Could not find interface in PRBS states: " + interface);
  }
  return componentPrbsStatesItr->second;
}

} // namespace

// Template instantiations
template void
CmdHandler<CmdShowInterfacePrbsState, CmdShowInterfacePrbsStateTraits>::run();
template const ValidFilterMapType CmdHandler<
    CmdShowInterfacePrbsState,
    CmdShowInterfacePrbsStateTraits>::getValidFilters();

} // namespace facebook::fboss
