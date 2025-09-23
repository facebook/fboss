/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

/**
 * @file TunManagerRuleProcessorTest.cpp
 * @brief Unit tests for TunManager's ruleProcessor functionality
 * @author Ashutosh Grewal (agrewal@meta.com)
 */

/**
 * @def TUNMANAGER_RULE_PROCESSOR_FRIEND_TESTS
 * @brief Macro defining friend declarations for TunManager rule processor
 * tests
 *
 * This macro contains all the friend class and FRIEND_TEST declarations needed
 * to access private members of TunManager for testing the ruleProcessor
 * functionality.
 */
#define TUNMANAGER_RULE_PROCESSOR_FRIEND_TESTS                      \
  friend class TunManagerRuleProcessorTest;                         \
  FRIEND_TEST(TunManagerRuleProcessorTest, ProcessIPv4SourceRule);  \
  FRIEND_TEST(TunManagerRuleProcessorTest, ProcessIPv6SourceRule);  \
  FRIEND_TEST(TunManagerRuleProcessorTest, SkipUnsupportedFamily);  \
  FRIEND_TEST(TunManagerRuleProcessorTest, SkipInvalidTableId);     \
  FRIEND_TEST(TunManagerRuleProcessorTest, SkipRuleWithoutSrcAddr); \
  FRIEND_TEST(TunManagerRuleProcessorTest, SkipLinkLocalAddress);   \
  FRIEND_TEST(                                                      \
      TunManagerRuleProcessorTest, BuildIfIndexToTableIdMapFromRulesEmpty);
#include <gmock/gmock.h>
#include <gtest/gtest.h>

extern "C" {
#include <linux/rtnetlink.h>
#include <netlink/addr.h>
#include <netlink/object.h>
#include <netlink/route/rule.h>
}

#include <folly/IPAddress.h>

#include "fboss/agent/SwSwitch.h"
#include "fboss/agent/TunManager.h"
#include "fboss/agent/test/MockTunIntf.h"
#include "fboss/agent/test/MockTunManager.h"
#include "fboss/agent/test/TestUtils.h"

using namespace facebook::fboss;
using ::testing::_;

namespace facebook::fboss {

/**
 * @class TunManagerRuleProcessorTest
 * @brief Test fixture for TunManager::ruleProcessor functionality
 */
class TunManagerRuleProcessorTest : public ::testing::Test {
 public:
  /**
   * @brief Set up test environment before each test
   *
   * Creates mock platform, switch, and TUN manager instances.
   */
  void SetUp() override {
    platform_ = createMockPlatform();
    sw_ = setupMockSwitchWithoutHW(
        platform_.get(), nullptr, SwitchFlags::ENABLE_TUN);
    tunMgr_ = dynamic_cast<MockTunManager*>(sw_->getTunManager());
    ASSERT_NE(nullptr, tunMgr_);
  }

  /**
   * @brief Clean up test environment after each test
   *
   * Properly destroys mock objects in correct order to avoid memory leaks.
   */
  void TearDown() override {
    // Reset in proper order: sw_ first, then platform_
    sw_.reset();
    platform_.reset();
  }

 protected:
  std::unique_ptr<MockPlatform> platform_; ///< Mock platform instance
  std::unique_ptr<SwSwitch> sw_; ///< Mock switch instance
  MockTunManager* tunMgr_{}; ///< Mock TUN manager for testing

