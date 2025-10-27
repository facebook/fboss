// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include <fboss/agent/if/gen-cpp2/ctrl_types.h>
#include <folly/String.h>
#include <folly/gen/Base.h>
#include <cstdint>
#include "fboss/agent/if/gen-cpp2/common_types.h"
#include "fboss/cli/fboss2/CmdHandler.h"
#include "fboss/cli/fboss2/commands/clear/interface/prbs/CmdClearInterfacePrbs.h"
#include "fboss/cli/fboss2/utils/CmdClientUtils.h"
#include "fboss/cli/fboss2/utils/CmdUtils.h"
#include "fboss/cli/fboss2/utils/PrbsUtils.h"
#include "fboss/cli/fboss2/utils/Table.h"
#include "fboss/lib/phy/gen-cpp2/phy_types.h"
#include "thrift/lib/cpp/util/EnumUtils.h"

namespace facebook::fboss {

struct CmdClearInterfacePrbsStatsTraits : public WriteCommandTraits {
  using ParentCmd = CmdClearInterfacePrbs;
  static constexpr utils::ObjectArgTypeId ObjectArgTypeId =
      utils::ObjectArgTypeId::OBJECT_ARG_TYPE_ID_NONE;
  using ObjectArgType = std::monostate;
  using RetType = std::string;
};

class CmdClearInterfacePrbsStats : public CmdHandler<
                                       CmdClearInterfacePrbsStats,
                                       CmdClearInterfacePrbsStatsTraits> {
 public:
  RetType queryClient(
      const HostInfo& hostInfo,
      const std::vector<std::string>& queriedIfs,
      const std::vector<std::string>& components) {
    // Since clearing stats could make a write to hardware, don't clear stats on
    // uninteded components. Set returnAllIfEmpty to false in PortComponents()
    return createModel(
        hostInfo,
        queriedIfs,
        prbsComponents(components, false /* returnAllIfEmpty */));
  }

  void printOutput(const RetType& model, std::ostream& out = std::cout) {
    out << model << std::endl;
  }

  RetType createModel(
      const HostInfo& hostInfo,
      const std::vector<std::string>& queriedIfs,
      const std::vector<phy::PortComponent>& components) {
    auto componentsStrings = folly::gen::from(components) |
        folly::gen::mapped([](phy::PortComponent p) {
                               return apache::thrift::util::enumNameSafe(p);
                             }) |
        folly::gen::as<std::vector>();
    for (const auto& component : components) {
      clearComponentPrbsStats(hostInfo, queriedIfs, component);
    }
    return fmt::format(
        "Cleared PRBS stats on interfaces {}, components {}",
        folly::join(",", queriedIfs),
        folly::join(",", componentsStrings));
  }

  void clearComponentPrbsStats(
      const HostInfo& hostInfo,
      const std::vector<std::string>& interfaces,
      const phy::PortComponent& component) {
    if (component == phy::PortComponent::TRANSCEIVER_LINE ||
        component == phy::PortComponent::TRANSCEIVER_SYSTEM ||
        component == phy::PortComponent::GB_LINE ||
        component == phy::PortComponent::GB_SYSTEM) {
      auto qsfpClient =
          utils::createClient<apache::thrift::Client<QsfpService>>(hostInfo);
      queryClearPrbsStats<apache::thrift::Client<QsfpService>>(
          std::move(qsfpClient), interfaces, component);
    } else {
      auto switchIndicesForInterfaces =
          utils::getSwitchIndicesForInterfaces(hostInfo, interfaces);
      for (const auto& [switchIndex, switchInterfaces] :
           switchIndicesForInterfaces) {
        auto hwAgentClient =
            utils::createClient<apache::thrift::Client<FbossHwCtrl>>(
                hostInfo, switchIndex);
        auto switchIndicesForInterfacesItr =
            switchIndicesForInterfaces.find(switchIndex);
        if (switchIndicesForInterfacesItr == switchIndicesForInterfaces.end()) {
          continue;
        }
        queryClearPrbsStats<apache::thrift::Client<FbossHwCtrl>>(
            std::move(hwAgentClient),
            switchIndicesForInterfacesItr->second,
            component);
      }
    }
  }

  template <class Client>
  void queryClearPrbsStats(
      std::unique_ptr<Client> client,
      const std::vector<std::string>& interfaces,
      const phy::PortComponent& component) {
    try {
      client->sync_bulkClearInterfacePrbsStats(interfaces, component);
    } catch (const std::exception& e) {
      std::cerr << e.what();
    }
  }

 private:
  std::map<int32_t, facebook::fboss::PortInfoThrift> portEntries_;
};

} // namespace facebook::fboss
