/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include <folly/IPAddress.h>
#include <folly/container/Array.h>

#include "fboss/agent/packet/SflowStructs.h"

#include <gtest/gtest.h>

using namespace facebook::fboss;

TEST(SflowStructsTest, Serialize) {
  folly::IPAddress agentIP("2401:db00:116:3016::1b");
  int bufSize = 1024;

  // The fake pkt data with 11 octets and with all '1's
  size_t datasize = 11;
  std::vector<uint8_t> dataBuf(datasize);
  memset(dataBuf.data(), 15, datasize);

  sflow::SampleDatagram datagram;
  sflow::SampleDatagramV5 datagramV5;
  sflow::SampleRecord record;
  sflow::FlowSample fsample;
  sflow::FlowRecord frecord;
  sflow::SampledHeader hdr;

  // The sample header test
  hdr.protocol = sflow::HeaderProtocol::ETHERNET_ISO88023;
  hdr.frameLength = 0;
  hdr.stripped = 0;
  hdr.headerLength = datasize;
  hdr.header = dataBuf.data();
  auto hdrSize = hdr.size();
  EXPECT_EQ(hdrSize, 27);
  if (hdrSize % sflow::XDR_BASIC_BLOCK_SIZE > 0) {
    hdrSize = (hdrSize / sflow::XDR_BASIC_BLOCK_SIZE + 1) *
        sflow::XDR_BASIC_BLOCK_SIZE;
  }
  EXPECT_EQ(hdrSize, 28);

  std::vector<uint8_t> hb(bufSize);
  auto hbuf = folly::IOBuf::wrapBuffer(hb.data(), bufSize);
  auto hc = std::make_shared<folly::io::RWPrivateCursor>(hbuf.get());
  hdr.serialize(hc.get());
  EXPECT_EQ(28, bufSize - hc->length());

  frecord.flowFormat = 1; // single flow sample
  frecord.flowDataLen = hdrSize;
  frecord.flowData = hb.data();
  size_t frecordSize = 8 + bufSize - hc->length();
  EXPECT_EQ(36, frecordSize);

  fsample.sequenceNumber = 0;
  fsample.sourceID = 0;
  fsample.samplingRate = 123;
  fsample.samplePool = 0;
  fsample.drops = 0;
  fsample.input = 56;
  fsample.output = 6;
  fsample.flowRecordsCnt = 1;
  fsample.flowRecords = &frecord;

  std::vector<uint8_t> fsb(bufSize);
  auto fbuf = folly::IOBuf::wrapBuffer(fsb.data(), bufSize);
  auto fc = std::make_shared<folly::io::RWPrivateCursor>(fbuf.get());
  fsample.serialize(fc.get());
  size_t fsampleSize = bufSize - fc->length();
  EXPECT_EQ(68, fsampleSize);

  record.sampleType = 1; // raw header
  record.sampleDataLen = fsampleSize;
  record.sampleData = fsb.data();

  size_t recordSize = 8 + fsampleSize;
  EXPECT_EQ(76, recordSize);

  datagramV5.agentAddress = agentIP;
  datagramV5.subAgentID = 0; // no sub agent
  datagramV5.sequenceNumber = 0; // not used
  datagramV5.uptime = 0; // not used
  datagramV5.samplesCnt = 1; // So far only 1 sample encapsuled
  datagramV5.samples = &record;

  datagram.datagramV5 = datagramV5;

  std::vector<uint8_t> b(bufSize);
  auto buf = folly::IOBuf::wrapBuffer(b.data(), bufSize);
  auto cursor = std::make_shared<folly::io::RWPrivateCursor>(buf.get());
  datagram.serialize(cursor.get());
  size_t datagramSize = bufSize - cursor->length();
  EXPECT_EQ(116, datagramSize);

  constexpr auto data = folly::make_array<uint8_t>(
      0x00,
      0x00,
      0x00,
      0x05, // version
      0x00,
      0x00,
      0x00,
      0x02, // ipv6 type = 2
      0x24,
      0x01,
      0xdb,
      0x00,
      0x01,
      0x16,
      0x30,
      0x16, // ......
      0x00,
      0x00,
      0x00,
      0x00,
      0x00,
      0x00,
      0x00,
      0x1b, // ... ipv6 addr
      0x00,
      0x00,
      0x00,
      0x00, // sub agent
      0x00,
      0x00,
      0x00,
      0x00, // seq no.
      0x00,
      0x00,
      0x00,
      0x00, // uptime
      0x00,
      0x00,
      0x00,
      0x01, // sample cnt
      0x00,
      0x00,
      0x00,
      0x01, // sample type
      0x00,
      0x00,
      0x00,
      0x44, // data length
      0x00,
      0x00,
      0x00,
      0x00, // seq no.
      0x00,
      0x00,
      0x00,
      0x00, // source ID
      0x00,
      0x00,
      0x00,
      0x7b, // sampling rate
      0x00,
      0x00,
      0x00,
      0x00, // sample pool
      0x00,
      0x00,
      0x00,
      0x00, // drops
      0x00,
      0x00,
      0x00,
      0x38, // input port
      0x00,
      0x00,
      0x00,
      0x06, // output port
      0x00,
      0x00,
      0x00,
      0x01, // record cnt
      0x00,
      0x00,
      0x00,
      0x01, // flow format
      0x00,
      0x00,
      0x00,
      0x1c, // header size
      0x00,
      0x00,
      0x00,
      0x01, // protocol
      0x00,
      0x00,
      0x00,
      0x00, // frame length
      0x00,
      0x00,
      0x00,
      0x00, // stripped
      0x00,
      0x00,
      0x00,
      0x0b, // data size
      0x0f,
      0x0f,
      0x0f,
      0x0f,
      0x0f,
      0x0f,
      0x0f,
      0x0f,
      0x0f,
      0x0f,
      0x0f,
      0x00);

  for (int i = 0; i < datagramSize; ++i) {
    EXPECT_EQ(b.at(i), data[i]);
  }
}