  /**
   * @brief Helper function to create a netlink rule object
   *
   * @param family Address family (AF_INET or AF_INET6)
   * @param tableId Routing table ID
   * @param srcAddr Source address string
   * @param prefixLen Prefix length
   * @return Allocated netlink rule object
   */
  struct rtnl_rule* createRule(
      int family,
      int tableId,
      const std::string& srcAddr,
      int prefixLen) {
    auto rule = rtnl_rule_alloc();
    EXPECT_NE(nullptr, rule);

    rtnl_rule_set_family(rule, family);
    rtnl_rule_set_table(rule, tableId);
    rtnl_rule_set_action(rule, FR_ACT_TO_TBL);

    // Use Folly to parse and convert the IP address
    auto ipAddr = folly::IPAddress(srcAddr);

    // Verify the address family matches
    if (family == AF_INET) {
      EXPECT_TRUE(ipAddr.isV4());
    } else if (family == AF_INET6) {
      EXPECT_TRUE(ipAddr.isV6());
    }

    // Get binary representation
    auto addrBytes = ipAddr.bytes();
    size_t addrSize = ipAddr.isV4() ? 4 : 16;

    // Set source address
    auto sourceaddr = nl_addr_build(family, addrBytes, addrSize);
    EXPECT_NE(nullptr, sourceaddr);

    nl_addr_set_prefixlen(sourceaddr, prefixLen);
    rtnl_rule_set_src(rule, sourceaddr);

    nl_addr_put(sourceaddr);

    return rule;
  }

