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
  // Forward declarations for methods to be added in later diffs
  void processDsfNodes(const cfg::SwitchConfig* config);
  void processLinkInfo(const cfg::SwitchConfig* config);

  // Basic member variables
  std::map<std::string, SwitchID> switchName2SwitchId_;
  SwitchID lowestLeafSwitchId_{SHRT_MAX};
  SwitchID lowestL1SwitchId_{SHRT_MAX};
  SwitchID lowestL2SwitchId_{SHRT_MAX};
  bool isVoqSwitch_;
};

} // namespace facebook::fboss
