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
    cfg::AsicType /*asicType*/) {
  cfg::Ttl ttl;
  std::tie(*ttl.value(), *ttl.mask()) = std::make_tuple(0x80, 0x80);

  configureQualifier(acl->ipType(), enable, ipType);
  if (ipType == cfg::IpType::IP6) {
    configureQualifier(acl->srcIp(), enable, "::ffff:c0a8:1");
    configureQualifier(
        acl->dstIp(), enable, "2401:db00:3020:70e2:face:0:63:0/64");

    configureQualifier(
        acl->lookupClassNeighbor(),
        enable,
        cfg::AclLookupClass::DST_CLASS_L3_LOCAL_IP6);
    configureQualifier(
        acl->lookupClassRoute(),
        enable,
        cfg::AclLookupClass::DST_CLASS_L3_LOCAL_IP6);

  } else {
    configureQualifier(acl->srcIp(), enable, "192.168.0.1");
    configureQualifier(acl->dstIp(), enable, "192.168.0.0/24");

    configureQualifier(
        acl->lookupClassNeighbor(),
        enable,
        cfg::AclLookupClass::DST_CLASS_L3_LOCAL_IP4);
    configureQualifier(
        acl->lookupClassRoute(),
        enable,
        cfg::AclLookupClass::DST_CLASS_L3_LOCAL_IP4);
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

template <bool enableMultiAclTable>
struct EnableMultiAclTableT {
  static constexpr auto multiAclTableEnabled = enableMultiAclTable;
};

using TestTypes =
    ::testing::Types<EnableMultiAclTableT<false>, EnableMultiAclTableT<true>>;

template <typename EnableMultiAclTableT>
class HwAclQualifierTest : public HwTest {
  static auto constexpr isMultiAclEnabled =
      EnableMultiAclTableT::multiAclTableEnabled;

 public:
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

  void configureIp4QualifiersHelper(cfg::AclEntry* acl) {
    cfg::Ttl ttl;
    std::tie(*ttl.value(), *ttl.mask()) = std::make_tuple(0x80, 0x80);

    configureQualifier(acl->ipType(), true, cfg::IpType::IP4);
    configureQualifier(acl->srcIp(), true, "192.168.0.1");
    configureQualifier(acl->dstIp(), true, "192.168.0.0/24");
    configureQualifier(acl->dscp(), true, 0x24);
    configureQualifier(acl->ttl(), true, ttl);
    configureQualifier(acl->proto(), true, 6);
  }

  void configureIp6QualifiersHelper(cfg::AclEntry* acl) {
    cfg::Ttl ttl;
    std::tie(*ttl.value(), *ttl.mask()) = std::make_tuple(0x80, 0x80);

    configureQualifier(acl->ipType(), true, cfg::IpType::IP6);
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

  void aclSetupHelper(bool isIpV4, QualifierType lookupClassType) {
    auto newCfg = initialConfig();
    auto* acl = utility::addAcl(&newCfg, kAclName(), cfg::AclActionType::DENY);

    if (isIpV4) {
      this->configureIp4QualifiersHelper(acl);
    } else {
      this->configureIp6QualifiersHelper(acl);
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
            isIpV4 ? cfg::AclLookupClass::DST_CLASS_L3_LOCAL_IP4
                   : cfg::AclLookupClass::DST_CLASS_L3_LOCAL_IP6);
        break;
      case QualifierType::LOOKUPCLASS_ROUTE:
        configureQualifier(
            acl->lookupClassRoute(),
            true,
            isIpV4 ? cfg::AclLookupClass::DST_CLASS_L3_LOCAL_IP4
                   : cfg::AclLookupClass::DST_CLASS_L3_LOCAL_IP6);
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
    FLAGS_enable_acl_table_group = isMultiAclEnabled;
    HwTest::SetUp();
    // Skip multi acl tests for fake sdk
    if ((this->getPlatform()->getAsic()->getAsicType() ==
         cfg::AsicType::ASIC_TYPE_FAKE) &&
        (isMultiAclEnabled)) {
      GTEST_SKIP();
    }
  }

  cfg::SwitchConfig initialConfig() const {
    auto cfg = utility::onePortPerInterfaceConfig(
        getHwSwitch(),
        masterLogicalPortIds(),
        getAsic()->desiredLoopbackModes());
    if (isMultiAclEnabled) {
      utility::addAclTableGroup(
          &cfg, cfg::AclStage::INGRESS, utility::getAclTableGroupName());
      utility::addDefaultAclTable(cfg);
    }
    return cfg;
  }
};

TYPED_TEST_SUITE(HwAclQualifierTest, TestTypes);

TYPED_TEST(HwAclQualifierTest, AclIp4TcpQualifiers) {
  auto setup = [=, this]() {
    auto newCfg = this->initialConfig();
    auto* acl1 = utility::addAcl(&newCfg, "ip4_tcp", cfg::AclActionType::DENY);
    this->configureAllHwQualifiers(acl1, true);
    this->configureAllL2QualifiersHelper(acl1);
    configureAllIpQualifiers(acl1, true, cfg::IpType::IP4, this->getAsicType());
    configureAllTcpQualifiers(acl1, true, this->getAsicType());
    this->applyNewConfig(newCfg);
  };

  auto verify = [=, this]() {
    ASSERT_TRUE(utility::isAclTableEnabled(this->getHwSwitch()));
    EXPECT_EQ(utility::getAclTableNumAclEntries(this->getHwSwitch()), 1);
    utility::checkSwHwAclMatch(
        this->getHwSwitch(), this->getProgrammedState(), "ip4_tcp");
  };

  this->verifyAcrossWarmBoots(setup, verify);
}

TYPED_TEST(HwAclQualifierTest, AclIp6TcpQualifiers) {
  auto setup = [=, this]() {
    auto newCfg = this->initialConfig();
    auto* acl1 = utility::addAcl(&newCfg, "ip6_tcp", cfg::AclActionType::DENY);
    this->configureAllHwQualifiers(acl1, true);
    this->configureAllL2QualifiersHelper(acl1);
    configureAllIpQualifiers(acl1, true, cfg::IpType::IP6, this->getAsicType());
    configureAllTcpQualifiers(acl1, true, this->getAsicType());
    this->applyNewConfig(newCfg);
  };

  auto verify = [=, this]() {
    ASSERT_TRUE(utility::isAclTableEnabled(this->getHwSwitch()));
    EXPECT_EQ(utility::getAclTableNumAclEntries(this->getHwSwitch()), 1);
    utility::checkSwHwAclMatch(
        this->getHwSwitch(), this->getProgrammedState(), "ip6_tcp");
  };

  this->verifyAcrossWarmBoots(setup, verify);
}

TYPED_TEST(HwAclQualifierTest, AclIcmp4Qualifiers) {
  auto setup = [=, this]() {
    auto newCfg = this->initialConfig();
    auto* acl1 = utility::addAcl(&newCfg, "icmp4", cfg::AclActionType::DENY);
    this->configureAllHwQualifiers(acl1, true);
    this->configureAllL2QualifiersHelper(acl1);
    configureAllIcmpQualifiers(
        acl1, true, cfg::IpType::IP4, this->getAsicType());
    this->applyNewConfig(newCfg);
  };

  auto verify = [=, this]() {
    ASSERT_TRUE(utility::isAclTableEnabled(this->getHwSwitch()));
    EXPECT_EQ(utility::getAclTableNumAclEntries(this->getHwSwitch()), 1);
    utility::checkSwHwAclMatch(
        this->getHwSwitch(), this->getProgrammedState(), "icmp4");
  };

  this->verifyAcrossWarmBoots(setup, verify);
}

TYPED_TEST(HwAclQualifierTest, AclIcmp6Qualifiers) {
  auto setup = [=, this]() {
    auto newCfg = this->initialConfig();
    auto* acl1 = utility::addAcl(&newCfg, "icmp6", cfg::AclActionType::DENY);
    this->configureAllHwQualifiers(acl1, true);
    this->configureAllL2QualifiersHelper(acl1);
    configureAllIcmpQualifiers(
        acl1, true, cfg::IpType::IP6, this->getAsicType());
    this->applyNewConfig(newCfg);
  };

  auto verify = [=, this]() {
    ASSERT_TRUE(utility::isAclTableEnabled(this->getHwSwitch()));
    EXPECT_EQ(utility::getAclTableNumAclEntries(this->getHwSwitch()), 1);
    utility::checkSwHwAclMatch(
        this->getHwSwitch(), this->getProgrammedState(), "icmp6");
  };

  this->verifyAcrossWarmBoots(setup, verify);
}

TYPED_TEST(HwAclQualifierTest, AclRemove) {
  auto setup = [=, this]() {
    auto newCfg = this->initialConfig();
    auto* acl0 = utility::addAcl(&newCfg, "acl0", cfg::AclActionType::DENY);
    acl0->proto() = 6;
    auto* acl1 = utility::addAcl(&newCfg, "acl1", cfg::AclActionType::DENY);
    acl1->proto() = 58;
    this->applyNewConfig(newCfg);

    utility::delAcl(&newCfg, "acl0");
    this->applyNewConfig(newCfg);
  };

  auto verify = [=, this]() {
    ASSERT_TRUE(utility::isAclTableEnabled(this->getHwSwitch()));
    EXPECT_EQ(utility::getAclTableNumAclEntries(this->getHwSwitch()), 1);
    utility::checkSwHwAclMatch(
        this->getHwSwitch(), this->getProgrammedState(), "acl1");
  };

  this->verifyAcrossWarmBoots(setup, verify);
}

TYPED_TEST(HwAclQualifierTest, AclModifyQualifier) {
  auto setup = [=, this]() {
    auto newCfg = this->initialConfig();
    auto* acl = utility::addAcl(&newCfg, "acl0", cfg::AclActionType::DENY);
    // icmp6
    this->configureAllHwQualifiers(acl, true);
    this->configureAllL2QualifiersHelper(acl);
    configureAllIcmpQualifiers(
        acl, true, cfg::IpType::IP6, this->getAsicType());
    this->applyNewConfig(newCfg);
    // ip6 tcp
    configureAllIcmpQualifiers(
        acl, false, cfg::IpType::IP6, this->getAsicType());
    configureAllIpQualifiers(acl, true, cfg::IpType::IP6, this->getAsicType());
    this->applyNewConfig(newCfg);
    // imcp6
    configureAllIpQualifiers(acl, false, cfg::IpType::IP6, this->getAsicType());
    configureAllIcmpQualifiers(
        acl, true, cfg::IpType::IP6, this->getAsicType());
    this->applyNewConfig(newCfg);
  };

  auto verify = [=, this]() {
    ASSERT_TRUE(utility::isAclTableEnabled(this->getHwSwitch()));
    EXPECT_EQ(utility::getAclTableNumAclEntries(this->getHwSwitch()), 1);
    utility::checkSwHwAclMatch(
        this->getHwSwitch(), this->getProgrammedState(), "acl0");
    EXPECT_FALSE(utility::isQualifierPresent<cfg::IpFragMatch>(
        this->getHwSwitch(), this->getProgrammedState(), "acl0"));
  };

  this->verifyAcrossWarmBoots(setup, verify);
}

TYPED_TEST(HwAclQualifierTest, AclEmptyCodeIcmp) {
  auto setup = [=, this]() {
    auto newCfg = this->initialConfig();
    auto* acl = utility::addAcl(&newCfg, "acl0", cfg::AclActionType::DENY);
    // add a icmp rule w/ type and code value
    // Destination Unreachable(type=3):Source host isolated(code=8)
    acl->proto() = 58;
    acl->icmpType() = 3;
    acl->icmpCode() = 8;
    this->applyNewConfig(newCfg);
    // change the rule to empty code icmp rule
    // Reserved for security(type=19 code is unset)
    acl->icmpType() = 19;
    acl->icmpCode().reset();
    this->applyNewConfig(newCfg);
  };

  auto verify = [=, this]() {
    EXPECT_EQ(utility::getAclTableNumAclEntries(this->getHwSwitch()), 1);
    utility::checkSwHwAclMatch(
        this->getHwSwitch(), this->getProgrammedState(), "acl0");
  };

  this->verifyAcrossWarmBoots(setup, verify);
}

TYPED_TEST(HwAclQualifierTest, AclVlanIDQualifier) {
  auto setup = [=, this]() {
    auto newCfg = this->initialConfig();
    if (this->getPlatform()->getAsic()->getAsicType() ==
        cfg::AsicType::ASIC_TYPE_TRIDENT2) {
#if defined(GTEST_SKIP)
      GTEST_SKIP();
#endif
      return;
    }
    auto* acl = utility::addAcl(&newCfg, "acl0", cfg::AclActionType::DENY);

    acl->vlanID() = 2001;
    this->applyNewConfig(newCfg);
  };

  auto verify = [=, this]() {
    if (this->getPlatform()->getAsic()->getAsicType() ==
        cfg::AsicType::ASIC_TYPE_TRIDENT2) {
      return;
    }
    EXPECT_EQ(utility::getAclTableNumAclEntries(this->getHwSwitch()), 1);
    utility::checkSwHwAclMatch(
        this->getHwSwitch(), this->getProgrammedState(), "acl0");
  };

  this->verifyAcrossWarmBoots(setup, verify);
}

TYPED_TEST(HwAclQualifierTest, AclIp4Qualifiers) {
  auto setup = [=, this]() {
    auto newCfg = this->initialConfig();
    auto* acl = utility::addAcl(&newCfg, "ip4", cfg::AclActionType::DENY);
    this->configureIp4QualifiersHelper(acl);
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

TYPED_TEST(HwAclQualifierTest, AclIp6Qualifiers) {
  auto setup = [=, this]() {
    auto newCfg = this->initialConfig();
    auto* acl = utility::addAcl(&newCfg, "ip6", cfg::AclActionType::DENY);
    this->configureIp6QualifiersHelper(acl);
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

TYPED_TEST(HwAclQualifierTest, AclIp4LookupClassL2) {
  auto setup = [=, this]() {
    this->aclSetupHelper(true /* isIpV4 */, QualifierType::LOOKUPCLASS_L2);
  };

  auto verify = [=, this]() { this->aclVerifyHelper(); };

  this->verifyAcrossWarmBoots(setup, verify);
}

TYPED_TEST(HwAclQualifierTest, AclIp4LookupClassNeighbor) {
  auto setup = [=, this]() {
    this->aclSetupHelper(
        true /* isIpV4 */, QualifierType::LOOKUPCLASS_NEIGHBOR);
  };

  auto verify = [=, this]() { this->aclVerifyHelper(); };

  this->verifyAcrossWarmBoots(setup, verify);
}

TYPED_TEST(HwAclQualifierTest, AclIp4LookupClassRoute) {
  auto setup = [=, this]() {
    this->aclSetupHelper(true /* isIpV4 */, QualifierType::LOOKUPCLASS_ROUTE);
  };

  auto verify = [=, this]() { this->aclVerifyHelper(); };

  this->verifyAcrossWarmBoots(setup, verify);
}

TYPED_TEST(HwAclQualifierTest, AclIp6LookupClassL2) {
  auto setup = [=, this]() {
    this->aclSetupHelper(false /* isIpV6 */, QualifierType::LOOKUPCLASS_L2);
  };

  auto verify = [=, this]() { this->aclVerifyHelper(); };

  this->verifyAcrossWarmBoots(setup, verify);
}

TYPED_TEST(HwAclQualifierTest, AclIp6LookupClassNeighbor) {
  auto setup = [=, this]() {
    this->aclSetupHelper(
        false /* isIpV6 */, QualifierType::LOOKUPCLASS_NEIGHBOR);
  };

  auto verify = [=, this]() { this->aclVerifyHelper(); };

  this->verifyAcrossWarmBoots(setup, verify);
}

TYPED_TEST(HwAclQualifierTest, AclIp6LookupClassRoute) {
  auto setup = [=, this]() {
    this->aclSetupHelper(false /* isIpV6 */, QualifierType::LOOKUPCLASS_ROUTE);
  };

  auto verify = [=, this]() { this->aclVerifyHelper(); };

  this->verifyAcrossWarmBoots(setup, verify);
}

} // namespace facebook::fboss
