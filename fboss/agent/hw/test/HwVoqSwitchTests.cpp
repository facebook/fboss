// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/agent/hw/test/HwTest.h"

namespace facebook::fboss {

class HwVoqSwitchTest : public HwTest {
 private:
  std::pair<std::optional<SwitchID>, cfg::SwitchType> getSwitchIdAndType()
      const override {
    return std::make_pair(SwitchID(0), cfg::SwitchType::VOQ);
  }
};

TEST_F(HwVoqSwitchTest, init) {
  verifyAcrossWarmBoots([]() {}, []() {});
}

} // namespace facebook::fboss
