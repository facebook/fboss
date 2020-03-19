/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include <gtest/gtest.h>

#include <optional>
#include <string>
#include <tuple>
#include <variant>
#include "fboss/agent/hw/sai/api/LoggingUtil.h"
#include "fboss/agent/hw/sai/api/RouteApi.h"
#include "fboss/agent/hw/sai/api/RouterInterfaceApi.h"

#include <fmt/format.h>
#include <fmt/ranges.h>

using namespace facebook::fboss;

TEST(LoggingUtilTest, variant) {
  std::variant<int, std::string> v{42};
  EXPECT_EQ("42", fmt::format("{}", v));
}

TEST(LoggingUtilTest, variantRecursive) {
  SaiRouterInterfaceTraits::AdapterKey rifKey{42};
  folly::CIDRNetwork prefix("42.42.42.0", 24);
  SaiRouteTraits::AdapterKey routeKey{0, 0, prefix};
  std::variant<SaiRouterInterfaceTraits::AdapterKey, SaiRouteTraits::AdapterKey>
      vRif{rifKey};
  EXPECT_EQ("RouterInterfaceSaiId(42)", fmt::format("{}", vRif));
  std::variant<SaiRouterInterfaceTraits::AdapterKey, SaiRouteTraits::AdapterKey>
      vRoute{routeKey};
  EXPECT_EQ(
      "RouteEntry(switch: 0, vrf: 0, prefix: 42.42.42.0/24)",
      fmt::format("{}", vRoute));
}

TEST(LoggingUtilTest, tuple) {
  std::tuple<int, std::string, double> t{42, "forty-two", 42.0};
  EXPECT_EQ("(42, \"forty-two\", 42.0)", fmt::format("{}", t));
}

TEST(LoggingUtilTest, emptyTuple) {
  std::tuple<> t{};
  EXPECT_EQ("()", fmt::format("{}", t));
}

TEST(LoggingUtilTest, monostate) {
  std::monostate m;
  EXPECT_EQ("(monostate)", fmt::format("{}", m));
}

TEST(LoggingUtilTest, optional) {
  SaiRouteTraits::Attributes::PacketAction rpa{SAI_PACKET_ACTION_FORWARD};
  std::optional<SaiRouteTraits::Attributes::PacketAction> opt{rpa};
  EXPECT_EQ(fmt::format("{}", rpa), fmt::format("{}", opt));
}

TEST(LoggingUtilTest, emptyOptional) {
  std::optional<SaiRouteTraits::Attributes::PacketAction> opt{};
  EXPECT_EQ("nullopt", fmt::format("{}", opt));
}
