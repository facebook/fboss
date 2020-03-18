// Copyright 2004-present Facebook. All Rights Reserved.

#include <gtest/gtest.h>

#include "fboss/agent/test/ResourceLibUtil.h"

namespace facebook::fboss {

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

TEST(ResourceLibUtilTest, MacGenerator) {
  auto generator = utility::MacAddressGenerator();
  std::array<folly::MacAddress, 5> macs{
      folly::MacAddress("00:00:00:00:00:01"),
      folly::MacAddress("00:00:00:00:00:02"),
      folly::MacAddress("00:00:00:00:00:03"),
      folly::MacAddress("00:00:00:00:00:04"),
      folly::MacAddress("00:00:00:00:00:05"),
  };

  for (int i = 0; i < 5; i++) {
    auto generatedMacs = generator.getNext();
    ASSERT_EQ(generatedMacs, macs[i]);
  }
}

TEST(ResourceLibUtilTest, HostPrefixV4Generator) {
  using IpT = folly::IPAddressV4;
  using RoutePrefixT = RoutePrefix<folly::IPAddressV4>;

  auto generator = utility::PrefixGenerator<IpT>(32);
  std::array<RoutePrefixT, 5> hostPrefixes = {
      RoutePrefixT{IpT("0.0.0.1"), 32},
      RoutePrefixT{IpT("0.0.0.2"), 32},
      RoutePrefixT{IpT("0.0.0.3"), 32},
      RoutePrefixT{IpT("0.0.0.4"), 32},
      RoutePrefixT{IpT("0.0.0.5"), 32},
  };

  for (int i = 0; i < 5; i++) {
    auto hostPrefix = generator.getNext();
    ASSERT_EQ(hostPrefix, hostPrefixes[i]);
  }
}

TEST(ResourceLibUtilTest, LpmPrefixV4Generator) {
  using IpT = folly::IPAddressV4;
  using RoutePrefixT = RoutePrefix<folly::IPAddressV4>;

  auto generator = utility::PrefixGenerator<IpT>(24);
  std::array<RoutePrefixT, 5> prefixes = {
      RoutePrefixT{IpT("0.0.1.0"), 24},
      RoutePrefixT{IpT("0.0.2.0"), 24},
      RoutePrefixT{IpT("0.0.3.0"), 24},
      RoutePrefixT{IpT("0.0.4.0"), 24},
      RoutePrefixT{IpT("0.0.5.0"), 24},
  };

  for (int i = 0; i < 5; i++) {
    auto prefix = generator.getNext();
    ASSERT_EQ(prefix, prefixes[i]);
  }
}

TEST(ResourceLibUtilTest, GenerateNv4Prefix) {
  using IpT = folly::IPAddressV4;
  using RoutePrefixT = RoutePrefix<folly::IPAddressV4>;

  auto generator = utility::PrefixGenerator<IpT>(24);
  std::vector<RoutePrefixT> generatedPrefixes = generator.getNextN(5);

  std::array<RoutePrefixT, 5> prefixes = {
      RoutePrefixT{IpT("0.0.1.0"), 24},
      RoutePrefixT{IpT("0.0.2.0"), 24},
      RoutePrefixT{IpT("0.0.3.0"), 24},
      RoutePrefixT{IpT("0.0.4.0"), 24},
      RoutePrefixT{IpT("0.0.5.0"), 24},
  };

  for (int i = 0; i < 5; i++) {
    ASSERT_EQ(generatedPrefixes[i], prefixes[i]);
  }
}

TEST(ResourceLibUtilTest, GenerateMask0V4) {
  using IpT = folly::IPAddressV4;
  using RoutePrefixT = RoutePrefix<folly::IPAddressV4>;

  auto generator = utility::PrefixGenerator<IpT>(0);
  EXPECT_EQ(generator.getNext(), (RoutePrefixT{IpT("0.0.0.0"), 0}));
}

TEST(ResourceLibUtilTest, GenerateMask0V6) {
  using IpT = folly::IPAddressV6;
  using RoutePrefixT = RoutePrefix<folly::IPAddressV6>;

  auto generator = utility::PrefixGenerator<IpT>(0);
  EXPECT_EQ(generator.getNext(), (RoutePrefixT{IpT("::"), 0}));
}

TEST(ResourceLibUtilTest, GenerateResetGenerateV4) {
  using IpT = folly::IPAddressV4;
  using RoutePrefixT = RoutePrefix<folly::IPAddressV4>;

  auto generator = utility::PrefixGenerator<IpT>(24);
  generator.getNextN(5);

  EXPECT_EQ(generator.getCursorPosition(), 5);
  generator.startOver(1);
  EXPECT_EQ(generator.getCursorPosition(), 1);
  auto expectedPrefix = RoutePrefixT{IpT("0.0.2.0"), 24};
  EXPECT_EQ(generator.getNext(), expectedPrefix);
  EXPECT_EQ(generator.getCursorPosition(), 2);
}

TEST(ResourceLibUtilTest, IPv6Generator) {
  auto generator = utility::IPAddressGenerator<folly::IPAddressV6>();
  std::array<folly::IPAddressV6, 5> ips = {
      folly::IPAddressV6("::1"),
      folly::IPAddressV6("::2"),
      folly::IPAddressV6("::3"),
      folly::IPAddressV6("::4"),
      folly::IPAddressV6("::5"),
  };

  for (int i = 0; i < 5; i++) {
    auto generatedIp = generator.getNext();
    EXPECT_EQ(generatedIp, ips[i]);
  }
  auto pos = generator.getCursorPosition();
  EXPECT_EQ(pos.second, 5);
  EXPECT_EQ(pos.first, 0);
}

TEST(ResourceLibUtilTest, HostPrefixV6Generator) {
  using IpT = folly::IPAddressV6;
  using RoutePrefixT = RoutePrefix<folly::IPAddressV6>;

  auto generator = utility::PrefixGenerator<IpT>(128);
  std::array<RoutePrefixT, 5> hostPrefixes = {
      RoutePrefixT{IpT("::1"), 128},
      RoutePrefixT{IpT("::2"), 128},
      RoutePrefixT{IpT("::3"), 128},
      RoutePrefixT{IpT("::4"), 128},
      RoutePrefixT{IpT("::5"), 128},
  };

  for (int i = 0; i < 5; i++) {
    auto hostPrefix = generator.getNext();
    ASSERT_EQ(hostPrefix, hostPrefixes[i]);
  }
}

TEST(ResourceLibUtilTest, LpmPrefixV6Generator) {
  using IpT = folly::IPAddressV6;
  using RoutePrefixT = RoutePrefix<folly::IPAddressV6>;

  auto generator = utility::PrefixGenerator<IpT>(64);
  std::array<RoutePrefixT, 5> prefixes = {
      RoutePrefixT{IpT("0:0:0:1::"), 64},
      RoutePrefixT{IpT("0:0:0:2::"), 64},
      RoutePrefixT{IpT("0:0:0:3::"), 64},
      RoutePrefixT{IpT("0:0:0:4::"), 64},
      RoutePrefixT{IpT("0:0:0:5::"), 64},
  };

  for (int i = 0; i < 5; i++) {
    auto prefix = generator.getNext();
    EXPECT_EQ(prefix.str(), prefixes[i].str());
  }
}

TEST(ResourceLibUtilTest, GenerateNv6Prefix) {
  using IpT = folly::IPAddressV6;
  using RoutePrefixT = RoutePrefix<folly::IPAddressV6>;

  auto generator = utility::PrefixGenerator<IpT>(64);
  std::vector<RoutePrefixT> generatedPrefixes = generator.getNextN(5);

  std::array<RoutePrefixT, 5> prefixes = {
      RoutePrefixT{IpT("0:0:0:1::"), 64},
      RoutePrefixT{IpT("0:0:0:2::"), 64},
      RoutePrefixT{IpT("0:0:0:3::"), 64},
      RoutePrefixT{IpT("0:0:0:4::"), 64},
      RoutePrefixT{IpT("0:0:0:5::"), 64},
  };

  for (int i = 0; i < 5; i++) {
    ASSERT_EQ(generatedPrefixes[i], prefixes[i]);
  }
}

TEST(ResourceLibUtilTest, GenerateResetGenerateV6) {
  using IpT = folly::IPAddressV6;
  using RoutePrefixT = RoutePrefix<folly::IPAddressV6>;

  auto generator = utility::PrefixGenerator<IpT>(64);
  generator.getNextN(5);

  EXPECT_EQ(generator.getCursorPosition(), utility::IdV6(0, 5));
  generator.startOver(utility::IdV6(0, 1));
  EXPECT_EQ(generator.getCursorPosition(), utility::IdV6(0, 1));
  auto expectedPrefix = RoutePrefixT{IpT("0:0:0:2::"), 64};
  EXPECT_EQ(generator.getNext(), expectedPrefix);
  EXPECT_EQ(generator.getCursorPosition(), utility::IdV6(0, 2));
}

TEST(ResourceLibUtilTest, RouteV4Generator) {
  using IpT = folly::IPAddressV4;
  using RoutePrefixT = RoutePrefix<folly::IPAddressV4>;

  auto generator = utility::RouteGenerator<folly::IPAddressV4>(
      24,
      AdminDistance::STATIC_ROUTE,
      {{InterfaceID(7), {folly::IPAddressV4("252.252.252.252")}}});
  std::array<RoutePrefixT, 5> prefixes = {
      RoutePrefixT{IpT("0.0.1.0"), 24},
      RoutePrefixT{IpT("0.0.2.0"), 24},
      RoutePrefixT{IpT("0.0.3.0"), 24},
      RoutePrefixT{IpT("0.0.4.0"), 24},
      RoutePrefixT{IpT("0.0.5.0"), 24},
  };

  for (int i = 0; i < 5; i++) {
    auto generatedRoute = generator.getNext();
    ASSERT_EQ(generatedRoute->prefix(), prefixes[i]);
  }
}
TEST(ResourceLibUtilTest, RouteV6Generator) {
  using IpT = folly::IPAddressV6;
  using RoutePrefixT = RoutePrefix<folly::IPAddressV6>;

  auto generator = utility::RouteGenerator<folly::IPAddressV6>(
      64,
      AdminDistance::STATIC_ROUTE,
      {{InterfaceID(7), {folly::IPAddressV6("2401:2881::")}}});
  std::array<RoutePrefixT, 5> prefixes = {
      RoutePrefixT{IpT("0:0:0:1::"), 64},
      RoutePrefixT{IpT("0:0:0:2::"), 64},
      RoutePrefixT{IpT("0:0:0:3::"), 64},
      RoutePrefixT{IpT("0:0:0:4::"), 64},
      RoutePrefixT{IpT("0:0:0:5::"), 64},
  };

  for (int i = 0; i < 5; i++) {
    auto generatedRoute = generator.getNext();
    ASSERT_EQ(generatedRoute->prefix(), prefixes[i]);
  }
}

} // namespace facebook::fboss
