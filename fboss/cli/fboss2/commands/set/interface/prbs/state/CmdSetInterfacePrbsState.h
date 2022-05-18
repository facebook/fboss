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

namespace {
class PrbsState {
 public:
  explicit PrbsState(const std::vector<std::string>& args) {
    auto state = folly::gen::from(args) |
        folly::gen::mapped([](const std::string& s) {
                   return boost::to_upper_copy(s);
                 }) |
        folly::gen::as<std::vector>();
    enabled = (state[0] != "OFF");
    if (enabled) {
      polynomial = apache::thrift::util::enumValueOrThrow<
          facebook::fboss::prbs::PrbsPolynomial>(state[0]);
    }
    if (state.size() <= 1) {
      // None of the generator or checker args are passed, therefore set both
      // the generator and checker
      generator = enabled;
      checker = enabled;
    } else {
      std::unordered_set<std::string> stateSet(state.begin() + 1, state.end());
      if (stateSet.find("GENERATOR") != stateSet.end()) {
        generator = enabled;
      }
      if (stateSet.find("CHECKER") != stateSet.end()) {
        checker = enabled;
      }
    }
  }
  bool enabled;
  facebook::fboss::prbs::PrbsPolynomial polynomial;
  std::optional<bool> generator;
  std::optional<bool> checker;
};
} // namespace

namespace facebook::fboss {

struct CmdSetInterfacePrbsStateTraits : public BaseCommandTraits {
  using ParentCmd = CmdSetInterfacePrbs;
  static constexpr utils::ObjectArgTypeId ObjectArgTypeId =
      utils::ObjectArgTypeId::OBJECT_ARG_TYPE_PRBS_STATE;
  using ObjectArgType = std::vector<std::string>;
  using RetType = std::string;
};

class CmdSetInterfacePrbsState : public CmdHandler<
                                     CmdSetInterfacePrbsState,
                                     CmdSetInterfacePrbsStateTraits> {
 public:
  RetType queryClient(
      const HostInfo& hostInfo,
      const std::vector<std::string>& queriedIfs,
      const std::vector<std::string>& components,
      const ObjectArgType& args) {
    if (args.empty()) {
      throw std::runtime_error(
          "Incomplete command, expecting 'show interface <interface_list> prbs <prbs_component> state <PRBSXX> [generator [checker]]'");
    }
    PrbsState state(args);
    // Setting PRBS state flaps the link, therefore only do it on the
    // requested components and not all if none are passed in the command
    return createModel(
        hostInfo,
        queriedIfs,
        prbsComponents(components, false /* returnAllIfEmpty */),
        state);
  }

  RetType createModel(
      const HostInfo& hostInfo,
      const std::vector<std::string>& queriedIfs,
      const std::vector<phy::PrbsComponent>& components,
      const PrbsState& state) {
    for (const auto& intf : queriedIfs) {
      for (const auto& component : components) {
        setPrbsState(hostInfo, intf, component, state);
      }
    }
    return "PRBS State set successfully";
  }

  void setPrbsState(
      const HostInfo& hostInfo,
      const std::string& interfaceName,
      const phy::PrbsComponent& component,
      const PrbsState& state) {
    if (component == phy::PrbsComponent::TRANSCEIVER_LINE ||
        component == phy::PrbsComponent::TRANSCEIVER_SYSTEM ||
        component == phy::PrbsComponent::GB_LINE ||
        component == phy::PrbsComponent::GB_SYSTEM) {
      auto qsfpClient = utils::createClient<QsfpServiceAsyncClient>(hostInfo);
      prbs::InterfacePrbsState prbsState;
      if (state.enabled) {
        prbsState.polynomial_ref() = state.polynomial;
      }
      if (state.generator.has_value()) {
        prbsState.generatorEnabled_ref() = *state.generator;
      }
      if (state.checker.has_value()) {
        prbsState.checkerEnabled_ref() = *state.checker;
      }
      qsfpClient->sync_setInterfacePrbs(interfaceName, component, prbsState);
    } else if (component == phy::PrbsComponent::ASIC) {
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
