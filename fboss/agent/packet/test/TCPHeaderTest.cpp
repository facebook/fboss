/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/packet/TCPHeader.h"
#include <gtest/gtest.h>
#include "fboss/agent/FbossError.h"
#include "fboss/agent/PortStats.h"
#include "fboss/agent/SwitchStats.h"

using namespace facebook::fboss;
using folly::IOBuf;
using folly::io::Cursor;

TEST(TCPHeader, writeAndParse) {
  TCPHeader tcp(1234, 4321);
  tcp.sequenceNumber = 100;
  tcp.ackNumber = 101;
  tcp.dataOffsetAndReserved = 32;
  tcp.flags = 10;
  tcp.windowSize = 1024;
  tcp.csum = 4242;
  tcp.urgentPointer = 0;
  uint8_t tcpSizeArr[TCPHeader::size()];
  folly::IOBuf buf(IOBuf::WRAP_BUFFER, tcpSizeArr, sizeof(tcpSizeArr));
  folly::io::RWPrivateCursor cursor(&buf);
  tcp.write(&cursor);
  TCPHeader tcp2(0, 0);
  Cursor readCursor(&buf);
  tcp2.parse(&readCursor);
  EXPECT_EQ(tcp, tcp2);
}

TEST(TCPHeader, parseError) {
  uint8_t tcpSizeSmall[TCPHeader::size() - 1];
  folly::IOBuf buf(IOBuf::WRAP_BUFFER, tcpSizeSmall, sizeof(tcpSizeSmall));
  Cursor readCursor(&buf);
  TCPHeader tcp(0, 0);
  EXPECT_THROW(tcp.parse(&readCursor), FbossError);
}
