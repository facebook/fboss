/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/packet/LlcHdr.h"

/*
 * References:
 *   https://code.google.com/p/googletest/wiki/Primer
 *   https://code.google.com/p/googletest/wiki/AdvancedGuide
 */
#include <gtest/gtest.h>

#include <folly/io/Cursor.h>

#include "fboss/agent/hw/mock/MockRxPacket.h"

using namespace facebook::fboss;
using folly::io::Cursor;

TEST(LlcHdrTest, default_constructor) {
  LlcHdr llcHdr;
  EXPECT_EQ(0, llcHdr.dsap);
  EXPECT_EQ(0, llcHdr.ssap);
  EXPECT_EQ(0, llcHdr.control);
}

TEST(LlcHdrTest, copy_constructor) {
  uint8_t dsap = static_cast<uint8_t>(LLC_SAP_ADDR::LLC_SAP_STP);
  uint8_t ssap = static_cast<uint8_t>(LLC_SAP_ADDR::LLC_SAP_STP);
  uint8_t control = static_cast<uint8_t>(LLC_CONTROL::LLC_CONTROL_UI);
  LlcHdr lhs(dsap, ssap, control);
  LlcHdr rhs(lhs);
  EXPECT_EQ(lhs, rhs);
}

TEST(LlcHdrTest, parameterized_data_constructor) {
  uint8_t dsap = static_cast<uint8_t>(LLC_SAP_ADDR::LLC_SAP_STP);
  uint8_t ssap = static_cast<uint8_t>(LLC_SAP_ADDR::LLC_SAP_STP);
  uint8_t control = static_cast<uint8_t>(LLC_CONTROL::LLC_CONTROL_UI);
  LlcHdr llcHdr(dsap, ssap, control);
  EXPECT_EQ(dsap, llcHdr.dsap);
  EXPECT_EQ(ssap, llcHdr.ssap);
  EXPECT_EQ(control, llcHdr.control);
}

TEST(LlcHdrTest, cursor_data_constructor) {
  uint8_t dsap = static_cast<uint8_t>(LLC_SAP_ADDR::LLC_SAP_STP);
  uint8_t ssap = static_cast<uint8_t>(LLC_SAP_ADDR::LLC_SAP_STP);
  uint8_t control = static_cast<uint8_t>(LLC_CONTROL::LLC_CONTROL_UI);
  auto pkt = MockRxPacket::fromHex(
      // LLC Header
      "42" // DSAP: STP
      "42" // SSAP: STP
      "03" // Control: UI
  );
  Cursor cursor(pkt->buf());
  LlcHdr llcHdr(cursor);
  EXPECT_EQ(dsap, llcHdr.dsap);
  EXPECT_EQ(ssap, llcHdr.ssap);
  EXPECT_EQ(control, llcHdr.control);
}

TEST(LlcHdrTest, cursor_data_constructor_too_small) {
  auto pkt = MockRxPacket::fromHex(
      // LLC Header
      "42" // DSAP: STP
      "42" // SSAP: STP
      "  " // OOPS! One octet too small!
  );
  Cursor cursor(pkt->buf());
  EXPECT_THROW({ LlcHdr llcHdr(cursor); }, HdrParseError);
}

TEST(LlcHdrTest, cursor_data_constructor_ssap_global) {
  auto pkt = MockRxPacket::fromHex(
      // LLC Header
      "42" // DSAP: STP
      "FF" // SSAP: Global
      "03" // Control: UI
  );
  Cursor cursor(pkt->buf());
  EXPECT_THROW({ LlcHdr llcHdr(cursor); }, HdrParseError);
}

TEST(LlcHdrTest, cursor_data_constructor_control_unsupported) {
  auto pkt = MockRxPacket::fromHex(
      // LLC Header
      "42" // DSAP: STP
      "42" // SSAP: STP
      "E3" // Control: Test
  );
  Cursor cursor(pkt->buf());
  EXPECT_THROW({ LlcHdr llcHdr(cursor); }, HdrParseError);
}

TEST(LlcHdrTest, assignment_operator) {
  uint8_t dsap = static_cast<uint8_t>(LLC_SAP_ADDR::LLC_SAP_STP);
  uint8_t ssap = static_cast<uint8_t>(LLC_SAP_ADDR::LLC_SAP_STP);
  uint8_t control = static_cast<uint8_t>(LLC_CONTROL::LLC_CONTROL_UI);
  LlcHdr lhs(dsap, ssap, control);
  LlcHdr rhs = lhs;
  EXPECT_EQ(lhs, rhs);
}

TEST(LlcHdrTest, equality_operator) {
  uint8_t dsap = static_cast<uint8_t>(LLC_SAP_ADDR::LLC_SAP_STP);
  uint8_t ssap = static_cast<uint8_t>(LLC_SAP_ADDR::LLC_SAP_STP);
  uint8_t control = static_cast<uint8_t>(LLC_CONTROL::LLC_CONTROL_UI);
  LlcHdr lhs(dsap, ssap, control);
  LlcHdr rhs(dsap, ssap, control);
  EXPECT_EQ(lhs, rhs);
}

TEST(LlcHdrTest, inequality_operator) {
  uint8_t dsap1 = static_cast<uint8_t>(LLC_SAP_ADDR::LLC_SAP_STP);
  uint8_t dsap2 = static_cast<uint8_t>(LLC_SAP_ADDR::LLC_SAP_GLOBAL);
  uint8_t ssap = static_cast<uint8_t>(LLC_SAP_ADDR::LLC_SAP_STP);
  uint8_t control = static_cast<uint8_t>(LLC_CONTROL::LLC_CONTROL_UI);
  LlcHdr lhs(dsap1, ssap, control);
  LlcHdr rhs(dsap2, ssap, control);
  EXPECT_NE(lhs, rhs);
}
