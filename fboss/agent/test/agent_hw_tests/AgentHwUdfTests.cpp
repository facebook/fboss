/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/test/AgentHwTest.h"
#include "fboss/agent/test/utils/AclTestUtils.h"
#include "fboss/agent/test/utils/ConfigUtils.h"
#include "fboss/agent/test/utils/UdfTestUtils.h"

namespace facebook::fboss {

class AgentHwUdfTest : public AgentHwTest {
 protected:
  std::vector<production_features::ProductionFeature>
  getProductionFeaturesVerified() const override {
    return {production_features::ProductionFeature::UDF_WR_IMMEDIATE_ACL};
  }

  cfg::UdfConfig setupUdfConfiguration(
      bool addConfig,
      bool udfHashEnabled = true,
      bool udfAclEnabled = false) {
    AgentEnsemble* ensemble = getAgentEnsemble();
    const auto port = masterLogicalPortIds()[0];
    auto switchId = scopeResolver().scope(port).switchId();
    auto client = ensemble->getHwAgentTestClient(switchId);
    auto asic = getSw()->getHwAsicTable()->getHwAsic(switchId);

    cfg::UdfConfig udfConfig;
    if (addConfig) {
      if (udfHashEnabled && udfAclEnabled) {
        udfConfig = utility::addUdfHashAclConfig(asic->getAsicType());
      } else if (udfHashEnabled) {
        udfConfig = utility::addUdfHashConfig(asic->getAsicType());
      } else {
        udfConfig = utility::addUdfAclConfig();
      }
    }
    return udfConfig;
  }

