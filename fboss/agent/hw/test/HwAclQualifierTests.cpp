/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/gen-cpp2/switch_config_types.h"
#include "fboss/agent/hw/test/ConfigFactory.h"
#include "fboss/agent/hw/test/HwTest.h"
#include "fboss/agent/hw/test/HwTestAclUtils.h"
#include "fboss/agent/state/SwitchState.h"
#include "fboss/agent/test/utils/QueuePerHostTestUtils.h"

#include <string>

namespace {

using namespace facebook::fboss;

enum class QualifierType : uint8_t {
  LOOKUPCLASS_L2,
  LOOKUPCLASS_NEIGHBOR,
  LOOKUPCLASS_ROUTE,
  LOOKUPCLASS_IGNORE,
};

template <typename T, typename U>
void configureQualifier(
    apache::thrift::optional_field_ref<T&> ref,
    bool enable,
    const U& val) {
  if (enable) {
    ref = val;
  } else {
    ref.reset();
  }
}

void configureAllIpQualifiers(
    cfg::AclEntry* acl,
    bool enable,
    cfg::IpType ipType,
    cfg::AsicType asicType) {
  cfg::Ttl ttl;
  std::tie(*ttl.value(), *ttl.mask()) = std::make_tuple(0x80, 0x80);

  if (asicType != cfg::AsicType::ASIC_TYPE_JERICHO3) {
    // TODO(daiweix): remove after J3 ACL supports IP_TYPE
    configureQualifier(acl->ipType(), enable, ipType);
  }
  if (ipType == cfg::IpType::IP6) {
    configureQualifier(acl->srcIp(), enable, "::ffff:c0a8:1");
    configureQualifier(
        acl->dstIp(), enable, "2401:db00:3020:70e2:face:0:63:0/64");

    configureQualifier(
        acl->lookupClassNeighbor(),
        enable,
        cfg::AclLookupClass::DST_CLASS_L3_LOCAL_2);
    configureQualifier(
        acl->lookupClassRoute(),
        enable,
        cfg::AclLookupClass::DST_CLASS_L3_LOCAL_2);

  } else {
    configureQualifier(acl->srcIp(), enable, "192.168.0.1");
    configureQualifier(acl->dstIp(), enable, "192.168.0.0/24");

    configureQualifier(
        acl->lookupClassNeighbor(),
        enable,
        cfg::AclLookupClass::DST_CLASS_L3_LOCAL_1);
    configureQualifier(
        acl->lookupClassRoute(),
        enable,
        cfg::AclLookupClass::DST_CLASS_L3_LOCAL_1);
  }
  configureQualifier(
      acl->ipFrag(), enable, cfg::IpFragMatch::MATCH_FIRST_FRAGMENT);
  configureQualifier(acl->dscp(), enable, 0x24);
  configureQualifier(acl->ttl(), enable, ttl);
}

void configureAllTcpQualifiers(
    cfg::AclEntry* acl,
    bool enable,
    cfg::AsicType /*asicType*/) {
  configureQualifier(acl->l4SrcPort(), enable, 10);
  configureQualifier(acl->l4DstPort(), enable, 20);
  configureQualifier(acl->proto(), enable, 6);
  configureQualifier(acl->tcpFlagsBitMap(), enable, 16);
}

void configureAllIcmpQualifiers(
    cfg::AclEntry* acl,
    bool enable,
    cfg::IpType ipType,
    cfg::AsicType /*asicType*/) {
  if (ipType == cfg::IpType::IP6) {
    configureQualifier(acl->proto(), enable, 58); // Icmp v6
    configureQualifier(acl->icmpType(), enable, 1); // Destination unreachable
    configureQualifier(acl->icmpCode(), enable, 4); // Port unreachable
  } else {
    configureQualifier(acl->proto(), enable, 1); // Icmp v4
    configureQualifier(acl->icmpType(), enable, 3); // Destination unreachable
    configureQualifier(acl->icmpCode(), enable, 3); // Port unreachable
  }
}

} // unnamed namespace

