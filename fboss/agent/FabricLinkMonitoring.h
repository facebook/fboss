// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once

#include <climits>
#include <map>
#include <optional>
#include <string>
#include <vector>

#include "fboss/agent/gen-cpp2/switch_config_types.h"
#include "fboss/agent/platforms/common/PlatformMapping.h"
#include "fboss/agent/state/Port.h"

namespace facebook::fboss {

class FabricLinkMonitoring {
 public:
  explicit FabricLinkMonitoring(const cfg::SwitchConfig* config);

  // Main public interface
  std::map<PortID, SwitchID> getPort2SwitchIdMapping();

 private:
  // Parallel link variables
  int maxParallelLeafToL1Links_{0};
  int maxParallelL1ToL2Links_{0};
  std::map<SwitchID, std::map<int32_t, std::vector<std::string>>>
      remoteSwitchId2Vd2Ports_;

  // Forward declarations for methods to be added in later diffs
  void processDsfNodes(const cfg::SwitchConfig* config);
  void processLinkInfo(const cfg::SwitchConfig* config);

  // DSF node processing helpers
  void updateSwitchNameMapping(
      const auto& dsfNode,
      const SwitchID& nodeSwitchId);
  void updateLowestSwitchIds(
      const auto& dsfNode,
      const SwitchID& nodeSwitchId,
      const int fabricLevel);

  // Link counting and validation
  void updateLinkCounts(
      const cfg::SwitchConfig* config,
      const SwitchID& neighborSwitchId);
  void validateLinkLimits() const;

  // Virtual device handling
  int32_t getVirtualDeviceIdForLink(
      const cfg::SwitchConfig* config,
      const cfg::Port& port,
      const SwitchID& neighborSwitchId);

  // Parallel link sequencing
  void sequenceParallelLinksToVds(
      const cfg::SwitchConfig* config,
      const std::map<
          SwitchID,
          std::map<int32_t, std::vector<std::pair<std::string, std::string>>>>&
          remoteSwitchId2Vd2PortNamePairs);
  void updateMaxParallelLinks(const cfg::SwitchConfig* config);

  // Basic member variables
  std::map<std::string, SwitchID> switchName2SwitchId_;
  SwitchID lowestLeafSwitchId_{SHRT_MAX};
  SwitchID lowestL1SwitchId_{SHRT_MAX};
  SwitchID lowestL2SwitchId_{SHRT_MAX};
  bool isVoqSwitch_;

  // Link counting variables
  int numLeafToL1Links_{0};
  int numL1ToL2Links_{0};

  // Virtual device variables
  std::map<PortID, int32_t> portId2Vd_;
};

} // namespace facebook::fboss
