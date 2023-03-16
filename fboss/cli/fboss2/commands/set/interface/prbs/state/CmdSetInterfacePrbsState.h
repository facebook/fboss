// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include <boost/algorithm/string.hpp>
#include <fboss/agent/if/gen-cpp2/ctrl_types.h>
#include <folly/String.h>
#include <folly/gen/Base.h>
#include <cstdint>
#include "fboss/agent/if/gen-cpp2/common_types.h"
#include "fboss/cli/fboss2/CmdHandler.h"
#include "fboss/cli/fboss2/commands/set/interface/prbs/CmdSetInterfacePrbs.h"
#include "fboss/cli/fboss2/utils/Table.h"
#include "fboss/lib/phy/gen-cpp2/phy_types.h"
#include "fboss/lib/phy/gen-cpp2/prbs_types.h"
#include "thrift/lib/cpp/util/EnumUtils.h"

namespace facebook::fboss {

struct CmdSetInterfacePrbsStateTraits : public BaseCommandTraits {
  static constexpr utils::ObjectArgTypeId ObjectArgTypeId =
      utils::ObjectArgTypeId::OBJECT_ARG_TYPE_PRBS_STATE;
  using ParentCmd = CmdSetInterfacePrbs;
  using ObjectArgType = utils::PrbsState;
  using RetType = std::string;
};

class CmdSetInterfacePrbsState : public CmdHandler<
                                     CmdSetInterfacePrbsState,
                                     CmdSetInterfacePrbsStateTraits> {
 public:
  RetType queryClient(
      const HostInfo& hostInfo,
      const utils::PortList& queriedIfs,
      const utils::PrbsComponent& components,
      const ObjectArgType& state) {
    // Setting PRBS state flaps the link, therefore only do it on the
    // requested components and not all if none are passed in the command
    return createModel(hostInfo, queriedIfs, components, state);
  }

  RetType createModel(
      const HostInfo& hostInfo,
      const utils::PortList& queriedIfs,
      const utils::PrbsComponent& components,
      const ObjectArgType& state) {
    for (const auto& intf : queriedIfs) {
      for (const auto& component : components.data()) {
        setPrbsState(hostInfo, intf, component, state);
      }
    }
    return "PRBS State set successfully";
  }

  void setPrbsState(
      const HostInfo& hostInfo,
      const std::string& interfaceName,
      const phy::PortComponent& component,
      const ObjectArgType& state) {
    if (component == phy::PortComponent::TRANSCEIVER_LINE ||
        component == phy::PortComponent::TRANSCEIVER_SYSTEM ||
        component == phy::PortComponent::GB_LINE ||
        component == phy::PortComponent::GB_SYSTEM) {
      auto qsfpClient = utils::createClient<QsfpServiceAsyncClient>(hostInfo);
      prbs::InterfacePrbsState prbsState;
      if (state.enabled) {
        prbsState.polynomial() = state.polynomial;
      }
      if (state.generator.has_value()) {
        prbsState.generatorEnabled() = *state.generator;
      }
      if (state.checker.has_value()) {
        prbsState.checkerEnabled() = *state.checker;
      }
      qsfpClient->sync_setInterfacePrbs(interfaceName, component, prbsState);
    } else if (component == phy::PortComponent::ASIC) {
      // Agent uses the setPortPrbs API currently, so handle it differently
      auto agentClient =
          utils::createClient<facebook::fboss::FbossCtrlAsyncClient>(hostInfo);
      if (portEntries_.empty()) {
        // Fetch all the port info once
        agentClient->sync_getAllPortInfo(portEntries_);
      }
      agentClient->sync_setPortPrbs(
          utils::getPortIDList({interfaceName}, portEntries_)[0],
          component,
          state.enabled,
          static_cast<int>(state.polynomial));
    } else {
      std::runtime_error(
          "Unsupported component " +
          apache::thrift::util::enumNameSafe(component));
    }
  }

  void printOutput(const RetType& model, std::ostream& out = std::cout) {
    out << model << std::endl;
  }

 private:
  std::map<int32_t, facebook::fboss::PortInfoThrift> portEntries_;
};

} // namespace facebook::fboss