namespace facebook::fboss {

class HwAclQualifierTest : public HwTest {
 public:
  bool addQualifiers = false;
  void configureAllHwQualifiers(cfg::AclEntry* acl, bool enable) {
    configureQualifier(
        acl->srcPort(), enable, masterLogicalInterfacePortIds()[0]);
    if ((getAsicType() != cfg::AsicType::ASIC_TYPE_JERICHO2) &&
        (getAsicType() != cfg::AsicType::ASIC_TYPE_JERICHO3)) {
      // No out port support on J2. Out port not used in prod
      configureQualifier(
          acl->dstPort(), enable, masterLogicalInterfacePortIds()[1]);
    }
  }

  void configureAllL2QualifiersHelper(cfg::AclEntry* acl) {
    configureQualifier(acl->dstMac(), true, "00:11:22:33:44:55");
    /*
     * lookupClassL2 is not configured for Trident2 or else we run out of
     * resources.
     * Note: lookupclassL2 is needed for MH-NIC queue-per-host solution.
     * However, the solution is not applicable for Trident2 as we don't
     * implement queues on trident2.
     */
    if (getPlatform()->getAsic()->getAsicType() !=
            cfg::AsicType::ASIC_TYPE_TRIDENT2 &&
        // L2 switching only on NPU type switch
        getHwSwitch()->getSwitchType() == cfg::SwitchType::NPU) {
      configureQualifier(
          acl->lookupClassL2(),
          true,
          cfg::AclLookupClass::CLASS_QUEUE_PER_HOST_QUEUE_1);
    }
  }

  void configureIp4QualifiersHelper(
      cfg::AclEntry* acl,
      cfg::AsicType asicType) {
    cfg::Ttl ttl;
    std::tie(*ttl.value(), *ttl.mask()) = std::make_tuple(0x80, 0x80);

    if (asicType != cfg::AsicType::ASIC_TYPE_JERICHO3) {
      // TODO(daiweix): remove after J3 ACL supports IP_TYPE
      configureQualifier(acl->ipType(), true, cfg::IpType::IP4);
    }
    configureQualifier(acl->srcIp(), true, "192.168.0.1");
    configureQualifier(acl->dstIp(), true, "192.168.0.0/24");
    configureQualifier(acl->dscp(), true, 0x24);
    configureQualifier(acl->ttl(), true, ttl);
    configureQualifier(acl->proto(), true, 6);
  }

  void configureIp6QualifiersHelper(
      cfg::AclEntry* acl,
      cfg::AsicType asicType) {
    cfg::Ttl ttl;
    std::tie(*ttl.value(), *ttl.mask()) = std::make_tuple(0x80, 0x80);

    if (asicType != cfg::AsicType::ASIC_TYPE_JERICHO3) {
      // TODO(daiweix): remove after J3 ACL supports IP_TYPE
      configureQualifier(acl->ipType(), true, cfg::IpType::IP6);
    }
    configureQualifier(acl->srcIp(), true, "::ffff:c0a8:1");
    configureQualifier(
        acl->dstIp(), true, "2401:db00:3020:70e2:face:0:63:0/64");
    configureQualifier(acl->dscp(), true, 0x24);
    configureQualifier(acl->ttl(), true, ttl);
    configureQualifier(acl->proto(), true, 6);
  }

  std::string kAclName() const {
    return "acl0";
  }

