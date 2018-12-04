/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/hw/mock/MockRxPacket.h"
#include "fboss/agent/packet/NDP.h"

#include <folly/io/Cursor.h>
#include <gtest/gtest.h>

using namespace facebook::fboss;
using folly::MacAddress;
using folly::io::Cursor;

namespace {
NDPOptions parseNdpOptions(const std::string& pktHex) {
  auto ndp = MockRxPacket::fromHex(pktHex);
  Cursor cursor(ndp->buf());
  return NDPOptions(cursor);
}
} // namespace

TEST(NDPOptionsTest, NDPOptionTooSmall) {
  EXPECT_THROW(
      parseNdpOptions("05" // type: mtu
                      "01" // length (1 * 8 bytes)
                           // no payload (6 bytes missing)
                      ),
      HdrParseError);
}

TEST(NDPOptionsTest, MtuOptionValid) {
  auto ndpOpts = parseNdpOptions(
      "05" // type: mtu
      "01" // length (1 * 8 bytes)
      "0000" // res
      "00000500" // mtu (1280)
  );
  EXPECT_TRUE(ndpOpts.mtu);
  EXPECT_EQ(1280, *ndpOpts.mtu);
}

TEST(NDPOptionsTest, MtuOptionResFieldIgnored) {
  auto ndpOpts = parseNdpOptions(
      "05" // type: mtu
      "01" // length (1 * 8 bytes)
      "1234" // res (1234)
      "00000500" // mtu (1280)
  );
  EXPECT_TRUE(ndpOpts.mtu);
  EXPECT_EQ(1280, *ndpOpts.mtu);
}

TEST(NDPOptionsTest, MtuOptionInvalidLength) {
  EXPECT_THROW(
      parseNdpOptions("05" // type: mtu
                      "02" // invalid length (expected 1)
                      "0000" // res (1234)
                      "00000500" // mtu (1280)
                      ),
      HdrParseError);
}

TEST(NDPOptionsTest, SrcLLAdrOptionValid) {
  auto ndpOpts = parseNdpOptions(
      // Option 1
      "01" // Type: Source Link-Layer Address
      "01" // Length (1 * 8 bytes)
      "0123456789ab" // SrcLLAaddr (ethernet addr)
  );
  EXPECT_TRUE(ndpOpts.sourceLinkLayerAddress);
  EXPECT_EQ(MacAddress("01:23:45:67:89:ab"), *ndpOpts.sourceLinkLayerAddress);
}

TEST(NDPOptionsTest, SrcLLAdrOptionInvalidLength) {
  EXPECT_THROW(
      parseNdpOptions(
          // Option 1
          "01" // Type: Source Link-Layer Address
          "02" // invalid length (expected 1)
          "0123456789ab" // SrcLLAaddr (ethernet addr)
          ),
      HdrParseError);
}

TEST(NDPOptionsTest, UnsupportedOption) {
  auto ndpOpts = parseNdpOptions(
      "02" // Type: Target Link-layer Address
      "01" // Length (1 * 8 bytes)
      "0123456789ab" // TgtLLAaddr (ethernet addr)
  );
  EXPECT_FALSE(ndpOpts.sourceLinkLayerAddress);
  EXPECT_FALSE(ndpOpts.mtu);
}

TEST(NDPOptionsTest, MultipleSupportedOptions) {
  auto ndpOpts = parseNdpOptions(
      // Option 1
      "01" // Type: Source Link-Layer Address
      "01" // Length (1 * 8 bytes)
      "0123456789ab" // SrcLLAaddr (ethernet addr)
      // Option 2
      "05" // Type: MTU
      "01" // Length (1 * 8 bytes)
      "0000" // res (0)
      "00000500" // mtu (1280)
  );
  EXPECT_TRUE(ndpOpts.sourceLinkLayerAddress);
  EXPECT_EQ(MacAddress("01:23:45:67:89:ab"), *ndpOpts.sourceLinkLayerAddress);
  EXPECT_TRUE(ndpOpts.mtu);
  EXPECT_EQ(1280, *ndpOpts.mtu);
}

TEST(NDPOptionsTest, supportedAndUnsupportedOptions) {
  auto ndpOpts = parseNdpOptions(
      // Option 1 (supported)
      "01" // Type: Source Link-Layer Address
      "01" // Length (1 * 8 bytes)
      "0123456789ab" // SrcLLAaddr (ethernet addr)
      // Option 3 (unsupported)
      "03" // Type: Prefix info
      "04" // Length (4 * 8 bytes)
      "000000000000"
      "0000000000000000"
      "0000000000000000"
      "0000000000000000"
      // Option 2 (supported)
      "05" // Type: MTU
      "01" // Length (1 * 8 bytes)
      "0000" // res (0)
      "00000500" // mtu (1280)
  );
  EXPECT_TRUE(ndpOpts.sourceLinkLayerAddress);
  EXPECT_EQ(MacAddress("01:23:45:67:89:ab"), *ndpOpts.sourceLinkLayerAddress);
  EXPECT_TRUE(ndpOpts.mtu);
  EXPECT_EQ(1280, *ndpOpts.mtu);
}
