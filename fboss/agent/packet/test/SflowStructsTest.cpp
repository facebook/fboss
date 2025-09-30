/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/packet/SflowStructs.h"

#include <cstdint>
#include <vector>

#include <folly/IPAddress.h>
#include <folly/container/Array.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace facebook::fboss::sflow {
using ::testing::ContainerEq;
using ::testing::ElementsAre;

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

TEST(SflowStructsTest, FlowRecordOwned) {
  // Test basic functionality of FlowRecordOwned
  sflow::FlowRecordOwned ownedFlowRecord;
  ownedFlowRecord.flowFormat = 1;
  ownedFlowRecord.flowData = {0x01, 0x02, 0x03, 0x04, 0x05};

  // Test size calculation - now includes XDR padding
  // 5 bytes data needs 3 bytes padding to align to 4-byte boundary
  uint32_t expectedSize = 4 /* flowFormat */ + 4 /* flowDataLen */ +
      8; /* 5 data bytes + 3 padding bytes */
  EXPECT_EQ(ownedFlowRecord.size(), expectedSize);

  // Test serialization
  int bufSize = 1024;
  std::vector<uint8_t> buffer(bufSize);
  auto buf = folly::IOBuf::wrapBuffer(buffer.data(), bufSize);
  auto cursor = std::make_shared<folly::io::RWPrivateCursor>(buf.get());

  ownedFlowRecord.serialize(cursor.get());
  size_t serializedSize = bufSize - cursor->length();

  // Verify serialized size matches expected (with XDR padding)
  // 5 bytes data + 3 bytes padding = 8 bytes aligned to 4-byte boundary
  uint32_t expectedSerializedSize = 4 /* flowFormat */ + 4 /* flowDataLen */ +
      8; /* 5 data bytes + 3 padding */
  EXPECT_EQ(serializedSize, expectedSerializedSize);

  // Verify the serialized content using ElementsAre matcher
  std::vector<uint8_t> actualData(
      buffer.begin(), buffer.begin() + serializedSize);
  EXPECT_THAT(
      actualData,
      ElementsAre(
          // flowFormat (42) as big-endian
          0x00,
          0x00,
          0x00,
          0x01,
          // flowDataLen (5) as big-endian
          0x00,
          0x00,
          0x00,
          0x05,
          // data content
          0x01,
          0x02,
          0x03,
          0x04,
          0x05,
          // padding bytes (zero)
          0x00,
          0x00,
          0x00));
}