  void aclSetupHelper(
      bool isIpV4,
      QualifierType lookupClassType,
      bool addQualifiers = false) {
    this->addQualifiers = addQualifiers;
    auto newCfg = initialConfig();
    auto* acl = utility::addAcl(&newCfg, kAclName(), cfg::AclActionType::DENY);

    if (isIpV4) {
      this->configureIp4QualifiersHelper(acl, this->getAsicType());
    } else {
      this->configureIp6QualifiersHelper(acl, this->getAsicType());
    }

    switch (lookupClassType) {
      case QualifierType::LOOKUPCLASS_L2:
        if (getPlatform()->getAsic()->getAsicType() !=
            cfg::AsicType::ASIC_TYPE_TRIDENT2) {
          configureQualifier(
              acl->lookupClassL2(),
              true,
              cfg::AclLookupClass::CLASS_QUEUE_PER_HOST_QUEUE_1);
        }
        break;
      case QualifierType::LOOKUPCLASS_NEIGHBOR:
        configureQualifier(
            acl->lookupClassNeighbor(),
            true,
            isIpV4 ? cfg::AclLookupClass::DST_CLASS_L3_LOCAL_1
                   : cfg::AclLookupClass::DST_CLASS_L3_LOCAL_2);
        break;
      case QualifierType::LOOKUPCLASS_ROUTE:
        configureQualifier(
            acl->lookupClassRoute(),
            true,
            isIpV4 ? cfg::AclLookupClass::DST_CLASS_L3_LOCAL_1
                   : cfg::AclLookupClass::DST_CLASS_L3_LOCAL_2);
        break;
      case QualifierType::LOOKUPCLASS_IGNORE:
        // This case is here exactly to not do anything
        break;
      default:
        CHECK(false);
    }

    applyNewConfig(newCfg);
  }

  void aclVerifyHelper() {
    ASSERT_TRUE(utility::isAclTableEnabled(getHwSwitch()));
    EXPECT_EQ(utility::getAclTableNumAclEntries(getHwSwitch()), 1);
    utility::checkSwHwAclMatch(getHwSwitch(), getProgrammedState(), kAclName());
  }

 protected:
  void SetUp() override {
    HwTest::SetUp();
    // Skip multi acl tests for fake sdk
    if ((this->getPlatform()->getAsic()->getAsicType() ==
         cfg::AsicType::ASIC_TYPE_FAKE) &&
        (FLAGS_enable_acl_table_group)) {
      GTEST_SKIP();
    }
  }