  /**
   * @brief Helper function to verify stored rule
   *
   * @param expectedFamily Expected address family
   * @param expectedTableId Expected table ID
   * @param expectedSrcAddr Expected source address string
   */
  void verifyStoredRule(
      int expectedFamily,
      int expectedTableId,
      const std::string& expectedSrcAddr) {
    ASSERT_EQ(1, tunMgr_->probedRules_.size());

    const auto& probedRule = tunMgr_->probedRules_[0];
    EXPECT_EQ(expectedFamily, probedRule.family);
    EXPECT_EQ(expectedTableId, probedRule.tableId);
    EXPECT_EQ(expectedSrcAddr, probedRule.srcAddr);
  }
};

/**
 * @brief Test processing of IPv4 source routing rule
 *
 * Verifies that ruleProcessor correctly processes and stores an IPv4 source
 * routing rule with valid parameters. The test creates a real netlink rule
 * object with:
 * - IPv4 address family (AF_INET)
 * - Valid table ID (100, within range [1-253])
 * - Source address with /32 prefix
 *
 * Expects the rule to be stored in probedRules_ with all fields correctly
 * extracted and parsed.
 */
TEST_F(TunManagerRuleProcessorTest, ProcessIPv4SourceRule) {
  auto rule = createRule(AF_INET, 100, "192.168.1.1", 32);
  ASSERT_NE(nullptr, rule);

  // Call the actual ruleProcessor function
  TunManager::ruleProcessor(
      reinterpret_cast<struct nl_object*>(rule), static_cast<void*>(tunMgr_));

  // Verify the rule was stored by ruleProcessor
  verifyStoredRule(AF_INET, 100, "192.168.1.1/32");

  // Cleanup
  rtnl_rule_put(rule);
}

/**
 * @brief Test processing of IPv6 source routing rule
 *
 * Verifies that ruleProcessor correctly processes and stores an IPv6 source
 * routing rule with valid parameters. The test creates a real netlink rule
 * object with:
 * - IPv6 address family (AF_INET6)
 * - Valid table ID (150, within range [1-253])
 * - Source address with /128 prefix
 *
 * Expects the rule to be stored in probedRules_ with all fields correctly
 * extracted and parsed.
 */
TEST_F(TunManagerRuleProcessorTest, ProcessIPv6SourceRule) {
  auto rule = createRule(AF_INET6, 150, "2001:db8::1", 128);
  ASSERT_NE(nullptr, rule);

  // Call the actual ruleProcessor function
  TunManager::ruleProcessor(
      reinterpret_cast<struct nl_object*>(rule), static_cast<void*>(tunMgr_));

  // Verify the rule was stored by ruleProcessor
  verifyStoredRule(AF_INET6, 150, "2001:db8::1/128");

  // Cleanup
  rtnl_rule_put(rule);
}

/**
 * @brief Test skipping rules with unsupported address family
 *
 * Verifies that ruleProcessor correctly skips rules with unsupported address
 * family (e.g., AF_PACKET) and does not store them in probedRules_.
 */
TEST_F(TunManagerRuleProcessorTest, SkipUnsupportedFamily) {
  auto rule = rtnl_rule_alloc();
  ASSERT_NE(nullptr, rule);

  // Set unsupported family
  rtnl_rule_set_family(rule, AF_PACKET);
  rtnl_rule_set_table(rule, 100);

  // Call the actual ruleProcessor function
  TunManager::ruleProcessor(
      reinterpret_cast<struct nl_object*>(rule), static_cast<void*>(tunMgr_));

  // Verify no rule was stored
  EXPECT_EQ(0, tunMgr_->probedRules_.size());

  // Cleanup
  rtnl_rule_put(rule);
}

/**
 * @brief Test skipping rules with invalid table ID
 *
 * Verifies that ruleProcessor correctly skips rules with table ID outside
 * the valid range [1-253] and does not store them in probedRules_.
 */
TEST_F(TunManagerRuleProcessorTest, SkipInvalidTableId) {
  auto rule = rtnl_rule_alloc();
  ASSERT_NE(nullptr, rule);

  rtnl_rule_set_family(rule, AF_INET);
  rtnl_rule_set_table(rule, 254); // Outside valid range [1-253]

  // Call the actual ruleProcessor function
  TunManager::ruleProcessor(
      reinterpret_cast<struct nl_object*>(rule), static_cast<void*>(tunMgr_));

  // Verify no rule was stored
  EXPECT_EQ(0, tunMgr_->probedRules_.size());

  // Cleanup
  rtnl_rule_put(rule);
}

/**
 * @brief Test skipping rules without source address
 *
 * Verifies that ruleProcessor correctly skips rules that don't have a source
 * address set and does not store them in probedRules_.
 */
TEST_F(TunManagerRuleProcessorTest, SkipRuleWithoutSrcAddr) {
  auto rule = rtnl_rule_alloc();
  ASSERT_NE(nullptr, rule);

  rtnl_rule_set_family(rule, AF_INET);
  rtnl_rule_set_table(rule, 100);
  // Intentionally not setting source address

  // Call the actual ruleProcessor function
  TunManager::ruleProcessor(
      reinterpret_cast<struct nl_object*>(rule), static_cast<void*>(tunMgr_));

  // Verify no rule was stored
  EXPECT_EQ(0, tunMgr_->probedRules_.size());

  // Cleanup
  rtnl_rule_put(rule);
}

/**
 * @brief Test skipping rules with link-local addresses
 *
 * Verifies that ruleProcessor correctly skips rules with link-local source
 * addresses since they don't have source routing rules in our implementation.
 */
TEST_F(TunManagerRuleProcessorTest, SkipLinkLocalAddress) {
  // Test IPv4 link-local (169.254.x.x)
  auto ipv4Rule = createRule(AF_INET, 100, "169.254.1.1", 32);
  ASSERT_NE(nullptr, ipv4Rule);

  TunManager::ruleProcessor(
      reinterpret_cast<struct nl_object*>(ipv4Rule),
      static_cast<void*>(tunMgr_));

  // Verify no rule was stored for IPv4 link-local
  EXPECT_EQ(0, tunMgr_->probedRules_.size());

  rtnl_rule_put(ipv4Rule);

  // Test IPv6 link-local (fe80::/10)
  auto ipv6Rule = createRule(AF_INET6, 100, "fe80::1", 128);
  ASSERT_NE(nullptr, ipv6Rule);

  TunManager::ruleProcessor(
      reinterpret_cast<struct nl_object*>(ipv6Rule),
      static_cast<void*>(tunMgr_));

  // Verify no rule was stored for IPv6 link-local
  EXPECT_EQ(0, tunMgr_->probedRules_.size());

  rtnl_rule_put(ipv6Rule);
}

/**
 * @brief Test empty interfaces map for buildIfIndexToTableIdMapFromRules
 *
 * Verifies that buildIfIndexToTableIdMapFromRules returns an empty map when
 * there are no interfaces configured.
 */
TEST_F(TunManagerRuleProcessorTest, BuildIfIndexToTableIdMapFromRulesEmpty) {
  // Clear any existing interfaces and rules
  tunMgr_->intfs_.clear();
  tunMgr_->probedRules_.clear();

  auto result = tunMgr_->buildIfIndexToTableIdMapFromRules();

  EXPECT_TRUE(result.empty());
}

} // namespace facebook::fboss