TEST(SflowStructsTest, FlowSampleOwned) {
  // Test comprehensive functionality of FlowSampleOwned with detailed
  // serialization verification
  sflow::FlowSampleOwned ownedSample;
  ownedSample.sequenceNumber = 12345; // 0x3039
  ownedSample.sourceID = 67890; // 0x10932
  ownedSample.samplingRate = 100; // 0x64
  ownedSample.samplePool = 200; // 0xC8
  ownedSample.drops = 5; // 0x5
  ownedSample.input = 42; // 0x2A
  ownedSample.output = 24; // 0x18

  // Add FlowRecordOwned records with specific data for testing
  sflow::FlowRecordOwned record1;
  record1.flowFormat = 1;
  record1.flowData = {0x11, 0x22, 0x33, 0x44}; // 4 bytes, no padding needed

  sflow::FlowRecordOwned record2;
  record2.flowFormat = 1;
  record2.flowData = {
      0xAA, 0xBB}; // 2 bytes, will need 2 bytes padding for XDR alignment

  ownedSample.flowRecords = {record1, record2};

  // Test size calculation
  uint32_t expectedSize = 4 /* sequenceNumber */ + 4 /* sourceID */ +
      4 /* samplingRate */ + 4 /* samplePool */ + 4 /* drops */ +
      4 /* input */ + 4 /* output */ + 4 /* flowRecordsCnt */ + record1.size() +
      record2.size();
  EXPECT_EQ(ownedSample.size(), expectedSize);

  // Test serialization
  int bufSize = 1024;
  std::vector<uint8_t> buffer(bufSize);
  auto buf = folly::IOBuf::wrapBuffer(buffer.data(), bufSize);
  auto cursor = std::make_shared<folly::io::RWPrivateCursor>(buf.get());

  ownedSample.serialize(cursor.get());
  size_t serializedSize = bufSize - cursor->length();

  // Fixed fields: 32 bytes
  // record1: 4 (format) + 4 (len) + 4 (data, no padding needed) = 12 bytes
  // record2: 4 (format) + 4 (len) + 4 (data with XDR padding: 2 bytes + 2
  // padding) = 12 bytes Total expected: 32 + 12 + 12 = 56 bytes
  uint32_t actualExpectedSize = 32 + record1.size() + record2.size();
  EXPECT_EQ(serializedSize, actualExpectedSize);

  // Verify the complete serialized structure byte by byte
  std::vector<uint8_t> actualData(
      buffer.begin(), buffer.begin() + serializedSize);
  EXPECT_THAT(
      actualData,
      ElementsAre(
          // sequenceNumber (12345 = 0x3039) as big-endian
          0x00,
          0x00,
          0x30,
          0x39,
          // sourceID (67890 = 0x10932) as big-endian
          0x00,
          0x01,
          0x09,
          0x32,
          // samplingRate (100 = 0x64) as big-endian
          0x00,
          0x00,
          0x00,
          0x64,
          // samplePool (200 = 0xC8) as big-endian
          0x00,
          0x00,
          0x00,
          0xC8,
          // drops (5) as big-endian
          0x00,
          0x00,
          0x00,
          0x05,
          // input (42 = 0x2A) as big-endian
          0x00,
          0x00,
          0x00,
          0x2A,
          // output (24 = 0x18) as big-endian
          0x00,
          0x00,
          0x00,
          0x18,
          // flowRecordsCnt (2) as big-endian
          0x00,
          0x00,
          0x00,
          0x02,

          // First FlowRecord:
          // flowFormat (1) as big-endian
          0x00,
          0x00,
          0x00,
          0x01,
          // flowDataLen (4) as big-endian
          0x00,
          0x00,
          0x00,
          0x04,
          // flowData: 0x11, 0x22, 0x33, 0x44 (4 bytes, no padding needed)
          0x11,
          0x22,
          0x33,
          0x44,

          // Second FlowRecord:
          // flowFormat (1) as big-endian
          0x00,
          0x00,
          0x00,
          0x01,
          // flowDataLen (2) as big-endian
          0x00,
          0x00,
          0x00,
          0x02,
          // flowData: 0xAA, 0xBB + 2 bytes padding for XDR alignment
          0xAA,
          0xBB,
          0x00,
          0x00));

  // Verify that the size calculation matches the actual serialized size
  EXPECT_EQ(ownedSample.size(), serializedSize);
}

TEST(SflowStructsTest, FlowSampleOwnedSizeCalculation) {
  // Test size calculation with various numbers of flow records
  sflow::FlowSampleOwned sample;
  sample.sequenceNumber = 1;
  sample.sourceID = 2;
  sample.samplingRate = 3;
  sample.samplePool = 4;
  sample.drops = 5;
  sample.input = 6;
  sample.output = 7;

  // Test empty flow records
  EXPECT_EQ(sample.size(), 32); // 8 fields * 4 bytes each

  // Add one record
  sflow::FlowRecordOwned record1;
  record1.flowFormat = 1;
  record1.flowData = {0x01, 0x02, 0x03}; // 3 bytes
  sample.flowRecords.push_back(record1);
  EXPECT_EQ(sample.size(), 32 + record1.size());

  // Add another record
  sflow::FlowRecordOwned record2;
  record2.flowFormat = 2;
  record2.flowData = {0x04, 0x05, 0x06, 0x07, 0x08, 0x09}; // 6 bytes
  sample.flowRecords.push_back(record2);
  EXPECT_EQ(sample.size(), 32 + record1.size() + record2.size());
}

