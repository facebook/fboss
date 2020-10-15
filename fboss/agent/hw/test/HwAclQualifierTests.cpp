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
    cfg::IpType ipType) {
  cfg::Ttl ttl;
  std::tie(*ttl.value_ref(), *ttl.mask_ref()) = std::make_tuple(0x80, 0x80);

  configureQualifier(acl->ipType_ref(), enable, ipType);
  if (ipType == cfg::IpType::IP6) {
    configureQualifier(acl->srcIp_ref(), enable, "::ffff:c0a8:1");
    configureQualifier(
        acl->dstIp_ref(), enable, "2401:db00:3020:70e2:face:0:63:0/64");
    configureQualifier(
        acl->lookupClass_ref(),
        enable,
        cfg::AclLookupClass::DST_CLASS_L3_LOCAL_IP6);
  } else {
    configureQualifier(acl->srcIp_ref(), enable, "192.168.0.1");
    configureQualifier(acl->dstIp_ref(), enable, "192.168.0.0/24");
    configureQualifier(
        acl->lookupClass_ref(),
        enable,
        cfg::AclLookupClass::DST_CLASS_L3_LOCAL_IP4);
  }
  configureQualifier(
      acl->ipFrag_ref(), enable, cfg::IpFragMatch::MATCH_FIRST_FRAGMENT);
  configureQualifier(acl->dscp_ref(), enable, 0x24);
  configureQualifier(acl->ttl_ref(), enable, ttl);
}

void configureAllHwQualifiers(cfg::AclEntry* acl, bool enable) {
  configureQualifier(acl->srcPort_ref(), enable, 5);
  configureQualifier(acl->dstPort_ref(), enable, 8);
}

void configureAllTcpQualifiers(cfg::AclEntry* acl, bool enable) {
  configureQualifier(acl->l4SrcPort_ref(), enable, 10);
  configureQualifier(acl->l4DstPort_ref(), enable, 20);
  configureQualifier(acl->proto_ref(), enable, 6);
  configureQualifier(acl->tcpFlagsBitMap_ref(), enable, 16);
}

void configureAllIcmpQualifiers(
    cfg::AclEntry* acl,
    bool enable,
    cfg::IpType ipType) {
  if (ipType == cfg::IpType::IP6) {
    configureQualifier(acl->proto_ref(), enable, 58); // Icmp v6
    configureQualifier(
        acl->icmpType_ref(), enable, 1); // Destination unreachable
    configureQualifier(acl->icmpCode_ref(), enable, 4); // Port unreachable
  } else {
    configureQualifier(acl->proto_ref(), enable, 1); // Icmp v4
    configureQualifier(
        acl->icmpType_ref(), enable, 3); // Destination unreachable
    configureQualifier(acl->icmpCode_ref(), enable, 3); // Port unreachable
  }
}

} // unnamed namespace

namespace facebook::fboss {

class HwAclQualifierTest : public HwTest {
 public:
  void configureAllL2QualifiersHelper(cfg::AclEntry* acl) {
    configureQualifier(acl->dstMac_ref(), true, "00:11:22:33:44:55");
    /*
     * lookupClassL2 is not configured for Trident2 or else we run out of
     * resources.
     * Note: lookupclassL2 is needed for MH-NIC queue-per-host solution.
     * However, the solution is not applicable for Trident2 as we don't
     * implement queues on trident2.
     */
    if (getPlatform()->getAsic()->getAsicType() !=
        HwAsic::AsicType::ASIC_TYPE_TRIDENT2) {
      configureQualifier(
          acl->lookupClassL2_ref(),
          true,
          cfg::AclLookupClass::CLASS_QUEUE_PER_HOST_QUEUE_1);
    }
  }

  void configureIp4QualifiersHelper(cfg::AclEntry* acl) {
    cfg::Ttl ttl;
    std::tie(*ttl.value_ref(), *ttl.mask_ref()) = std::make_tuple(0x80, 0x80);

    configureQualifier(acl->ipType_ref(), true, cfg::IpType::IP4);
    configureQualifier(acl->srcIp_ref(), true, "192.168.0.1");
    configureQualifier(acl->dstIp_ref(), true, "192.168.0.0/24");
    configureQualifier(acl->dscp_ref(), true, 0x24);
    configureQualifier(acl->ttl_ref(), true, ttl);
    configureQualifier(acl->proto_ref(), true, 6);
  }

  void configureIp6QualifiersHelper(cfg::AclEntry* acl) {
    cfg::Ttl ttl;
    std::tie(*ttl.value_ref(), *ttl.mask_ref()) = std::make_tuple(0x80, 0x80);

    configureQualifier(acl->ipType_ref(), true, cfg::IpType::IP6);
    configureQualifier(acl->srcIp_ref(), true, "::ffff:c0a8:1");
    configureQualifier(
        acl->dstIp_ref(), true, "2401:db00:3020:70e2:face:0:63:0/64");
    configureQualifier(acl->dscp_ref(), true, 0x24);
    configureQualifier(acl->ttl_ref(), true, ttl);
    configureQualifier(acl->proto_ref(), true, 6);
  }

