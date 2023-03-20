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

class HwAclQualifierTest : public HwTest {
 public:
  void configureAllHwQualifiers(cfg::AclEntry* acl, bool enable) {
    configureQualifier(
        acl->srcPort(), enable, masterLogicalInterfacePortIds()[0]);
    if (getAsicType() != cfg::AsicType::ASIC_TYPE_JERICHO2) {
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
      configureIp4QualifiersHelper(acl);
    } else {
      configureIp6QualifiersHelper(acl);
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
  cfg::SwitchConfig initialConfig() const {
    return utility::onePortPerInterfaceConfig(
        getHwSwitch(), masterLogicalPortIds());
  }
};

TEST_F(HwAclQualifierTest, AclIp4TcpQualifiers) {
  auto setup = [=]() {
    auto newCfg = initialConfig();
    auto* acl1 = utility::addAcl(&newCfg, "ip4_tcp", cfg::AclActionType::DENY);
    configureAllHwQualifiers(acl1, true);
    configureAllL2QualifiersHelper(acl1);
    configureAllIpQualifiers(acl1, true, cfg::IpType::IP4, getAsicType());
    configureAllTcpQualifiers(acl1, true, getAsicType());
    applyNewConfig(newCfg);
  };

  auto verify = [=]() {
    ASSERT_TRUE(utility::isAclTableEnabled(getHwSwitch()));
    EXPECT_EQ(utility::getAclTableNumAclEntries(getHwSwitch()), 1);
    utility::checkSwHwAclMatch(getHwSwitch(), getProgrammedState(), "ip4_tcp");
  };

  verifyAcrossWarmBoots(setup, verify);
}

TEST_F(HwAclQualifierTest, AclIp6TcpQualifiers) {
  auto setup = [=]() {
    auto newCfg = initialConfig();
    auto* acl1 = utility::addAcl(&newCfg, "ip6_tcp", cfg::AclActionType::DENY);
    configureAllHwQualifiers(acl1, true);
    configureAllL2QualifiersHelper(acl1);
    configureAllIpQualifiers(acl1, true, cfg::IpType::IP6, getAsicType());
    configureAllTcpQualifiers(acl1, true, getAsicType());
    applyNewConfig(newCfg);
  };

  auto verify = [=]() {
    ASSERT_TRUE(utility::isAclTableEnabled(getHwSwitch()));
    EXPECT_EQ(utility::getAclTableNumAclEntries(getHwSwitch()), 1);
    utility::checkSwHwAclMatch(getHwSwitch(), getProgrammedState(), "ip6_tcp");
  };

  verifyAcrossWarmBoots(setup, verify);
}

TEST_F(HwAclQualifierTest, AclIcmp4Qualifiers) {
  auto setup = [=]() {
    auto newCfg = initialConfig();
    auto* acl1 = utility::addAcl(&newCfg, "icmp4", cfg::AclActionType::DENY);
    configureAllHwQualifiers(acl1, true);
    configureAllL2QualifiersHelper(acl1);
    configureAllIcmpQualifiers(acl1, true, cfg::IpType::IP4, getAsicType());
    applyNewConfig(newCfg);
  };

  auto verify = [=]() {
    ASSERT_TRUE(utility::isAclTableEnabled(getHwSwitch()));
    EXPECT_EQ(utility::getAclTableNumAclEntries(getHwSwitch()), 1);
    utility::checkSwHwAclMatch(getHwSwitch(), getProgrammedState(), "icmp4");
  };

  verifyAcrossWarmBoots(setup, verify);
}

TEST_F(HwAclQualifierTest, AclIcmp6Qualifiers) {
  auto setup = [=]() {
    auto newCfg = initialConfig();
    auto* acl1 = utility::addAcl(&newCfg, "icmp6", cfg::AclActionType::DENY);
    configureAllHwQualifiers(acl1, true);
    configureAllL2QualifiersHelper(acl1);
    configureAllIcmpQualifiers(acl1, true, cfg::IpType::IP6, getAsicType());
    applyNewConfig(newCfg);
  };

  auto verify = [=]() {
    ASSERT_TRUE(utility::isAclTableEnabled(getHwSwitch()));
    EXPECT_EQ(utility::getAclTableNumAclEntries(getHwSwitch()), 1);
    utility::checkSwHwAclMatch(getHwSwitch(), getProgrammedState(), "icmp6");
  };

  verifyAcrossWarmBoots(setup, verify);
}

TEST_F(HwAclQualifierTest, AclRemove) {
  auto setup = [=]() {
    auto newCfg = initialConfig();
    auto* acl0 = utility::addAcl(&newCfg, "acl0", cfg::AclActionType::DENY);
    acl0->proto() = 6;
    auto* acl1 = utility::addAcl(&newCfg, "acl1", cfg::AclActionType::DENY);
    acl1->proto() = 58;
    applyNewConfig(newCfg);

    utility::delAcl(&newCfg, "acl0");
    applyNewConfig(newCfg);
  };

  auto verify = [=]() {
    ASSERT_TRUE(utility::isAclTableEnabled(getHwSwitch()));
    EXPECT_EQ(utility::getAclTableNumAclEntries(getHwSwitch()), 1);
    utility::checkSwHwAclMatch(getHwSwitch(), getProgrammedState(), "acl1");
  };

  verifyAcrossWarmBoots(setup, verify);
}

TEST_F(HwAclQualifierTest, AclModifyQualifier) {
  auto setup = [=]() {
    auto newCfg = initialConfig();
    auto* acl = utility::addAcl(&newCfg, "acl0", cfg::AclActionType::DENY);
    // icmp6
    configureAllHwQualifiers(acl, true);
    configureAllL2QualifiersHelper(acl);
    configureAllIcmpQualifiers(acl, true, cfg::IpType::IP6, getAsicType());
    applyNewConfig(newCfg);
    // ip6 tcp
    configureAllIcmpQualifiers(acl, false, cfg::IpType::IP6, getAsicType());
    configureAllIpQualifiers(acl, true, cfg::IpType::IP6, getAsicType());
    applyNewConfig(newCfg);
    // imcp6
    configureAllIpQualifiers(acl, false, cfg::IpType::IP6, getAsicType());
    configureAllIcmpQualifiers(acl, true, cfg::IpType::IP6, getAsicType());
    applyNewConfig(newCfg);
  };

  auto verify = [=]() {
    ASSERT_TRUE(utility::isAclTableEnabled(getHwSwitch()));
    EXPECT_EQ(utility::getAclTableNumAclEntries(getHwSwitch()), 1);
    utility::checkSwHwAclMatch(getHwSwitch(), getProgrammedState(), "acl0");
    EXPECT_FALSE(utility::isQualifierPresent<cfg::IpFragMatch>(
        getHwSwitch(), getProgrammedState(), "acl0"));
  };

  verifyAcrossWarmBoots(setup, verify);
}

TEST_F(HwAclQualifierTest, AclEmptyCodeIcmp) {
  auto setup = [=]() {
    auto newCfg = initialConfig();
    auto* acl = utility::addAcl(&newCfg, "acl0", cfg::AclActionType::DENY);
    // add a icmp rule w/ type and code value
    // Destination Unreachable(type=3):Source host isolated(code=8)
    acl->proto() = 58;
    acl->icmpType() = 3;
    acl->icmpCode() = 8;
    applyNewConfig(newCfg);
    // change the rule to empty code icmp rule
    // Reserved for security(type=19 code is unset)
    acl->icmpType() = 19;
    acl->icmpCode().reset();
    applyNewConfig(newCfg);
  };

  auto verify = [=]() {
    EXPECT_EQ(utility::getAclTableNumAclEntries(getHwSwitch()), 1);
    utility::checkSwHwAclMatch(getHwSwitch(), getProgrammedState(), "acl0");
  };

  verifyAcrossWarmBoots(setup, verify);
}

TEST_F(HwAclQualifierTest, AclVlanIDQualifier) {
  auto setup = [=]() {
    auto newCfg = initialConfig();
    if (getPlatform()->getAsic()->getAsicType() ==
        cfg::AsicType::ASIC_TYPE_TRIDENT2) {
      return;
    }
    auto* acl = utility::addAcl(&newCfg, "acl0", cfg::AclActionType::DENY);

    acl->vlanID() = 2001;
    applyNewConfig(newCfg);
  };

  auto verify = [=]() {
    if (getPlatform()->getAsic()->getAsicType() ==
        cfg::AsicType::ASIC_TYPE_TRIDENT2) {
      return;
    }
    EXPECT_EQ(utility::getAclTableNumAclEntries(getHwSwitch()), 1);
    utility::checkSwHwAclMatch(getHwSwitch(), getProgrammedState(), "acl0");
  };

  verifyAcrossWarmBoots(setup, verify);
}

TEST_F(HwAclQualifierTest, AclIp4Qualifiers) {
  auto setup = [=]() {
    auto newCfg = initialConfig();
    auto* acl = utility::addAcl(&newCfg, "ip4", cfg::AclActionType::DENY);
    configureIp4QualifiersHelper(acl);
    applyNewConfig(newCfg);
  };

  auto verify = [=]() {
    ASSERT_TRUE(utility::isAclTableEnabled(getHwSwitch()));
    EXPECT_EQ(utility::getAclTableNumAclEntries(getHwSwitch()), 1);
    utility::checkSwHwAclMatch(getHwSwitch(), getProgrammedState(), "ip4");
  };

  verifyAcrossWarmBoots(setup, verify);
}

TEST_F(HwAclQualifierTest, AclIp6Qualifiers) {
  auto setup = [=]() {
    auto newCfg = initialConfig();
    auto* acl = utility::addAcl(&newCfg, "ip6", cfg::AclActionType::DENY);
    configureIp6QualifiersHelper(acl);
    applyNewConfig(newCfg);
  };

  auto verify = [=]() {
    ASSERT_TRUE(utility::isAclTableEnabled(getHwSwitch()));
    EXPECT_EQ(utility::getAclTableNumAclEntries(getHwSwitch()), 1);
    utility::checkSwHwAclMatch(getHwSwitch(), getProgrammedState(), "ip6");
  };

  verifyAcrossWarmBoots(setup, verify);
}

TEST_F(HwAclQualifierTest, AclIp4LookupClassL2) {
  auto setup = [=]() {
    aclSetupHelper(true /* isIpV4 */, QualifierType::LOOKUPCLASS_L2);
  };

  auto verify = [=]() { aclVerifyHelper(); };

  verifyAcrossWarmBoots(setup, verify);
}

TEST_F(HwAclQualifierTest, AclIp4LookupClassNeighbor) {
  auto setup = [=]() {
    aclSetupHelper(true /* isIpV4 */, QualifierType::LOOKUPCLASS_NEIGHBOR);
  };

  auto verify = [=]() { aclVerifyHelper(); };

  verifyAcrossWarmBoots(setup, verify);
}

TEST_F(HwAclQualifierTest, AclIp4LookupClassRoute) {
  auto setup = [=]() {
    aclSetupHelper(true /* isIpV4 */, QualifierType::LOOKUPCLASS_ROUTE);
  };

  auto verify = [=]() { aclVerifyHelper(); };

  verifyAcrossWarmBoots(setup, verify);
}

TEST_F(HwAclQualifierTest, AclIp6LookupClassL2) {
  auto setup = [=]() {
    aclSetupHelper(false /* isIpV6 */, QualifierType::LOOKUPCLASS_L2);
  };

  auto verify = [=]() { aclVerifyHelper(); };

  verifyAcrossWarmBoots(setup, verify);
}

TEST_F(HwAclQualifierTest, AclIp6LookupClassNeighbor) {
  auto setup = [=]() {
    aclSetupHelper(false /* isIpV6 */, QualifierType::LOOKUPCLASS_NEIGHBOR);
  };

  auto verify = [=]() { aclVerifyHelper(); };

  verifyAcrossWarmBoots(setup, verify);
}

TEST_F(HwAclQualifierTest, AclIp6LookupClassRoute) {
  auto setup = [=]() {
    aclSetupHelper(false /* isIpV6 */, QualifierType::LOOKUPCLASS_ROUTE);
  };

  auto verify = [=]() { aclVerifyHelper(); };

  verifyAcrossWarmBoots(setup, verify);
}

} // namespace facebook::fboss
