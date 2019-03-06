// Copyright 2004-present Facebook. All Rights Reserved.

#include <gtest/gtest.h>

#include "fboss/agent/test/ResourceLibUtil.h"

namespace facebook {
namespace fboss {

TEST(ResourceLibUtilTest, IPv4Generator) {
  auto generator = utility::IPAddressGenerator<folly::IPAddressV4>();
  std::array<folly::IPAddressV4, 5> ips = {
      folly::IPAddressV4("0.0.0.1"),
      folly::IPAddressV4("0.0.0.2"),
      folly::IPAddressV4("0.0.0.3"),
      folly::IPAddressV4("0.0.0.4"),
      folly::IPAddressV4("0.0.0.5"),
  };

  for (int i = 0; i < 5; i++) {
    auto generatedIp = generator.getNext();
    ASSERT_EQ(generatedIp, ips[i]);
  }
}

} // namespace fboss
} // namespace facebook