  cfg::SwitchConfig initialConfig() const {
    auto cfg = utility::onePortPerInterfaceConfig(
        getHwSwitch(),
        masterLogicalPortIds(),
        getAsic()->desiredLoopbackModes());
    if (FLAGS_enable_acl_table_group) {
      utility::addAclTableGroup(
          &cfg, cfg::AclStage::INGRESS, utility::getAclTableGroupName());
      std::vector<cfg::AclTableActionType> actions = {};
      std::vector<cfg::AclTableQualifier> qualifiers = addQualifiers
          ? utility::genAclQualifiersConfig(this->getAsicType())
          : std::vector<cfg::AclTableQualifier>();
      utility::addAclTable(
          &cfg,
          utility::kDefaultAclTable(),
          0 /* priority */,
          actions,
          qualifiers);
    }
    return cfg;
  }
};

TEST_F(HwAclQualifierTest, AclIp4Qualifiers) {
  auto setup = [=, this]() {
    auto newCfg = this->initialConfig();
    auto* acl = utility::addAcl(&newCfg, "ip4", cfg::AclActionType::DENY);
    this->configureIp4QualifiersHelper(acl, this->getAsicType());
    this->applyNewConfig(newCfg);
  };

  auto verify = [=, this]() {
    ASSERT_TRUE(utility::isAclTableEnabled(this->getHwSwitch()));
    EXPECT_EQ(utility::getAclTableNumAclEntries(this->getHwSwitch()), 1);
    utility::checkSwHwAclMatch(
        this->getHwSwitch(), this->getProgrammedState(), "ip4");
  };

  this->verifyAcrossWarmBoots(setup, verify);
}

TEST_F(HwAclQualifierTest, AclIp6Qualifiers) {
  auto setup = [=, this]() {
    auto newCfg = this->initialConfig();
    auto* acl = utility::addAcl(&newCfg, "ip6", cfg::AclActionType::DENY);
    this->configureIp6QualifiersHelper(acl, this->getAsicType());
    this->applyNewConfig(newCfg);
  };

  auto verify = [=, this]() {
    ASSERT_TRUE(utility::isAclTableEnabled(this->getHwSwitch()));
    EXPECT_EQ(utility::getAclTableNumAclEntries(this->getHwSwitch()), 1);
    utility::checkSwHwAclMatch(
        this->getHwSwitch(), this->getProgrammedState(), "ip6");
  };

  this->verifyAcrossWarmBoots(setup, verify);
}

TEST_F(HwAclQualifierTest, AclIp4LookupClassL2) {
  auto setup = [=, this]() {
    this->aclSetupHelper(true /* isIpV4 */, QualifierType::LOOKUPCLASS_L2);
  };

  auto verify = [=, this]() { this->aclVerifyHelper(); };

  this->verifyAcrossWarmBoots(setup, verify);
}

TEST_F(HwAclQualifierTest, AclIp4LookupClassNeighbor) {
  auto setup = [=, this]() {
    this->aclSetupHelper(
        true /* isIpV4 */, QualifierType::LOOKUPCLASS_NEIGHBOR);
  };

  auto verify = [=, this]() { this->aclVerifyHelper(); };

  this->verifyAcrossWarmBoots(setup, verify);
}

TEST_F(HwAclQualifierTest, AclIp4LookupClassRoute) {
  auto setup = [=, this]() {
    this->aclSetupHelper(true /* isIpV4 */, QualifierType::LOOKUPCLASS_ROUTE);
  };

  auto verify = [=, this]() { this->aclVerifyHelper(); };

  this->verifyAcrossWarmBoots(setup, verify);
}

TEST_F(HwAclQualifierTest, AclIp6LookupClassL2) {
  auto setup = [=, this]() {
    this->aclSetupHelper(false /* isIpV6 */, QualifierType::LOOKUPCLASS_L2);
  };

  auto verify = [=, this]() { this->aclVerifyHelper(); };

  this->verifyAcrossWarmBoots(setup, verify);
}

TEST_F(HwAclQualifierTest, AclIp6LookupClassNeighbor) {
  auto setup = [=, this]() {
    this->aclSetupHelper(
        false /* isIpV6 */, QualifierType::LOOKUPCLASS_NEIGHBOR);
  };

  auto verify = [=, this]() { this->aclVerifyHelper(); };

  this->verifyAcrossWarmBoots(setup, verify);
}

TEST_F(HwAclQualifierTest, AclIp6LookupClassRoute) {
  auto setup = [=, this]() {
    this->aclSetupHelper(false /* isIpV6 */, QualifierType::LOOKUPCLASS_ROUTE);
  };

  auto verify = [=, this]() { this->aclVerifyHelper(); };

  this->verifyAcrossWarmBoots(setup, verify);
}

// canary on for qualifiers from default to coop
TEST_F(HwAclQualifierTest, AclQualifiersCanaryOn) {
  auto setup = [=, this]() {
    this->aclSetupHelper(true, QualifierType::LOOKUPCLASS_IGNORE, false);
  };

  auto verify = [=, this]() { this->aclVerifyHelper(); };

  auto setupPostWarmboot = [=, this]() {
    this->aclSetupHelper(true, QualifierType::LOOKUPCLASS_IGNORE, true);
  };

  this->verifyAcrossWarmBoots(setup, verify, setupPostWarmboot, []() {});
}

// canary off for qualifiers from coop to default
TEST_F(HwAclQualifierTest, AclQualifiersCanaryOff) {
  auto setup = [=, this]() {
    this->aclSetupHelper(true, QualifierType::LOOKUPCLASS_IGNORE, true);
  };

  auto verify = [=, this]() { this->aclVerifyHelper(); };

  auto setupPostWarmboot = [=, this]() {
    this->aclSetupHelper(true, QualifierType::LOOKUPCLASS_IGNORE, false);
  };

  this->verifyAcrossWarmBoots(setup, verify, setupPostWarmboot, []() {});
}

} // namespace facebook::fboss
