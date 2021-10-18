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

#include "fboss/agent/AddressUtil.h"
#include "fboss/agent/Utils.h"

#include <folly/IPAddress.h>
#include "common/network/if/gen-cpp2/Address_types.h"

#include <string>

using namespace facebook::network;
using namespace facebook::network::thrift;
using folly::IPAddress;
using folly::IPAddressV4;
using folly::IPAddressV6;

namespace {
IPAddress kV4Ip("10.0.0.0");
IPAddress kV6Ip("a::");
fbbinary kV4Binary{10, 0, 0, 0};
fbbinary kV6Binary{0, 0xa, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
std::string kV4Str{"10.0.0.0"};
std::string kV6Str{"000a:0000:0000:0000:0000:0000:0000:0000"};
} // namespace

TEST(AddressUtilTests, ToBinaryAddressV4) {
  auto thriftBinary = toBinaryAddress(kV4Ip);
  EXPECT_EQ(kV4Binary, *thriftBinary.addr_ref());
  EXPECT_FALSE(thriftBinary.port_ref());
  EXPECT_FALSE(thriftBinary.ifName_ref());
}

TEST(AddressUtilTests, ToBinaryAddressV6) {
  auto thriftBinary = toBinaryAddress(kV6Ip);
  EXPECT_EQ(kV6Binary, *thriftBinary.addr_ref());
  EXPECT_FALSE(thriftBinary.port_ref());
  EXPECT_FALSE(thriftBinary.ifName_ref());
}

TEST(AddressUtilTests, fromBinaryAddressV4) {
  BinaryAddress thriftBinary;
  *thriftBinary.addr_ref() = kV4Binary;
  EXPECT_EQ(kV4Ip, toIPAddress(thriftBinary));
}

TEST(AddressUtilTests, fromBinaryAddressV6) {
  BinaryAddress thriftBinary;
  *thriftBinary.addr_ref() = kV6Binary;
  EXPECT_EQ(kV6Ip, toIPAddress(thriftBinary));
}

TEST(AddressUtilTests, fromBinaryAddressInvalidBinary) {
  BinaryAddress thriftBinary;
  *thriftBinary.addr_ref() = {0};
  EXPECT_THROW(toIPAddress(thriftBinary), folly::IPAddressFormatException);
}

TEST(AddressUtilTests, ToThriftAddressV4) {
  auto thriftAddr = toAddress(kV4Ip);
  EXPECT_EQ(kV4Str, *thriftAddr.addr_ref());
  EXPECT_EQ(AddressType::V4, *thriftAddr.type_ref());
  EXPECT_FALSE(thriftAddr.port_ref());
}

TEST(AddressUtilTests, ToThriftAddressV6) {
  auto thriftAddr = toAddress(kV6Ip);
  EXPECT_EQ(kV6Str, *thriftAddr.addr_ref());
  EXPECT_EQ(AddressType::V6, *thriftAddr.type_ref());
  EXPECT_FALSE(thriftAddr.port_ref());
}

TEST(AddressUtilTests, fromThriftAddressV4) {
  thrift::Address thriftAddr;
  *thriftAddr.addr_ref() = kV4Str;
  *thriftAddr.type_ref() = AddressType::V4;
  EXPECT_EQ(kV4Ip, toIPAddress(thriftAddr));
}

TEST(AddressUtilTests, fromThriftAddressV6) {
  thrift::Address thriftAddr;
  *thriftAddr.addr_ref() = kV6Str;
  *thriftAddr.type_ref() = AddressType::V6;
  EXPECT_EQ(kV6Ip, toIPAddress(thriftAddr));
}

TEST(AddressUtilTests, toIpPrefixV6) {
  auto ipPfx = facebook::fboss::toIpPrefix(folly::CIDRNetwork{kV6Ip, 64});
  EXPECT_EQ(kV6Ip, toIPAddress(*ipPfx.ip_ref()));
  EXPECT_EQ(64, ipPfx.prefixLength_ref());
}

TEST(AddressUtilTests, toIpPrefixV4) {
  auto ipPfx = facebook::fboss::toIpPrefix(folly::CIDRNetwork{kV4Ip, 24});
  EXPECT_EQ(kV4Ip, toIPAddress(*ipPfx.ip_ref()));
  EXPECT_EQ(24, ipPfx.prefixLength_ref());
}
