// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/cli/fboss2/commands/clear/interface/prbs/stats/CmdClearInterfacePrbsStats.h"
#include "fboss/cli/fboss2/CmdHandler.cpp"

#include <folly/String.h>
#include <folly/gen/Base.h>

#include "fboss/cli/fboss2/utils/CmdClientUtils.h"
#include "fboss/cli/fboss2/utils/CmdUtils.h"
#include "fboss/cli/fboss2/utils/PrbsUtils.h"
#include "thrift/lib/cpp/util/EnumUtils.h"

namespace facebook::fboss {

CmdClearInterfacePrbsStats::RetType CmdClearInterfacePrbsStats::queryClient(
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

void CmdClearInterfacePrbsStats::printOutput(
    const RetType& model,
    std::ostream& out) {
  out << model << std::endl;
}

CmdClearInterfacePrbsStats::RetType CmdClearInterfacePrbsStats::createModel(
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

void CmdClearInterfacePrbsStats::clearComponentPrbsStats(
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

// Explicit template instantiation
template void
CmdHandler<CmdClearInterfacePrbsStats, CmdClearInterfacePrbsStatsTraits>::run();
template const ValidFilterMapType CmdHandler<
    CmdClearInterfacePrbsStats,
    CmdClearInterfacePrbsStatsTraits>::getValidFilters();

} // namespace facebook::fboss
