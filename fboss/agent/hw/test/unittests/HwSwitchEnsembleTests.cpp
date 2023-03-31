#include <gtest/gtest.h>
#include "fboss/agent/hw/test/HwSwitchEnsemble.h"

namespace facebook::fboss {

// the HwSwitchEnsemble class has lots of abstractions, so we need
// to create a concrete class just to test it
class TestHwSwitchEnsemble : HwSwitchEnsemble {
  std::vector<PortID> masterLogicalPortIds() const override {
    return std::vector<PortID>();
  }
  std::vector<PortID> getAllPortsInGroup(PortID /* portID */) const override {
    return std::vector<PortID>();
  }
  std::vector<FlexPortMode> getSupportedFlexPortModes() const override {
    return std::vector<FlexPortMode>();
  }
  void dumpHwCounters() const override {}
  std::map<PortID, HwPortStats> getLatestPortStats(
      const std::vector<PortID>& /* ports */) override {
    return std::map<PortID, HwPortStats>();
  }
  std::map<AggregatePortID, HwTrunkStats> getLatestAggregatePortStats(
      const std::vector<AggregatePortID>& aggregatePorts) override {
    return {};
  }
  bool isRouteScaleEnabled() const override {
    return true;
  }
  uint64_t getSdkSwitchId() const override {
    return 0;
  }
  bool isSai() const override {
    return false;
  }

  void runDiagCommand(const std::string& /*input*/, std::string& /*output*/)
      override {}

  void init(
      const HwSwitchEnsemble::HwSwitchEnsembleInitInfo& /*info*/) override {}

 public:
  std::unique_ptr<std::thread> setupThrift() override {
    return nullptr;
  }
  explicit TestHwSwitchEnsemble(const HwSwitchEnsemble::Features& features = {})
      : HwSwitchEnsemble(features) {}
};

void call_ctor_dtor() {
  // just trigger the ctor and dtor back-to-back
  // early versions of the dtor assumed that intermediary
  // calls setup state which were not always true
  TestHwSwitchEnsemble foo;
}

TEST(TestHwSwitchEnsemble, Unprogrammed) {
  call_ctor_dtor(); // does the dtor trigger an ASAN error?
}

} // namespace facebook::fboss
