/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/hw/bcm/tests/BcmTest.h"

#include "fboss/agent/ApplyThriftConfig.h"
#include "fboss/agent/FbossError.h"
#include "fboss/agent/gen-cpp2/switch_config_types.h"
#include "fboss/agent/hw/bcm/BcmAclEntry.h"
#include "fboss/agent/hw/bcm/BcmAclTable.h"
#include "fboss/agent/hw/bcm/BcmFieldProcessorUtils.h"
#include "fboss/agent/hw/bcm/BcmStatUpdater.h"
#include "fboss/agent/hw/bcm/BcmSwitch.h"
#include "fboss/agent/hw/bcm/tests/BcmTestUtils.h"
#include "fboss/agent/platforms/tests/utils/BcmTestPlatform.h"
#include "fboss/agent/state/StateDelta.h"
#include "fboss/agent/state/SwitchState.h"

#include "fboss/agent/hw/test/ConfigFactory.h"

#include <boost/container/flat_map.hpp>
#include <string>

extern "C" {
#include <bcm/error.h>
#include <bcm/field.h>
}

DECLARE_int32(acl_gid);

namespace {

using namespace facebook::fboss;
using namespace facebook::fboss::utility;

bool aclEnabled(int unit, bcm_field_group_t group) {
  int enable = -1;
  auto rv = bcm_field_group_enable_get(unit, group, &enable);
  bcmCheckError(rv, "failed to get field group enable status");
  CHECK(enable == 0 || enable == 1);
  return (enable == 1);
}

template <typename Func, typename Param>
void checkEnumQualRemoved(
    Func getBcmQualifierFn,
    Param hwValue,
    BcmSwitch* hw,
    int unit,
    std::shared_ptr<SwitchState> state,
    const std::string& aclName) {
  auto swAcl = state->getAcl(aclName);
  ASSERT_NE(nullptr, swAcl);
  auto hwAcl = hw->getAclTable()->getAclIf(swAcl->getPriority());
  ASSERT_NE(nullptr, hwAcl);
  ASSERT_EQ(
      BCM_E_INTERNAL, getBcmQualifierFn(unit, hwAcl->getHandle(), hwValue));
}

cfg::AclEntry* addAcl(cfg::SwitchConfig* cfg, const std::string& aclName) {
  auto acl = cfg::AclEntry();
  acl.name = aclName;
  acl.actionType = cfg::AclActionType::DENY;
  cfg->acls.push_back(acl);
  return &cfg->acls.back();
}

void delAcl(cfg::SwitchConfig* cfg, const std::string& aclName) {
  auto& acls = cfg->acls;
  acls.erase(
      std::remove_if(
          acls.begin(),
          acls.end(),
          [&](cfg::AclEntry const& acl) { return acl.name == aclName; }),
      acls.end());
}

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
  std::tie(ttl.value, ttl.mask) = std::make_tuple(0x80, 0x80);

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
  configureQualifier(acl->dscp_ref(), enable, 0x42);
  configureQualifier(acl->ttl_ref(), enable, ttl);
}

void configureAllHwQualifiers(cfg::AclEntry* acl, bool enable) {
  configureQualifier(acl->srcPort_ref(), enable, 5);
  configureQualifier(acl->dstPort_ref(), enable, 8);
}

