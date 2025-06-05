/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/lib/ThriftMethodRateLimit.h"

#include <gtest/gtest.h>
#include <thrift/lib/cpp2/server/Cpp2ConnContext.h>
#include <thrift/lib/cpp2/server/PreprocessFunctions.h>
#include <thrift/lib/cpp2/server/PreprocessResult.h>

using namespace facebook::fboss;

class ThriftMethodRateLimitTest : public ::testing::Test {
 protected:
  void SetUp() override {}
  void TearDown() override {}
};

TEST_F(ThriftMethodRateLimitTest, ConstructorTest) {
  std::map<std::string, int> methodLimits = {
      {"method1", 10},
      {"method2", 20},
  };

  ThriftMethodRateLimit rateLimiter(methodLimits);
  // Constructor doesn't expose internal state, so we'll test functionality
  // through the other methods
}

TEST_F(ThriftMethodRateLimitTest, IsQpsLimitExceededTest) {
  std::map<std::string, int> methodLimits = {
      {"method1", 10},
  };

  ThriftMethodRateLimit rateLimiter(methodLimits);

  // Method with limit should not be rate limited initially
  for (int i = 0; i < 10; i++) {
    EXPECT_FALSE(rateLimiter.isQpsLimitExceeded("method1"));
  }
  // 11th call should be rate limited
  EXPECT_TRUE(rateLimiter.isQpsLimitExceeded("method1"));

  // Unknown method should not be rate limited
  EXPECT_FALSE(rateLimiter.isQpsLimitExceeded("unknown_method"));
}

TEST_F(ThriftMethodRateLimitTest, RateLimitPreprocessFuncTest) {
  std::map<std::string, int> methodLimits = {
      {"method1", 1}, // Set a very low limit for testing
  };

  auto rateLimiter = std::make_unique<ThriftMethodRateLimit>(methodLimits);
  auto preprocessFunc =
      ThriftMethodRateLimit::getThriftMethodRateLimitPreprocessFunc(
          std::move(rateLimiter));

  // Create test params
  apache::thrift::transport::THeader::StringToStringMap headers;
  std::string methodName = "method1";
  apache::thrift::Cpp2ConnContext connContext;
  apache::thrift::server::PreprocessParams params(
      headers, methodName, connContext);

  // First call should succeed
  auto result1 = preprocessFunc(params);
  EXPECT_TRUE(std::holds_alternative<std::monostate>(result1));

  // Subsequent calls should be rate limited and throw an exception
  auto result2 = preprocessFunc(params);
  EXPECT_FALSE(std::holds_alternative<std::monostate>(result2));

  // Test with unknown method (should be allowed)
  apache::thrift::transport::THeader::StringToStringMap headers2;
  std::string unknownMethod = "unknown_method";
  apache::thrift::server::PreprocessParams params2(
      headers2, unknownMethod, connContext);
  auto result3 = preprocessFunc(params2);
  EXPECT_TRUE(std::holds_alternative<std::monostate>(result3));
}

TEST_F(ThriftMethodRateLimitTest, MultipleMethodsTest) {
  std::map<std::string, int> methodLimits = {
      {"method1", 5},
      {"method2", 10},
  };

  auto rateLimiter = std::make_unique<ThriftMethodRateLimit>(methodLimits);
  auto preprocessFunc =
      ThriftMethodRateLimit::getThriftMethodRateLimitPreprocessFunc(
          std::move(rateLimiter));

  // Test method1 with limit of 5
  apache::thrift::transport::THeader::StringToStringMap headers1;
  std::string methodName1 = "method1";
  apache::thrift::Cpp2ConnContext connContext1;
  apache::thrift::server::PreprocessParams params1(
      headers1, methodName1, connContext1);

  // First 4 calls should succeed
  for (int i = 0; i < 4; i++) {
    auto result = preprocessFunc(params1);
    EXPECT_TRUE(std::holds_alternative<std::monostate>(result));
  }

  // Test method2 with limit of 10
  apache::thrift::transport::THeader::StringToStringMap headers2;
  std::string methodName2 = "method2";
  apache::thrift::Cpp2ConnContext connContext2;
  apache::thrift::server::PreprocessParams params2(
      headers2, methodName2, connContext2);

  // First 9 call should succeed
  for (int i = 0; i < 9; i++) {
    auto result = preprocessFunc(params2);
    EXPECT_TRUE(std::holds_alternative<std::monostate>(result));
  }

  // Verify that limits are separate for different methods
  // Call method1 again - should still be allowed since we're testing different
  // methods
  // 6th call should get exception
  auto result1 = preprocessFunc(params1);
  EXPECT_TRUE(std::holds_alternative<std::monostate>(result1));
  auto result2 = preprocessFunc(params2);
  EXPECT_TRUE(std::holds_alternative<std::monostate>(result2));

  // Now both thrift calls should got rate limit exception
  auto result3 = preprocessFunc(params1);
  EXPECT_FALSE(std::holds_alternative<std::monostate>(result3));
  auto result4 = preprocessFunc(params2);
  EXPECT_FALSE(std::holds_alternative<std::monostate>(result4));
}
