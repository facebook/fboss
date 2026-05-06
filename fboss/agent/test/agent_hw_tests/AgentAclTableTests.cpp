// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include <gflags/gflags.h>

#include "fboss/agent/test/AgentHwTest.h"

#include "fboss/agent/test/utils/AclTestUtils.h"
#include "fboss/agent/test/utils/ConfigUtils.h"

namespace facebook::fboss {

class AgentAclTableRecreateTest : public AgentHwTest {
 public:
  std::vector<ProductionFeature> getProductionFeaturesVerified()
      const override {
    return {
        ProductionFeature::SINGLE_ACL_TABLE, ProductionFeature::L3_FORWARDING};
  }

  void setCmdLineFlagOverrides() const override {
    AgentHwTest::setCmdLineFlagOverrides();
    gflags::SetCommandLineOptionWithMode(
        "force_recreate_acl_tables", "true", gflags::SET_FLAG_IF_DEFAULT);
  }
};

TEST_F(AgentAclTableRecreateTest, AclEntryCount) {
  auto setup = [=, this]() {
    auto& ensemble = *getAgentEnsemble();
    auto config = initialConfig(ensemble);

    auto switchId = scopeResolver().scope(masterLogicalPortIds()[0]).switchId();

    cfg::AclEntry acl1;
    acl1.name() = "aclEntry1";
    acl1.actionType() = cfg::AclActionType::DENY;
    acl1.dscp() = 0x20;
    utility::addEtherTypeToAcl(
        hwAsicForSwitch(switchId), &acl1, cfg::EtherType::IPv6);
    utility::addAcl(&config, acl1, cfg::AclStage::INGRESS);

    cfg::AclEntry acl2;
    acl2.name() = "aclEntry2";
    acl2.actionType() = cfg::AclActionType::DENY;
    acl2.dscp() = 0x21;
    utility::addEtherTypeToAcl(
        hwAsicForSwitch(switchId), &acl2, cfg::EtherType::IPv6);
    utility::addAcl(&config, acl2, cfg::AclStage::INGRESS);

    applyNewConfig(config);
  };

  auto verify = [=, this]() {
    auto& ensemble = *getAgentEnsemble();
    auto switchId = scopeResolver().scope(masterLogicalPortIds()[0]).switchId();
    auto client = ensemble.getHwAgentTestClient(switchId);

    // Ensure ACL table exists with 2 ACL entries after force recreate
    auto numEntries = client->sync_getDefaultAclTableNumAclEntries();
    EXPECT_EQ(numEntries, 2);
  };

  verifyAcrossWarmBoots(setup, verify);
}

} // namespace facebook::fboss