  std::string kAclName() const {
    return "acl0";
  }

  void aclSetupHelper(bool isIpV4, bool isL3LookupClass) {
    auto newCfg = initialConfig();
    auto* acl = utility::addAcl(&newCfg, kAclName(), cfg::AclActionType::DENY);

    if (isIpV4) {
      configureIp4QualifiersHelper(acl);
    } else {
      configureIp6QualifiersHelper(acl);
    }

    if (isL3LookupClass) {
      configureQualifier(
          acl->lookupClass_ref(),
          true,
          cfg::AclLookupClass::DST_CLASS_L3_LOCAL_IP4);
    } else { // isL2LookupClass
      if (getPlatform()->getAsic()->getAsicType() !=
          HwAsic::AsicType::ASIC_TYPE_TRIDENT2) {
        configureQualifier(
            acl->lookupClassL2_ref(),
            true,
            cfg::AclLookupClass::CLASS_QUEUE_PER_HOST_QUEUE_1);
      }
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
    return utility::oneL3IntfConfig(getHwSwitch(), masterLogicalPortIds()[0]);
  }
};

TEST_F(HwAclQualifierTest, AclIp4TcpQualifiers) {
  auto setup = [=]() {
    auto newCfg = initialConfig();
    auto* acl1 = utility::addAcl(&newCfg, "ip4_tcp", cfg::AclActionType::DENY);
    configureAllHwQualifiers(acl1, true);
    configureAllL2QualifiersHelper(acl1);
    configureAllIpQualifiers(acl1, true, cfg::IpType::IP4);
    configureAllTcpQualifiers(acl1, true);
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
    configureAllIpQualifiers(acl1, true, cfg::IpType::IP6);
    configureAllTcpQualifiers(acl1, true);
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
    configureAllIcmpQualifiers(acl1, true, cfg::IpType::IP4);
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
    configureAllIcmpQualifiers(acl1, true, cfg::IpType::IP6);
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
    acl0->proto_ref() = 6;
    auto* acl1 = utility::addAcl(&newCfg, "acl1", cfg::AclActionType::DENY);
    acl1->proto_ref() = 58;
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
    configureAllIcmpQualifiers(acl, true, cfg::IpType::IP6);
    applyNewConfig(newCfg);
    // ip6 tcp
    configureAllIcmpQualifiers(acl, false, cfg::IpType::IP6);
    configureAllIpQualifiers(acl, true, cfg::IpType::IP6);
    applyNewConfig(newCfg);
    // imcp6
    configureAllIpQualifiers(acl, false, cfg::IpType::IP6);
    configureAllIcmpQualifiers(acl, true, cfg::IpType::IP6);
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
    acl->proto_ref() = 58;
    acl->icmpType_ref() = 3;
    acl->icmpCode_ref() = 8;
    applyNewConfig(newCfg);
    // change the rule to empty code icmp rule
    // Reserved for security(type=19 code is unset)
    acl->icmpType_ref() = 19;
    acl->icmpCode_ref().reset();
    applyNewConfig(newCfg);
  };

  auto verify = [=]() {
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

TEST_F(HwAclQualifierTest, AclIp4L2LookupClass) {
  auto setup = [=]() {
    aclSetupHelper(true /* isIpV4 */, false /* L2 LookupClass */);
  };

  auto verify = [=]() { aclVerifyHelper(); };

  verifyAcrossWarmBoots(setup, verify);
}

TEST_F(HwAclQualifierTest, AclIp4L3LookupClass) {
  auto setup = [=]() {
    aclSetupHelper(true /* isIpV4 */, true /* L3 LookupClass */);
  };

  auto verify = [=]() { aclVerifyHelper(); };

  verifyAcrossWarmBoots(setup, verify);
}

TEST_F(HwAclQualifierTest, AclIp6L2LookupClass) {
  auto setup = [=]() {
    aclSetupHelper(false /* isIpV6 */, false /* L2 lookupClass */);
  };

  auto verify = [=]() { aclVerifyHelper(); };

  verifyAcrossWarmBoots(setup, verify);
}

TEST_F(HwAclQualifierTest, AclIp6L3LookupClass) {
  auto setup = [=]() {
    aclSetupHelper(false /* isIpV6 */, true /* L3 LookupClass */);
  };

  auto verify = [=]() { aclVerifyHelper(); };

  verifyAcrossWarmBoots(setup, verify);
}

} // namespace facebook::fboss
