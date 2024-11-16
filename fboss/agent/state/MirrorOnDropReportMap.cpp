// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include <utility>

#include "fboss/agent/state/MirrorOnDropReport.h"
#include "fboss/agent/state/MirrorOnDropReportMap.h"
#include "fboss/agent/state/SwitchState.h"

namespace facebook::fboss {

std::shared_ptr<MirrorOnDropReportMap> MirrorOnDropReportMap::fromThrift(
    const std::map<std::string, state::MirrorOnDropReportFields>&
        mirrorOnDropReports) {
  auto map = std::make_shared<MirrorOnDropReportMap>();
  for (auto report : mirrorOnDropReports) {
    auto node = std::make_shared<MirrorOnDropReport>();
    node->fromThrift(report.second);
    map->insert(*report.second.name(), std::move(node));
  }
  return map;
}

MultiSwitchMirrorOnDropReportMap* MultiSwitchMirrorOnDropReportMap::modify(
    std::shared_ptr<SwitchState>* state) {
  return SwitchState::modify<switch_state_tags::mirrorOnDropReportMaps>(state);
}

std::shared_ptr<MultiSwitchMirrorOnDropReportMap>
MultiSwitchMirrorOnDropReportMap::fromThrift(
    const std::map<
        std::string,
        std::map<std::string, state::MirrorOnDropReportFields>>&
        mnpuMirrorOnDropReports) {
  auto mnpuMap = std::make_shared<MultiSwitchMirrorOnDropReportMap>();
  for (const auto& reports : mnpuMirrorOnDropReports) {
    auto map = MirrorOnDropReportMap::fromThrift(reports.second);
    mnpuMap->insert(reports.first, std::move(map));
  }
  return mnpuMap;
}

template struct ThriftMapNode<
    MirrorOnDropReportMap,
    MirrorOnDropReportMapTraits>;

} // namespace facebook::fboss
