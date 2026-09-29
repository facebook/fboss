// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/agent/test/agent_hw_tests/AgentAccessPolicyAclTestBase.h"

#include <algorithm>
#include <string>

namespace facebook::fboss {

namespace {
std::string aclEntryTestName(
    const ::testing::TestParamInfo<std::string>& info) {
  auto name = info.param;
  std::replace(name.begin(), name.end(), '-', '_');
  return name;
}
} // namespace

// AclTable1 holds the CPU policing entries, which the forwarding tests must
// not see.
template <typename BaseT>
class AgentAccessPolicyControlPlaneTest : public BaseT {
 protected:
  cfg::SwitchConfig initialConfig(
      const AgentEnsemble& ensemble) const override {
    return this->addCoppConfig(ensemble, BaseT::initialConfig(ensemble));
  }

  std::vector<ProductionFeature> getProductionFeaturesVerified()
      const override {
    auto features = BaseT::getProductionFeaturesVerified();
    features.push_back(ProductionFeature::COPP);
    return features;
  }
};

template <typename BaseT>
class AgentAccessPolicyAclAddedTest : public BaseT {
 protected:
  bool coldBootWithAccessPolicy() const override {
    return false;
  }
};

template <typename BaseT>
class AgentAccessPolicyAclUpgradedTest : public BaseT {
 protected:
  utility::AccessPolicyVersion warmBootRuleSet() const override {
    return utility::AccessPolicyVersion::V1;
  }
};

template <typename BaseT>
class AgentAccessPolicyAclDowngradedTest : public BaseT {
 protected:
  utility::AccessPolicyVersion coldBootRuleSet() const override {
    return utility::AccessPolicyVersion::V1;
  }
};

template <typename BaseT>
class AgentAccessPolicyAclV2UpgradedTest : public BaseT {
 protected:
  utility::AccessPolicyVersion warmBootRuleSet() const override {
    return utility::AccessPolicyVersion::V2;
  }
};

template <typename BaseT>
class AgentAccessPolicyAclV1ToV2UpgradedTest : public BaseT {
 protected:
  utility::AccessPolicyVersion coldBootRuleSet() const override {
    return utility::AccessPolicyVersion::V1;
  }
  utility::AccessPolicyVersion warmBootRuleSet() const override {
    return utility::AccessPolicyVersion::V2;
  }
};

template <typename BaseT>
class AgentAccessPolicyAclRemovedTest : public BaseT {
 protected:
  bool warmBootWithAccessPolicy() const override {
    return false;
  }
};

template <typename BaseT>
class AgentAccessPolicyAclEntryAddedTest
    : public BaseT,
      public ::testing::WithParamInterface<std::string> {
 protected:
  std::set<std::string> coldBootOmitRules() const override {
    return {this->GetParam()};
  }
};

template <typename BaseT>
class AgentAccessPolicyAclEntryDeletedTest
    : public BaseT,
      public ::testing::WithParamInterface<std::string> {
 protected:
  std::set<std::string> warmBootOmitRules() const override {
    return {this->GetParam()};
  }
};

using AgentAccessPolicyClassIdAclAddedTest =
    AgentAccessPolicyAclAddedTest<AgentAccessPolicyClassIdAclTest>;
using AgentAccessPolicyPortBoundAclAddedTest =
    AgentAccessPolicyAclAddedTest<AgentAccessPolicyPortBoundAclTest>;
using AgentAccessPolicyClassIdAclUpgradedTest =
    AgentAccessPolicyAclUpgradedTest<AgentAccessPolicyClassIdAclTest>;
using AgentAccessPolicyPortBoundAclUpgradedTest =
    AgentAccessPolicyAclUpgradedTest<AgentAccessPolicyPortBoundAclTest>;
using AgentAccessPolicyClassIdAclV1AddedTest = AgentAccessPolicyAclAddedTest<
    AgentAccessPolicyAclUpgradedTest<AgentAccessPolicyClassIdAclTest>>;
using AgentAccessPolicyPortBoundAclV1AddedTest = AgentAccessPolicyAclAddedTest<
    AgentAccessPolicyAclUpgradedTest<AgentAccessPolicyPortBoundAclTest>>;
using AgentAccessPolicyClassIdAclDowngradedTest =
    AgentAccessPolicyAclDowngradedTest<AgentAccessPolicyClassIdAclTest>;
using AgentAccessPolicyPortBoundAclDowngradedTest =
    AgentAccessPolicyAclDowngradedTest<AgentAccessPolicyPortBoundAclTest>;
using AgentAccessPolicyClassIdAclV1ToV2UpgradedTest =
    AgentAccessPolicyAclV1ToV2UpgradedTest<AgentAccessPolicyClassIdAclTest>;
using AgentAccessPolicyPortBoundAclV1ToV2UpgradedTest =
    AgentAccessPolicyAclV1ToV2UpgradedTest<AgentAccessPolicyPortBoundAclTest>;
using AgentAccessPolicyClassIdAclV2AddedTest = AgentAccessPolicyAclAddedTest<
    AgentAccessPolicyAclV2UpgradedTest<AgentAccessPolicyClassIdAclTest>>;
using AgentAccessPolicyPortBoundAclV2AddedTest = AgentAccessPolicyAclAddedTest<
    AgentAccessPolicyAclV2UpgradedTest<AgentAccessPolicyPortBoundAclTest>>;
using AgentAccessPolicyClassIdAclRemovedTest =
    AgentAccessPolicyAclRemovedTest<AgentAccessPolicyClassIdAclTest>;
using AgentAccessPolicyPortBoundAclRemovedTest =
    AgentAccessPolicyAclRemovedTest<AgentAccessPolicyPortBoundAclTest>;
using AgentAccessPolicyClassIdAclEntryAddedTest =
    AgentAccessPolicyAclEntryAddedTest<AgentAccessPolicyClassIdAclTest>;
using AgentAccessPolicyPortBoundAclEntryAddedTest =
    AgentAccessPolicyAclEntryAddedTest<AgentAccessPolicyPortBoundAclTest>;
using AgentAccessPolicyClassIdControlPlaneTest =
    AgentAccessPolicyControlPlaneTest<AgentAccessPolicyClassIdAclTest>;
using AgentAccessPolicyPortBoundControlPlaneTest =
    AgentAccessPolicyControlPlaneTest<AgentAccessPolicyPortBoundAclTest>;
// Only Broadcom implements SAI_PACKET_ACTION_DENY; on Leaba the ACL entry fails
// to create, which is fatal to the agent. The class id shape is gated to
// tomahawk3 by ACCESS_POLICY_CLASS_ID_ACL, so this cannot reach a Leaba ASIC.
class AgentAccessPolicyClassIdControlPlaneStrongDenyTest
    : public AgentAccessPolicyClassIdControlPlaneTest {
 protected:
  cfg::AclActionType denyActionType(
      const std::vector<const HwAsic*>& /*asics*/) const override {
    return cfg::AclActionType::DENY_DATA_AND_CONTROL_PLANE;
  }
};
using AgentAccessPolicyClassIdAclEntryDeletedTest =
    AgentAccessPolicyAclEntryDeletedTest<AgentAccessPolicyClassIdAclTest>;
using AgentAccessPolicyPortBoundAclEntryDeletedTest =
    AgentAccessPolicyAclEntryDeletedTest<AgentAccessPolicyPortBoundAclTest>;

TEST_F(AgentAccessPolicyClassIdAclTest, AccessPolicyAcl) {
  runAccessPolicyTest();
}

TEST_F(AgentAccessPolicyPortBoundAclTest, AccessPolicyAcl) {
  runAccessPolicyTest();
}

TEST_F(AgentAccessPolicyClassIdControlPlaneTest, ControlPlanePunt) {
  runControlPlaneTest();
}

TEST_F(AgentAccessPolicyPortBoundControlPlaneTest, ControlPlanePunt) {
  runControlPlaneTest();
}

TEST_F(AgentAccessPolicyClassIdControlPlaneStrongDenyTest, ControlPlanePunt) {
  runControlPlaneTest();
}

TEST_F(AgentAccessPolicyClassIdAclAddedTest, AccessPolicyAclAddedOnWarmboot) {
  runAccessPolicyTest();
}

TEST_F(AgentAccessPolicyPortBoundAclAddedTest, AccessPolicyAclAddedOnWarmboot) {
  runAccessPolicyTest();
}

TEST_F(
    AgentAccessPolicyClassIdAclUpgradedTest,
    AccessPolicyAclUpgradedOnWarmboot) {
  runAccessPolicyTest();
}

TEST_F(
    AgentAccessPolicyPortBoundAclUpgradedTest,
    AccessPolicyAclUpgradedOnWarmboot) {
  runAccessPolicyTest();
}

TEST_F(
    AgentAccessPolicyClassIdAclV1AddedTest,
    AccessPolicyAclV1AddedOnWarmboot) {
  runAccessPolicyTest();
}

TEST_F(
    AgentAccessPolicyPortBoundAclV1AddedTest,
    AccessPolicyAclV1AddedOnWarmboot) {
  runAccessPolicyTest();
}

TEST_F(
    AgentAccessPolicyClassIdAclDowngradedTest,
    AccessPolicyAclDowngradedOnWarmboot) {
  runAccessPolicyTest();
}

TEST_F(
    AgentAccessPolicyPortBoundAclDowngradedTest,
    AccessPolicyAclDowngradedOnWarmboot) {
  runAccessPolicyTest();
}

TEST_F(
    AgentAccessPolicyClassIdAclV1ToV2UpgradedTest,
    AccessPolicyAclV1ToV2UpgradedOnWarmboot) {
  runAccessPolicyTest();
}

TEST_F(
    AgentAccessPolicyPortBoundAclV1ToV2UpgradedTest,
    AccessPolicyAclV1ToV2UpgradedOnWarmboot) {
  runAccessPolicyTest();
}

TEST_F(
    AgentAccessPolicyClassIdAclV2AddedTest,
    AccessPolicyAclV2AddedOnWarmboot) {
  runAccessPolicyTest();
}

TEST_F(
    AgentAccessPolicyPortBoundAclV2AddedTest,
    AccessPolicyAclV2AddedOnWarmboot) {
  runAccessPolicyTest();
}

TEST_F(
    AgentAccessPolicyClassIdAclRemovedTest,
    AccessPolicyAclRemovedOnWarmboot) {
  runAccessPolicyTest();
}

TEST_F(
    AgentAccessPolicyPortBoundAclRemovedTest,
    AccessPolicyAclRemovedOnWarmboot) {
  runAccessPolicyTest();
}

TEST_P(
    AgentAccessPolicyClassIdAclEntryAddedTest,
    AccessPolicyAclEntryAddedOnWarmboot) {
  runAccessPolicyTest();
}

TEST_P(
    AgentAccessPolicyPortBoundAclEntryAddedTest,
    AccessPolicyAclEntryAddedOnWarmboot) {
  runAccessPolicyTest();
}

TEST_P(
    AgentAccessPolicyClassIdAclEntryDeletedTest,
    AccessPolicyAclEntryDeletedOnWarmboot) {
  runAccessPolicyTest();
}

TEST_P(
    AgentAccessPolicyPortBoundAclEntryDeletedTest,
    AccessPolicyAclEntryDeletedOnWarmboot) {
  runAccessPolicyTest();
}

INSTANTIATE_TEST_SUITE_P(
    AccessPolicy,
    AgentAccessPolicyClassIdAclEntryAddedTest,
    ::testing::ValuesIn(utility::accessPolicyRepresentativeRules()),
    aclEntryTestName);

INSTANTIATE_TEST_SUITE_P(
    AccessPolicy,
    AgentAccessPolicyPortBoundAclEntryAddedTest,
    ::testing::ValuesIn(utility::accessPolicyRepresentativeRules()),
    aclEntryTestName);

INSTANTIATE_TEST_SUITE_P(
    AccessPolicy,
    AgentAccessPolicyClassIdAclEntryDeletedTest,
    ::testing::ValuesIn(utility::accessPolicyRepresentativeRules()),
    aclEntryTestName);

INSTANTIATE_TEST_SUITE_P(
    AccessPolicy,
    AgentAccessPolicyPortBoundAclEntryDeletedTest,
    ::testing::ValuesIn(utility::accessPolicyRepresentativeRules()),
    aclEntryTestName);

} // namespace facebook::fboss
