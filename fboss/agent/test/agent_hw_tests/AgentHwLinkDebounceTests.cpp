// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/agent/state/Port.h"
#include "fboss/agent/state/PortMap.h"
#include "fboss/agent/state/SwitchState.h"
#include "fboss/agent/test/AgentEnsemble.h"
#include "fboss/agent/test/AgentHwTest.h"
#include "fboss/agent/test/utils/ConfigUtils.h"
#include "fboss/lib/CommonUtils.h"

#include <fb303/ServiceData.h>
#include <folly/logging/xlog.h>

#include <chrono>

namespace facebook::fboss {

class AgentHwLinkDebounceTest : public AgentHwTest {
 public:
  std::vector<ProductionFeature> getProductionFeaturesVerified()
      const override {
    return {ProductionFeature::PORT_DEBOUNCE};
  }

 protected:
  cfg::SwitchConfig initialConfig(
      const AgentEnsemble& ensemble) const override {
    return utility::onePortPerInterfaceConfig(
        ensemble.getSw(),
        ensemble.masterLogicalPortIds(),
        true /*interfaceHasSubnet*/);
  }

  PortID portForTest() const {
    return getAgentEnsemble()->masterLogicalInterfacePortIds(
        getCurrentSwitchIdForTesting())[0];
  }

  // Apply a config that overrides the debounce values for the test port.
  // Either argument may be std::nullopt to leave the SDK default in place.
  void applyDebounceConfig(
      std::optional<int32_t> upMs,
      std::optional<int32_t> downMs) {
    auto config = initialConfig(*getAgentEnsemble());
    auto portCfg = utility::findCfgPort(config, portForTest());
    if (upMs.has_value()) {
      portCfg->portUpHoldoffTimeMs() = *upMs;
    } else {
      portCfg->portUpHoldoffTimeMs().reset();
    }
    if (downMs.has_value()) {
      portCfg->portDownHoldoffTimeMs() = *downMs;
    } else {
      portCfg->portDownHoldoffTimeMs().reset();
    }
    applyNewConfig(config);
  }

  // Time how long it takes for the test port to reach `up` after triggering
  // a flap via bringUpPort/bringDownPort. Both calls block until the
  // corresponding state change is observed, so the elapsed wall-clock time
  // reflects the SDK debounce hold timer + baseline FBOSS overhead.
  std::chrono::milliseconds measureBringUpDownLatency(bool up) {
    auto port = portForTest();
    auto start = std::chrono::steady_clock::now();
    if (up) {
      bringUpPort(port);
    } else {
      bringDownPort(port);
    }
    auto end = std::chrono::steady_clock::now();
    return std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
  }

  int64_t getLinkStateFlapCount(PortID port) const {
    auto name = getProgrammedState()->getPorts()->getNodeIf(port)->getName();
    return fb303::fbData->getCounterIfExists(name + ".link_state.flap.sum")
        .value_or(0);
  }
};

} // namespace facebook::fboss