  cfg::SwitchConfig addUdfAclRoceOpcodeConfig(cfg::SwitchConfig& cfg) {
    cfg::SwitchConfig* switchCfg = &cfg;

    switchCfg->udfConfig() = utility::addUdfAclConfig();
    // Add ACL configuration
    auto aclEntry = cfg::AclEntry();
    *aclEntry.name() = utility::kUdfAclRoceOpcodeName;
    *aclEntry.actionType() = cfg::AclActionType::PERMIT;

    auto acl = utility::addAcl(switchCfg, aclEntry, cfg::AclStage::INGRESS);

    acl->udfGroups() = {utility::kUdfAclRoceOpcodeGroupName};
    acl->roceOpcode() = utility::kUdfRoceOpcodeAck;

    // Add AclStat configuration
    utility::addAclStat(
        switchCfg,
        utility::kUdfAclRoceOpcodeName,
        utility::kUdfAclRoceOpcodeStats);
    return *switchCfg;
  }
};

TEST_F(AgentHwUdfTest, checkUdfHashConfiguration) {
  AgentEnsemble* ensemble = getAgentEnsemble();
  auto setup = [=, this]() {
    auto newCfg{initialConfig(*ensemble)};
    newCfg.udfConfig() = setupUdfConfiguration(true, true);
    applyNewConfig(newCfg);
  };
  auto verify = [=, this]() {
    const auto port = masterLogicalPortIds()[0];
    auto switchId = scopeResolver().scope(port).switchId();
    auto client = getAgentEnsemble()->getHwAgentTestClient(switchId);
    WITH_RETRIES({
      EXPECT_EVENTUALLY_TRUE(client->sync_validateUdfConfig(
          utility::kUdfHashDstQueuePairGroupName,
          utility::kUdfL4UdpRocePktMatcherName));
    });
  };

  verifyAcrossWarmBoots(setup, verify);
}

TEST_F(AgentHwUdfTest, checkUdfAclConfiguration) {
  AgentEnsemble* ensemble = getAgentEnsemble();

  auto setup = [=, this]() {
    auto newCfg{initialConfig(*ensemble)};
    newCfg.udfConfig() = setupUdfConfiguration(true, false);
    applyNewConfig(newCfg);
  };
  auto verify = [=, this]() {
    const auto port = masterLogicalPortIds()[0];
    auto switchId = scopeResolver().scope(port).switchId();
    auto client = getAgentEnsemble()->getHwAgentTestClient(switchId);
    WITH_RETRIES({
      EXPECT_EVENTUALLY_TRUE(client->sync_validateUdfConfig(
          utility::kUdfAclRoceOpcodeGroupName,
          utility::kUdfL4UdpRocePktMatcherName));
    });
  };

  verifyAcrossWarmBoots(setup, verify);
}

TEST_F(AgentHwUdfTest, deleteUdfHashConfig) {
  // Add UdfGroup and PacketMatcher configuration for UDF Hash
  AgentEnsemble* ensemble = getAgentEnsemble();

  auto newCfg{initialConfig(*ensemble)};
  newCfg.udfConfig() = setupUdfConfiguration(true);
  applyNewConfig(newCfg);

  // Get UdfGroup and PacketMatcher Ids for verify
  const auto port = masterLogicalPortIds()[0];
  auto switchId = scopeResolver().scope(port).switchId();
  auto client = getAgentEnsemble()->getHwAgentTestClient(switchId);

  int udfGroupId =
      client->sync_getHwUdfGroupId(utility::kUdfHashDstQueuePairGroupName);

  int udfPacketMatcherId = client->sync_getHwUdfPacketMatcherId(
      utility::kUdfL4UdpRocePktMatcherName);

  // Remove UdfGroup and PacketMatcher configuration for UDF Hash
  newCfg.udfConfig() = setupUdfConfiguration(false);
  applyNewConfig(newCfg);

  auto verify = [&]() {
    // Verify that UdfGroup and PacketMatcher are deleted
    WITH_RETRIES({
      EXPECT_EVENTUALLY_TRUE(client->sync_validateRemoveUdfGroup(
          utility::kUdfHashDstQueuePairGroupName, udfGroupId));
    });

    WITH_RETRIES({
      EXPECT_EVENTUALLY_TRUE(client->sync_validateRemoveUdfPacketMatcher(
          utility::kUdfL4UdpRocePktMatcherName, udfPacketMatcherId));
    });
  };
  verifyAcrossWarmBoots([] {}, verify);
}

// This test is to verify that UdfGroup(roceOpcode) for UdfAcl and associated
// PacketMatcher can be successfully deleted.
TEST_F(AgentHwUdfTest, deleteUdfAclConfig) {
  AgentEnsemble* ensemble = getAgentEnsemble();
  const auto port = masterLogicalPortIds()[0];
  auto switchId = scopeResolver().scope(port).switchId();
  auto client = getAgentEnsemble()->getHwAgentTestClient(switchId);

  auto newCfg{initialConfig(*ensemble)};
  // Add UdfGroup and PacketMatcher configuration for UDF ACL
  newCfg.udfConfig() = utility::addUdfAclConfig();

  // Add ACL configuration
  auto aclEntry = cfg::AclEntry();
  *aclEntry.name() = "test-udf-acl";
  *aclEntry.actionType() = cfg::AclActionType::PERMIT;

  auto acl = utility::addAcl(&newCfg, aclEntry, cfg::AclStage::INGRESS);
  acl->udfGroups() = {utility::kUdfAclRoceOpcodeGroupName};
  acl->roceMask() = {utility::kUdfRoceOpcodeAck};
  acl->roceBytes() = {utility::kUdfRoceOpcodeAck};
  applyNewConfig(newCfg);

  // Get UdfGroup and PacketMatcher Ids for verify
  int udfGroupId =
      client->sync_getHwUdfGroupId(utility::kUdfAclRoceOpcodeGroupName);

  int udfPacketMatcherId = client->sync_getHwUdfPacketMatcherId(
      utility::kUdfL4UdpRocePktMatcherName);

  // Delete UdfGroup and PacketMatcher configuration for UDF ACL
  auto resetCfg{initialConfig(*ensemble)};
  applyNewConfig(resetCfg);

  auto verify = [&, this]() {
    // Verify that UdfGroup and PacketMatcher are deleted
    WITH_RETRIES({
      EXPECT_EVENTUALLY_TRUE(client->sync_validateRemoveUdfGroup(
          utility::kUdfAclRoceOpcodeGroupName, udfGroupId));
    });

    WITH_RETRIES({
      EXPECT_EVENTUALLY_TRUE(client->sync_validateRemoveUdfPacketMatcher(
          utility::kUdfL4UdpRocePktMatcherName, udfPacketMatcherId));
    });

    WITH_RETRIES({
      EXPECT_EVENTUALLY_TRUE(client->sync_validateUdfIdsInQset(
          getSw()
              ->getHwAsicTable()
              ->getHwAsic(switchId)
              ->getDefaultACLGroupID(),
          false));
    });
  };
  verifyAcrossWarmBoots([] {}, verify);
}

TEST_F(AgentHwUdfTest, addAclConfig) {
  AgentEnsemble* ensemble = getAgentEnsemble();
  const auto port = masterLogicalPortIds()[0];
  auto switchId = scopeResolver().scope(port).switchId();
  auto client = ensemble->getHwAgentTestClient(switchId);
  auto asic = getSw()->getHwAsicTable()->getHwAsic(switchId);

  if (asic->getAsicType() == cfg::AsicType::ASIC_TYPE_FAKE) {
    GTEST_SKIP();
  }
  auto setup = [&]() {
    auto cfg{initialConfig(*ensemble)};
    if (!ensemble->isSai()) {
      addUdfAclRoceOpcodeConfig(cfg);
    }
    applyNewConfig(cfg);
  };

  auto verify = [&, this]() {
    WITH_RETRIES({
      EXPECT_EVENTUALLY_TRUE(client->sync_validateUdfAclRoceOpcodeConfig(
          getProgrammedState()->toThrift()));
    });
  };

  verifyAcrossWarmBoots(setup, verify);
}

TEST_F(AgentHwUdfTest, checkUdfHashAclConfiguration) {
  const auto port = masterLogicalPortIds()[0];
  auto switchId = scopeResolver().scope(port).switchId();
  auto client = getAgentEnsemble()->getHwAgentTestClient(switchId);
  AgentEnsemble* ensemble = getAgentEnsemble();

  auto setup = [=, this]() {
    // Add Udf configuration for both hash and acl, first parameter is
    // addConfig and second udfHashEnabaled and third is udfAclEnabled

    auto newCfg{initialConfig(*ensemble)};
    newCfg.udfConfig() = setupUdfConfiguration(true, true, true);
    applyNewConfig(newCfg);
  };
  auto verify = [&]() {
    // Verify that UdfHash configuration is added
    WITH_RETRIES({
      EXPECT_EVENTUALLY_TRUE(client->sync_validateUdfConfig(
          utility::kUdfHashDstQueuePairGroupName,
          utility::kUdfL4UdpRocePktMatcherName));
    });

    // Verify that UdfAcl configuration is added
    WITH_RETRIES({
      EXPECT_EVENTUALLY_TRUE(client->sync_validateUdfConfig(
          utility::kUdfAclRoceOpcodeGroupName,
          utility::kUdfL4UdpRocePktMatcherName));
    });
  };

  verifyAcrossWarmBoots(setup, verify);
}

} // namespace facebook::fboss
