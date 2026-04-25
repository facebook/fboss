// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/agent/hw/test/HwTest.h"

#include "fboss/agent/hw/sai/switch/SaiAclTableManager.h"
#include "fboss/agent/hw/sai/switch/SaiSwitch.h"
#include "fboss/agent/hw/test/ConfigFactory.h"

namespace facebook::fboss {

class SaiAclTableRecreateTests : public HwTest {
 public:
  void SetUp() override {
    // enforce acl table is re-created
    FLAGS_force_recreate_acl_tables = true;
    HwTest::SetUp();
  }
  cfg::SwitchConfig initialConfig() const {
    return utility::onePortPerInterfaceConfig(
        getHwSwitch(),
        masterLogicalPortIds(),
        getAsic()->desiredLoopbackModes());
  }
};

} // namespace facebook::fboss
