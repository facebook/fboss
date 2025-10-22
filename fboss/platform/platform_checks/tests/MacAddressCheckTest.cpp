/*
 *  Copyright (c) 2004-present, Meta Platforms, Inc. and affiliates.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/platform/platform_checks/checks/MacAddressCheck.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace ::testing;
using namespace facebook::fboss::platform::platform_checks;

class MockMacAddressCheck : public MacAddressCheck {
 public:
  MOCK_METHOD(
      folly::MacAddress,
      getMacAddress,
      (const std::string& interface),
      ());

  MOCK_METHOD(folly::MacAddress, getEepromMacAddress, (), ());
};

class MacAddressCheckTest : public ::testing::Test {
 protected:
  void SetUp() override {
    check_ = std::make_unique<MockMacAddressCheck>();
  }

  std::unique_ptr<MockMacAddressCheck> check_;
};

TEST_F(MacAddressCheckTest, MatchingAddresses) {
  folly::MacAddress eth0Mac("01:02:03:04:05:06");
  EXPECT_CALL(*check_, getMacAddress("eth0")).WillRepeatedly(Return(eth0Mac));
  EXPECT_CALL(*check_, getEepromMacAddress()).WillRepeatedly(Return(eth0Mac));

  auto result = check_->run();

  EXPECT_EQ(result.checkType(), CheckType::MAC_ADDRESS_CHECK);
  EXPECT_EQ(result.status(), CheckStatus::OK);
}

TEST_F(MacAddressCheckTest, MismatchedAddresses) {
  folly::MacAddress eth0Mac("01:02:03:04:05:06");
  folly::MacAddress eth0Mac2("00:02:03:04:05:06");
  EXPECT_CALL(*check_, getMacAddress("eth0")).WillRepeatedly(Return(eth0Mac));
  EXPECT_CALL(*check_, getEepromMacAddress()).WillRepeatedly(Return(eth0Mac2));

  auto result = check_->run();

  EXPECT_EQ(result.checkType(), CheckType::MAC_ADDRESS_CHECK);
  EXPECT_EQ(result.status(), CheckStatus::PROBLEM);
}

TEST_F(MacAddressCheckTest, ZeroAddress) {
  folly::MacAddress eth0Mac("01:02:03:04:05:06");
  EXPECT_CALL(*check_, getMacAddress("eth0"))
      .WillRepeatedly(Return(folly::MacAddress::ZERO));
  EXPECT_CALL(*check_, getEepromMacAddress()).WillRepeatedly(Return(eth0Mac));

  auto result = check_->run();

  EXPECT_EQ(result.checkType(), CheckType::MAC_ADDRESS_CHECK);
  EXPECT_EQ(result.status(), CheckStatus::ERROR);
}
