/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/packet/tajo/TajoPuntHeader.h"

#include <cstring>

#include <folly/io/Cursor.h>
#include <folly/io/IOBuf.h>
#include <gtest/gtest.h>

namespace facebook::fboss {

TEST(TajoPuntHeaderTest, InnerBytesOnWire) {
  const size_t overhead = kTajoModOuterEthLen + kTajoModOuterIpv6Len +
      kTajoModOuterUdpLen + kTajoNplPuntHeaderBytes;
  // A frame smaller than the fixed outer+punt overhead has no inner sample.
  EXPECT_EQ(tajoModInnerBytesOnWire(overhead - 1, kTajoModOuterEthLen), 0);
  // Otherwise the inner sample is whatever is left after the overhead.
  EXPECT_EQ(tajoModInnerBytesOnWire(overhead + 64, kTajoModOuterEthLen), 64);
}

TEST(TajoPuntHeaderTest, InnerLayoutPredicates) {
  EXPECT_TRUE(tajoPuntInnerHasEthernetHdr(TajoPuntNextHeaderKind::Ethernet));
  EXPECT_FALSE(tajoPuntInnerHasEthernetHdr(TajoPuntNextHeaderKind::Ipv4));

  EXPECT_TRUE(tajoPuntInnerStartsAtL3(TajoPuntNextHeaderKind::Ipv4));
  EXPECT_TRUE(tajoPuntInnerStartsAtL3(TajoPuntNextHeaderKind::Ipv6));
  EXPECT_FALSE(tajoPuntInnerStartsAtL3(TajoPuntNextHeaderKind::Ethernet));
}

TEST(TajoPuntHeaderTest, ParseUnknownFallsBackAndAdvancesCursor) {
  // An all-zero punt header maps to next_header == Unknown in both byte
  // orders, so the wire parser rejects it and parseTajoPuntHeader falls back
  // to consuming the fixed header size.
  constexpr size_t kTrailing = 8;
  auto buf = folly::IOBuf::create(kTajoNplPuntHeaderBytes + kTrailing);
  buf->append(kTajoNplPuntHeaderBytes + kTrailing);
  std::memset(buf->writableData(), 0, buf->length());

  folly::io::Cursor cursor(buf.get());
  const auto parsed = parseTajoPuntHeader(cursor);

  EXPECT_EQ(parsed.consumedBytes, kTajoNplPuntHeaderBytes);
  EXPECT_EQ(parsed.nextHeaderKind, TajoPuntNextHeaderKind::Unknown);
  EXPECT_FALSE(parsed.parsedByWire);
  // Cursor advanced exactly past the punt header.
  EXPECT_EQ(cursor.totalLength(), kTrailing);
}

TEST(TajoPuntHeaderTest, ParseWellFormedHeaderPopulatesFields) {
  // Construct a punt header whose fixed 40 bytes decode, in raw wire order, to
  // a recognized next_header plus a non-zero source system port and drop code,
  // exercising the recognized-header path (populateFromLaPacketFields,
  // setIngressPort, mapKernelNextHeaderKind). Byte offsets follow the
  // LaPacketPuntHeader bit layout (little-endian, packed): next_header is the
  // top 5 bits of byte 39, source_sp is bytes 33-34, code is byte 36. Byte 0 is
  // left zero so the SDK (reversed) byte order fails and the raw-order path is
  // taken.
  constexpr size_t kTrailing = 4;
  auto buf = folly::IOBuf::create(kTajoNplPuntHeaderBytes + kTrailing);
  buf->append(kTajoNplPuntHeaderBytes + kTrailing);
  auto* data = buf->writableData();
  std::memset(data, 0, buf->length());
  data[39] = static_cast<uint8_t>(6 << 3); // next_header = 6 (Ipv6)
  data[33] = 0x34; // source_sp low byte
  data[34] = 0x12; // source_sp high byte -> sourceSp = 0x1234
  data[36] = 0x2A; // code = 0x2A

  folly::io::Cursor cursor(buf.get());
  const auto parsed = parseTajoPuntHeader(cursor);

  EXPECT_TRUE(parsed.parsedByWire);
  EXPECT_EQ(parsed.consumedBytes, kTajoNplPuntHeaderBytes);
  EXPECT_EQ(parsed.nextHeaderKind, TajoPuntNextHeaderKind::Ipv6);
  EXPECT_EQ(parsed.sourceSp, 0x1234);
  EXPECT_EQ(parsed.ingressPort, 0x1234); // derived from sourceSp
  EXPECT_EQ(parsed.code, 0x2A);
  EXPECT_EQ(parsed.dropReason, 0x2A); // dropReason mirrors code
  EXPECT_EQ(cursor.totalLength(), kTrailing);
}

TEST(TajoPuntHeaderTest, ParseShortBufferDoesNotThrow) {
  // Fewer bytes than the fixed punt header: parse must not throw and must
  // report that it consumed nothing, leaving the cursor untouched.
  constexpr size_t kShort = kTajoNplPuntHeaderBytes - 1;
  auto buf = folly::IOBuf::create(kShort);
  buf->append(kShort);
  std::memset(buf->writableData(), 0, buf->length());

  folly::io::Cursor cursor(buf.get());
  ParsedTajoPuntHeader parsed;
  EXPECT_NO_THROW({ parsed = parseTajoPuntHeader(cursor); });
  EXPECT_EQ(parsed.consumedBytes, 0);
  EXPECT_FALSE(parsed.parsedByWire);
  EXPECT_EQ(cursor.totalLength(), kShort);
}

} // namespace facebook::fboss