void configureAllTcpQualifiers(cfg::AclEntry* acl, bool enable) {
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

class BcmAclQualifierTest : public BcmTest {
 protected:
  cfg::SwitchConfig initialConfig() const override {
    return utility::oneL3IntfConfig(getHwSwitch(), masterLogicalPortIds()[0]);
  }
};

TEST_F(BcmAclQualifierTest, AclNoQualifier) {
  // While comparing a Acl's h/w and s/w state, we make certain
  // assumptions about what the SDK will return for unset fields.
  // These assumptions have been arrived at experimentally and could
  // possibly change in newer SDK versions. This test makes sure that
  // if that happens we get a test failure as a means of knowing that
  // we need to update our code.
  auto setup = [=]() {
    auto newCfg = initialConfig();
    addAcl(&newCfg, "acl0");
    applyNewConfig(newCfg);
  };

  auto verify = [=]() {
    ASSERT_TRUE(aclEnabled(getUnit(), FLAGS_acl_gid));
    EXPECT_EQ(fpGroupNumAclEntries(getUnit(), FLAGS_acl_gid), 1);
    checkSwHwAclMatch(getHwSwitch(), getProgrammedState(), "acl0");
  };

  verifyAcrossWarmBoots(setup, verify);
}

TEST_F(BcmAclQualifierTest, AclIp4TcpQualifiers) {
  auto setup = [=]() {
    auto newCfg = initialConfig();
    auto* acl1 = addAcl(&newCfg, "ip4_tcp");
    configureAllHwQualifiers(acl1, true);
    configureAllIpQualifiers(acl1, true, cfg::IpType::IP4);
    configureAllTcpQualifiers(acl1, true);
    applyNewConfig(newCfg);
  };

  auto verify = [=]() {
    ASSERT_TRUE(aclEnabled(getUnit(), FLAGS_acl_gid));
    EXPECT_EQ(fpGroupNumAclEntries(getUnit(), FLAGS_acl_gid), 1);
    checkSwHwAclMatch(getHwSwitch(), getProgrammedState(), "ip4_tcp");
  };

  verifyAcrossWarmBoots(setup, verify);
}

TEST_F(BcmAclQualifierTest, AclIp6TcpQualifiers) {
  auto setup = [=]() {
    auto newCfg = initialConfig();
    auto* acl1 = addAcl(&newCfg, "ip6_tcp");
    configureAllHwQualifiers(acl1, true);
    configureAllIpQualifiers(acl1, true, cfg::IpType::IP6);
    configureAllTcpQualifiers(acl1, true);
    applyNewConfig(newCfg);
  };

  auto verify = [=]() {
    ASSERT_TRUE(aclEnabled(getUnit(), FLAGS_acl_gid));
    EXPECT_EQ(fpGroupNumAclEntries(getUnit(), FLAGS_acl_gid), 1);
    checkSwHwAclMatch(getHwSwitch(), getProgrammedState(), "ip6_tcp");
  };

  verifyAcrossWarmBoots(setup, verify);
}

TEST_F(BcmAclQualifierTest, AclIcmp4Qualifiers) {
  auto setup = [=]() {
    auto newCfg = initialConfig();
    auto* acl1 = addAcl(&newCfg, "icmp4");
    configureAllHwQualifiers(acl1, true);
    configureAllIcmpQualifiers(acl1, true, cfg::IpType::IP4);
    applyNewConfig(newCfg);
  };

  auto verify = [=]() {
    ASSERT_TRUE(aclEnabled(getUnit(), FLAGS_acl_gid));
    EXPECT_EQ(fpGroupNumAclEntries(getUnit(), FLAGS_acl_gid), 1);
    checkSwHwAclMatch(getHwSwitch(), getProgrammedState(), "icmp4");
  };

  verifyAcrossWarmBoots(setup, verify);
}

TEST_F(BcmAclQualifierTest, AclIcmp6Qualifiers) {
  auto setup = [=]() {
    auto newCfg = initialConfig();
    auto* acl1 = addAcl(&newCfg, "icmp6");
    configureAllHwQualifiers(acl1, true);
    configureAllIcmpQualifiers(acl1, true, cfg::IpType::IP6);
    applyNewConfig(newCfg);
  };

  auto verify = [=]() {
    ASSERT_TRUE(aclEnabled(getUnit(), FLAGS_acl_gid));
    EXPECT_EQ(fpGroupNumAclEntries(getUnit(), FLAGS_acl_gid), 1);
    checkSwHwAclMatch(getHwSwitch(), getProgrammedState(), "icmp6");
  };

  verifyAcrossWarmBoots(setup, verify);
}

TEST_F(BcmAclQualifierTest, AclRemove) {
  auto setup = [=]() {
    auto newCfg = initialConfig();
    auto* acl0 = addAcl(&newCfg, "acl0");
    acl0->proto_ref() = 6;
    auto* acl1 = addAcl(&newCfg, "acl1");
    acl1->proto_ref() = 58;
    applyNewConfig(newCfg);

    delAcl(&newCfg, "acl0");
    applyNewConfig(newCfg);
  };

  auto verify = [=]() {
    ASSERT_TRUE(aclEnabled(getUnit(), FLAGS_acl_gid));
    EXPECT_EQ(fpGroupNumAclEntries(getUnit(), FLAGS_acl_gid), 1);
    checkSwHwAclMatch(getHwSwitch(), getProgrammedState(), "acl1");
  };

  verifyAcrossWarmBoots(setup, verify);
}

TEST_F(BcmAclQualifierTest, AclModifyQualifier) {
  auto setup = [=]() {
    auto newCfg = initialConfig();
    auto* acl = addAcl(&newCfg, "acl0");
    // icmp6
    configureAllHwQualifiers(acl, true);
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
    ASSERT_TRUE(aclEnabled(getUnit(), FLAGS_acl_gid));
    EXPECT_EQ(fpGroupNumAclEntries(getUnit(), FLAGS_acl_gid), 1);
    checkSwHwAclMatch(getHwSwitch(), getProgrammedState(), "acl0");
    bcm_field_IpFrag_t hwValueIpFrag{};
    checkEnumQualRemoved(
        bcm_field_qualify_IpFrag_get,
        &hwValueIpFrag,
        getHwSwitch(),
        getUnit(),
        getProgrammedState(),
        "acl0");
  };

  verifyAcrossWarmBoots(setup, verify);
}

TEST_F(BcmAclQualifierTest, AclEmptyCodeIcmp) {
  auto setup = [=]() {
    auto newCfg = initialConfig();
    auto* acl = addAcl(&newCfg, "acl0");
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
    EXPECT_EQ(fpGroupNumAclEntries(getUnit(), FLAGS_acl_gid), 1);
    checkSwHwAclMatch(getHwSwitch(), getProgrammedState(), "acl0");
  };

  verifyAcrossWarmBoots(setup, verify);
}

} // namespace facebook::fboss