TEST(SflowStructsTest, SampleRecordOwned) {
  // Test comprehensive functionality of SampleRecordOwned with detailed
  // serialization verification
  sflow::SampleRecordOwned ownedRecord;
  ownedRecord.sampleType = 1; // Flow sample type

  // Create a FlowSampleOwned to add to the sample data
  sflow::FlowSampleOwned flowSample;
  flowSample.sequenceNumber = 98765; // 0x181CD
  flowSample.sourceID = 54321; // 0xD431
  flowSample.samplingRate = 1000; // 0x3E8
  flowSample.samplePool = 2000; // 0x7D0
  flowSample.drops = 10; // 0xA
  flowSample.input = 123; // 0x7B
  flowSample.output = 456; // 0x1C8

  // Add FlowRecords to the FlowSample
  sflow::FlowRecordOwned record1;
  record1.flowFormat = 1;
  record1.flowData = {0xDE, 0xAD, 0xBE, 0xEF}; // 4 bytes, no padding needed

  sflow::FlowRecordOwned record2;
  record2.flowFormat = 1;
  record2.flowData = {0xCA, 0xFE}; // 2 bytes, will need 2 bytes padding

  flowSample.flowRecords = {record1, record2};

  // Add the FlowSample to the SampleRecord
  ownedRecord.sampleData.push_back(flowSample);

  uint32_t expectedSize = 4 /* sampleType */ + 4 /* sampleDataLen */ +
      std::get<FlowSampleOwned>(ownedRecord.sampleData[0]).size();
  EXPECT_EQ(ownedRecord.size(), expectedSize);

  // Test serialization
  int bufSize = 1024;
  std::vector<uint8_t> buffer(bufSize);
  auto buf = folly::IOBuf::wrapBuffer(buffer.data(), bufSize);
  auto cursor = std::make_shared<folly::io::RWPrivateCursor>(buf.get());

  ownedRecord.serialize(cursor.get());
  size_t serializedSize = bufSize - cursor->length();

  // Verify basic structure - should include sample type, data length, and the
  // flow sample data
  EXPECT_GT(serializedSize, 8); // At least the sample type + data length

  // Verify the complete serialized buffer content
  std::vector<uint8_t> actualData(
      buffer.begin(), buffer.begin() + serializedSize);
  EXPECT_THAT(
      actualData,
      ElementsAre(
          // sampleType (1) as big-endian
          0x00,
          0x00,
          0x00,
          0x01,
          // sampleDataLen - the size of the FlowSample data (56 bytes)
          0x00,
          0x00,
          0x00,
          0x38,

          // FlowSample data:
          // sequenceNumber (98765 = 0x181CD) as big-endian
          0x00,
          0x01,
          0x81,
          0xCD,
          // sourceID (54321 = 0xD431) as big-endian
          0x00,
          0x00,
          0xD4,
          0x31,
          // samplingRate (1000 = 0x3E8) as big-endian
          0x00,
          0x00,
          0x03,
          0xE8,
          // samplePool (2000 = 0x7D0) as big-endian
          0x00,
          0x00,
          0x07,
          0xD0,
          // drops (10 = 0xA) as big-endian
          0x00,
          0x00,
          0x00,
          0x0A,
          // input (123 = 0x7B) as big-endian
          0x00,
          0x00,
          0x00,
          0x7B,
          // output (456 = 0x1C8) as big-endian
          0x00,
          0x00,
          0x01,
          0xC8,
          // flowRecordsCnt (2) as big-endian
          0x00,
          0x00,
          0x00,
          0x02,

          // First FlowRecord:
          // flowFormat (1) as big-endian
          0x00,
          0x00,
          0x00,
          0x01,
          // flowDataLen (4) as big-endian
          0x00,
          0x00,
          0x00,
          0x04,
          // flowData: 0xDE, 0xAD, 0xBE, 0xEF (4 bytes, no padding needed)
          0xDE,
          0xAD,
          0xBE,
          0xEF,

          // Second FlowRecord:
          // flowFormat (1) as big-endian
          0x00,
          0x00,
          0x00,
          0x01,
          // flowDataLen (2) as big-endian
          0x00,
          0x00,
          0x00,
          0x02,
          // flowData: 0xCA, 0xFE + 2 bytes padding for XDR alignment
          0xCA,
          0xFE,
          0x00,
          0x00));

  // Verify that the size calculation matches the actual serialized size
  EXPECT_EQ(ownedRecord.size(), serializedSize);
}

