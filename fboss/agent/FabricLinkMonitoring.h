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
  const std::map<PortID, SwitchID>& getPort2LinkSwitchIdMapping() const;
  SwitchID getSwitchIdForPort(const PortID& portId) const;

  // Test-only methods to enable/check test mode
  static void setTestMode(bool enabled);
  static bool isTestMode();

 private:
  // Forward declarations for methods to be added in later diffs
  // Compute switch ID offsets
  int getSwitchIdOffset(
      const SwitchID& localSwitchId,
      const SwitchID& remoteSwitchId);

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
      const SwitchID& neighborSwitchId,
      const int vd);
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
  // Parallel link offset computation
  int calculateParallelLinkOffset(
      const cfg::Port& port,
      SwitchID remoteSwitchId,
      int vd,
      int parallelLinks);

  // Allocate a unique switch ID for each of the links in a VD
  void allocateSwitchIdForPorts(const cfg::SwitchConfig* config);

  // Basic member variables
  std::map<std::string, SwitchID> switchName2SwitchId_;
  SwitchID lowestLeafSwitchId_{SHRT_MAX};
  SwitchID lowestL1SwitchId_{SHRT_MAX};
  SwitchID lowestL2SwitchId_{SHRT_MAX};
  bool isVoqSwitch_;

  // Link counting variables per VD
  std::map<int, int> numLeafL1Links_;
  std::map<int, int> numL1L2Links_;

  // Virtual device variables
  std::map<PortID, int32_t> portId2Vd_;

  // Parallel link variables
  int maxParallelLeafToL1Links_{0};
  int maxParallelL1ToL2Links_{0};
  std::map<SwitchID, std::map<int32_t, std::vector<std::string>>>
      remoteSwitchId2Vd2Ports_;

  // Unique fabric link monitoring switch ID per port
  std::map<PortID, SwitchID> portId2LinkSwitchId_;
};

} // namespace facebook::fboss
