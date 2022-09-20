// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include <fboss/agent/if/gen-cpp2/ctrl_types.h>
#include <folly/String.h>
#include <folly/gen/Base.h>
#include <cstdint>
#include "fboss/agent/if/gen-cpp2/common_types.h"
#include "fboss/cli/fboss2/CmdHandler.h"
#include "fboss/cli/fboss2/commands/show/interface/prbs/CmdShowInterfacePrbs.h"
#include "fboss/cli/fboss2/commands/show/interface/prbs/capabilities/gen-cpp2/model_types.h"
#include "fboss/cli/fboss2/utils/PrbsUtils.h"
#include "fboss/cli/fboss2/utils/Table.h"
#include "fboss/lib/phy/gen-cpp2/phy_types.h"
#include "fboss/lib/phy/gen-cpp2/prbs_types.h"
#include "thrift/lib/cpp/util/EnumUtils.h"

namespace facebook::fboss {

struct CmdShowInterfacePrbsCapabilitiesTraits : public BaseCommandTraits {
  using ParentCmd = CmdShowInterfacePrbs;
  static constexpr utils::ObjectArgTypeId ObjectArgTypeId =
      utils::ObjectArgTypeId::OBJECT_ARG_TYPE_ID_NONE;
  using ObjectArgType = std::monostate;
  using RetType = cli::ShowCapabilitiesModel;
  static constexpr bool ALLOW_FILTERING = true;
  static constexpr bool ALLOW_AGGREGATION = true;
};

class CmdShowInterfacePrbsCapabilities
    : public CmdHandler<
          CmdShowInterfacePrbsCapabilities,
          CmdShowInterfacePrbsCapabilitiesTraits> {
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
    table.setHeader({"Interface", "Component", "PRBS Polynomials"});
    for (const auto& intfEntry : *model.interfaceEntries()) {
      for (const auto& entry : *intfEntry.componentEntries()) {
        auto polynomialStrings = folly::gen::from(*entry.prbsCapabilties()) |
            folly::gen::mapped([](prbs::PrbsPolynomial p) {
                                   return apache::thrift::util::enumNameSafe(p);
                                 }) |
            folly::gen::as<std::vector>();
        table.addRow(
            {*entry.interfaceName(),
             apache::thrift::util::enumNameSafe(*entry.component()),
             folly::join(",", polynomialStrings)});
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
      cli::InterfaceEntry intfEntry;
      for (const auto& component : components) {
        cli::ComponentEntry entry;
        entry.interfaceName() = intf;
        entry.component() = component;
        entry.prbsCapabilties() = getPrbsPolynomials(hostInfo, intf, component);
        intfEntry.componentEntries()->push_back(entry);
      }
      if (intfEntry.componentEntries()->size()) {
        model.interfaceEntries()->push_back(intfEntry);
      }
    }
    return model;
  }

  std::vector<prbs::PrbsPolynomial> getPrbsPolynomials(
      const HostInfo& hostInfo,
      const std::string& interfaceName,
      const phy::PortComponent& component) {
    std::vector<prbs::PrbsPolynomial> polynomials;
    if (component == phy::PortComponent::TRANSCEIVER_LINE ||
        component == phy::PortComponent::TRANSCEIVER_SYSTEM) {
      auto qsfpClient = utils::createClient<QsfpServiceAsyncClient>(hostInfo);
      qsfpClient->sync_getSupportedPrbsPolynomials(
          polynomials, interfaceName, component);
    } else if (component == phy::PortComponent::ASIC) {
      auto agentClient =
          utils::createClient<facebook::fboss::FbossCtrlAsyncClient>(hostInfo);
      agentClient->sync_getSupportedPrbsPolynomials(
          polynomials, interfaceName, component);
    }
    return polynomials;
  }
};

} // namespace facebook::fboss