TEST(SflowStructsTest, SampleRecordOwnedMultipleFlowSamples) {
  // Test SampleRecordOwned with multiple FlowSample data
  sflow::SampleRecordOwned ownedRecord;
  ownedRecord.sampleType = 1;

  // Create first FlowSample
  sflow::FlowSampleOwned flowSample1;
  flowSample1.sequenceNumber = 1111;
  flowSample1.sourceID = 2222;
  flowSample1.samplingRate = 3333;
  flowSample1.samplePool = 4444;
  flowSample1.drops = 5555;
  flowSample1.input = 6666;
  flowSample1.output = 7777;

  sflow::FlowRecordOwned record1;
  record1.flowFormat = 1;
  record1.flowData = {0x11, 0x22}; // 2 bytes + 2 padding = 4 aligned
  flowSample1.flowRecords = {record1};

  // Create second FlowSample
  sflow::FlowSampleOwned flowSample2;
  flowSample2.sequenceNumber = 8888;
  flowSample2.sourceID = 9999;
  flowSample2.samplingRate = 1010;
  flowSample2.samplePool = 2020;
  flowSample2.drops = 3030;
  flowSample2.input = 4040;
  flowSample2.output = 5050;

  sflow::FlowRecordOwned record2;
  record2.flowFormat = 1;
  record2.flowData = {0xAA, 0xBB, 0xCC}; // 3 bytes + 1 padding = 4 aligned
  flowSample2.flowRecords = {record2};

  // Add both FlowSamples to the SampleRecord
  ownedRecord.sampleData.push_back(flowSample1);
  ownedRecord.sampleData.push_back(flowSample2);

  // Test size calculation - should include both FlowSamples
  uint32_t expectedFlowSample1Size = 32 + 12; // 32 fixed + 12 for record1
  uint32_t expectedFlowSample2Size = 32 + 12; // 32 fixed + 12 for record2
  uint32_t expectedTotalSize = 4 /* sampleType */ + 4 /* sampleDataLen */ +
      expectedFlowSample1Size + expectedFlowSample2Size;
  EXPECT_EQ(ownedRecord.size(), expectedTotalSize);

  // Test serialization
  int bufSize = 1024;
  std::vector<uint8_t> buffer(bufSize);
  auto buf = folly::IOBuf::wrapBuffer(buffer.data(), bufSize);
  auto cursor = std::make_shared<folly::io::RWPrivateCursor>(buf.get());

  ownedRecord.serialize(cursor.get());
  size_t serializedSize = bufSize - cursor->length();

  // Verify that serialization size matches calculated size
  EXPECT_EQ(ownedRecord.size(), serializedSize);
  EXPECT_EQ(ownedRecord.sampleData.size(), 2);
}

TEST(SflowStructsTest, SampleRecordOwnedSizeCalculation) {
  // Test size calculation with various configurations
  sflow::SampleRecordOwned record;
  record.sampleType = 1;

  // Test empty sample data
  EXPECT_EQ(record.size(), 8); // 4 + 4 + 0

  // Add one FlowSample
  sflow::FlowSampleOwned flowSample1;
  flowSample1.sequenceNumber = 1;
  flowSample1.sourceID = 2;
  flowSample1.samplingRate = 3;
  flowSample1.samplePool = 4;
  flowSample1.drops = 5;
  flowSample1.input = 6;
  flowSample1.output = 7;
  // Empty flow records

  record.sampleData.emplace_back(flowSample1);
  uint32_t expectedSize1 = 8 + flowSample1.size();
  EXPECT_EQ(record.size(), expectedSize1);

  // Add another FlowSample
  sflow::FlowSampleOwned flowSample2;
  flowSample2.sequenceNumber = 10;
  flowSample2.sourceID = 20;
  flowSample2.samplingRate = 30;
  flowSample2.samplePool = 40;
  flowSample2.drops = 50;
  flowSample2.input = 60;
  flowSample2.output = 70;

  // Add a FlowRecord to the second sample
  sflow::FlowRecordOwned flowRecord;
  flowRecord.flowFormat = 1;
  flowRecord.flowData = {
      0x01, 0x02, 0x03, 0x04, 0x05}; // 5 bytes + 3 padding = 8
  flowSample2.flowRecords = {flowRecord};

  record.sampleData.emplace_back(flowSample2);
  uint32_t expectedSize2 = 8 + flowSample1.size() + flowSample2.size();
  EXPECT_EQ(record.size(), expectedSize2);
}

} // namespace facebook::fboss::sflow
