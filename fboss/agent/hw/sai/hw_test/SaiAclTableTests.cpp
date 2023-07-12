// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/agent/hw/test/HwTest.h"

#include "fboss/agent/hw/sai/switch/SaiAclTableManager.h"
#include "fboss/agent/hw/sai/switch/SaiSwitch.h"
#include "fboss/agent/hw/test/ConfigFactory.h"
#include "fboss/agent/hw/test/HwTestAclUtils.h"
#include "fboss/agent/platforms/sai/SaiPlatform.h"

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

TEST_F(SaiAclTableRecreateTests, AclEntryCount) {
  auto setup = [=]() {
    auto config = initialConfig();
    auto* acl1 =
        utility::addAcl(&config, "aclEntry1", cfg::AclActionType::DENY);
    acl1->dscp() = 0x20;
    auto* acl2 =
        utility::addAcl(&config, "aclEntry2", cfg::AclActionType::DENY);
    acl2->dscp() = 0x21;
    applyNewConfig(config);
    const auto* saiSwitch = static_cast<const SaiSwitch*>(getHwSwitch());
    const auto& aclTableManager = saiSwitch->managerTable()->aclTableManager();
    EXPECT_EQ(aclTableManager.aclEntryCount(kAclTable1), 2);
  };
  auto verify = [=]() {
    // ensure acl table exists with same number of acl entries after force
    // recreate.
    const auto* saiSwitch = static_cast<const SaiSwitch*>(getHwSwitch());
    const auto& aclTableManager = saiSwitch->managerTable()->aclTableManager();
    EXPECT_EQ(aclTableManager.aclEntryCount(kAclTable1), 2);
  };
  verifyAcrossWarmBoots(setup, verify);
}

} // namespace facebook::fboss
