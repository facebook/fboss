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

TEST(SflowStructsTest, FlowRecord) {
  // Test basic functionality of FlowRecord
  sflow::FlowRecord flowRecord;
  flowRecord.flowFormat = 1;
  flowRecord.flowData = {0x01, 0x02, 0x03, 0x04, 0x05};

  // Test size calculation - now includes XDR padding
  // 5 bytes data needs 3 bytes padding to align to 4-byte boundary
  uint32_t expectedSize = 4 /* flowFormat */ + 4 /* flowDataLen */ +
      8; /* 5 data bytes + 3 padding bytes */
  EXPECT_EQ(flowRecord.size(), expectedSize);

  // Test serialization
  int bufSize = 1024;
  std::vector<uint8_t> buffer(bufSize);
  auto buf = folly::IOBuf::wrapBuffer(buffer.data(), bufSize);
  auto cursor = std::make_shared<folly::io::RWPrivateCursor>(buf.get());

  flowRecord.serialize(cursor.get());
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

TEST(SflowStructsTest, FlowRecordDeserialization) {
  // Test FlowRecord deserialization functionality

  // Create a buffer with serialized FlowRecord data
  std::vector<uint8_t> serializedData = {// flowFormat (1) as big-endian
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
                                         0x00};

  auto buf =
      folly::IOBuf::wrapBuffer(serializedData.data(), serializedData.size());
  folly::io::Cursor cursor(buf.get());

  // Deserialize the FlowRecord
  sflow::FlowRecord flowRecord = sflow::FlowRecord::deserialize(cursor);

  // Verify the deserialized data
  EXPECT_EQ(flowRecord.flowFormat, 1);
  EXPECT_EQ(flowRecord.flowData.size(), 5);
  EXPECT_THAT(flowRecord.flowData, ElementsAre(0x01, 0x02, 0x03, 0x04, 0x05));
}

TEST(SflowStructsTest, FlowRecordSerializeDeserializeRoundTrip) {
  // Test serialize-deserialize round trip for FlowRecord

  // Create original FlowRecord with various data sizes to test padding
  std::vector<std::vector<uint8_t>> testData = {
      {},
      {0xAA}, // 1 byte (3 bytes padding)
      {0xBB, 0xCC}, // 2 bytes (2 bytes padding)
      {0xDD, 0xEE, 0xFF}, // 3 bytes (1 byte padding)
      {0x11, 0x22, 0x33, 0x44}, // 4 bytes (no padding)
      {0x55, 0x66, 0x77, 0x88, 0x99} // 5 bytes (3 bytes padding)
  };

  for (size_t i = 0; i < testData.size(); ++i) {
    // Create original FlowRecord
    sflow::FlowRecord original;
    original.flowFormat = 1;
    original.flowData = testData[i];

    // Serialize
    int bufSize = 1024;
    std::vector<uint8_t> buffer(bufSize);
    auto buf = folly::IOBuf::wrapBuffer(buffer.data(), bufSize);
    auto rwCursor = std::make_shared<folly::io::RWPrivateCursor>(buf.get());

    original.serialize(rwCursor.get());
    size_t serializedSize = bufSize - rwCursor->length();

    // Deserialize
    auto readBuf = folly::IOBuf::wrapBuffer(buffer.data(), serializedSize);
    folly::io::Cursor readCursor(readBuf.get());

    sflow::FlowRecord deserialized = sflow::FlowRecord::deserialize(readCursor);

    // Verify round-trip correctness
    EXPECT_EQ(deserialized.flowFormat, original.flowFormat)
        << "Mismatch for test case " << i;
    EXPECT_EQ(deserialized.flowData.size(), original.flowData.size())
        << "Data size mismatch for test case " << i;
    EXPECT_THAT(deserialized.flowData, ContainerEq(original.flowData))
        << "Data content mismatch for test case " << i;
  }
}

TEST(SflowStructsTest, FlowSampleDeserialization) {
  // Test FlowSample deserialization functionality

  // Create a buffer with serialized FlowSample data
  std::vector<uint8_t> serializedData = {
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
      0x00};

  auto buf =
      folly::IOBuf::wrapBuffer(serializedData.data(), serializedData.size());
  folly::io::Cursor cursor(buf.get());

  // Deserialize the FlowSample
  sflow::FlowSample flowSample = sflow::FlowSample::deserialize(cursor);

  // Verify the deserialized data
  EXPECT_EQ(flowSample.sequenceNumber, 12345);
  EXPECT_EQ(flowSample.sourceID, 67890);
  EXPECT_EQ(flowSample.samplingRate, 100);
  EXPECT_EQ(flowSample.samplePool, 200);
  EXPECT_EQ(flowSample.drops, 5);
  EXPECT_EQ(flowSample.input, 42);
  EXPECT_EQ(flowSample.output, 24);
  EXPECT_EQ(flowSample.flowRecords.size(), 2);

  // Verify first flow record
  EXPECT_EQ(flowSample.flowRecords[0].flowFormat, 1);
  EXPECT_EQ(flowSample.flowRecords[0].flowData.size(), 4);
  EXPECT_THAT(
      flowSample.flowRecords[0].flowData, ElementsAre(0x11, 0x22, 0x33, 0x44));

  // Verify second flow record
  EXPECT_EQ(flowSample.flowRecords[1].flowFormat, 1);
  EXPECT_EQ(flowSample.flowRecords[1].flowData.size(), 2);
  EXPECT_THAT(flowSample.flowRecords[1].flowData, ElementsAre(0xAA, 0xBB));
}

TEST(SflowStructsTest, FlowSampleSerializeDeserializeRoundTrip) {
  // Test serialize-deserialize round trip for FlowSample

  // Create original FlowSample with various flow records
  sflow::FlowSample original;
  original.sequenceNumber = 99999;
  original.sourceID = 88888;
  original.samplingRate = 77777;
  original.samplePool = 66666;
  original.drops = 55555;
  original.input = 44444;
  original.output = 33333;

  // Add flow records with different data sizes
  sflow::FlowRecord record1;
  record1.flowFormat = 1;
  record1.flowData = {0xDE, 0xAD, 0xBE, 0xEF}; // 4 bytes, no padding

  sflow::FlowRecord record2;
  record2.flowFormat = 1;
  record2.flowData = {0xCA, 0xFE}; // 2 bytes, 2 bytes padding

  sflow::FlowRecord record3;
  record3.flowFormat = 1;
  record3.flowData = {0x12, 0x34, 0x56, 0x78, 0x9A}; // 5 bytes, 3 bytes padding

  original.flowRecords = {record1, record2, record3};

  // Serialize
  int bufSize = 1024;
  std::vector<uint8_t> buffer(bufSize);
  auto buf = folly::IOBuf::wrapBuffer(buffer.data(), bufSize);
  auto rwCursor = std::make_shared<folly::io::RWPrivateCursor>(buf.get());

  original.serialize(rwCursor.get());
  size_t serializedSize = bufSize - rwCursor->length();

  // Deserialize
  auto readBuf = folly::IOBuf::wrapBuffer(buffer.data(), serializedSize);
  folly::io::Cursor readCursor(readBuf.get());

  sflow::FlowSample deserialized = sflow::FlowSample::deserialize(readCursor);

  // Verify round-trip correctness
  EXPECT_EQ(deserialized.sequenceNumber, original.sequenceNumber);
  EXPECT_EQ(deserialized.sourceID, original.sourceID);
  EXPECT_EQ(deserialized.samplingRate, original.samplingRate);
  EXPECT_EQ(deserialized.samplePool, original.samplePool);
  EXPECT_EQ(deserialized.drops, original.drops);
  EXPECT_EQ(deserialized.input, original.input);
  EXPECT_EQ(deserialized.output, original.output);
  EXPECT_EQ(deserialized.flowRecords.size(), original.flowRecords.size());

  // Verify each flow record
  for (size_t i = 0; i < original.flowRecords.size(); ++i) {
    EXPECT_EQ(
        deserialized.flowRecords[i].flowFormat,
        original.flowRecords[i].flowFormat)
        << "FlowFormat mismatch for record " << i;
    EXPECT_EQ(
        deserialized.flowRecords[i].flowData.size(),
        original.flowRecords[i].flowData.size())
        << "FlowData size mismatch for record " << i;
    EXPECT_THAT(
        deserialized.flowRecords[i].flowData,
        ContainerEq(original.flowRecords[i].flowData))
        << "FlowData content mismatch for record " << i;
  }
}

TEST(SflowStructsTest, FlowSampleDeserializeEmptyRecords) {
  // Test FlowSample deserialization with no flow records

  std::vector<uint8_t> serializedData = {// sequenceNumber (1) as big-endian
                                         0x00,
                                         0x00,
                                         0x00,
                                         0x01,
                                         // sourceID (2) as big-endian
                                         0x00,
                                         0x00,
                                         0x00,
                                         0x02,
                                         // samplingRate (3) as big-endian
                                         0x00,
                                         0x00,
                                         0x00,
                                         0x03,
                                         // samplePool (4) as big-endian
                                         0x00,
                                         0x00,
                                         0x00,
                                         0x04,
                                         // drops (5) as big-endian
                                         0x00,
                                         0x00,
                                         0x00,
                                         0x05,
                                         // input (6) as big-endian
                                         0x00,
                                         0x00,
                                         0x00,
                                         0x06,
                                         // output (7) as big-endian
                                         0x00,
                                         0x00,
                                         0x00,
                                         0x07,
                                         // flowRecordsCnt (0) as big-endian
                                         0x00,
                                         0x00,
                                         0x00,
                                         0x00};

  auto buf =
      folly::IOBuf::wrapBuffer(serializedData.data(), serializedData.size());
  folly::io::Cursor cursor(buf.get());

  // Deserialize the FlowSample
  sflow::FlowSample flowSample = sflow::FlowSample::deserialize(cursor);

  // Verify the deserialized data
  EXPECT_EQ(flowSample.sequenceNumber, 1);
  EXPECT_EQ(flowSample.sourceID, 2);
  EXPECT_EQ(flowSample.samplingRate, 3);
  EXPECT_EQ(flowSample.samplePool, 4);
  EXPECT_EQ(flowSample.drops, 5);
  EXPECT_EQ(flowSample.input, 6);
  EXPECT_EQ(flowSample.output, 7);
  EXPECT_EQ(flowSample.flowRecords.size(), 0);
}

TEST(SflowStructsTest, FlowSample) {
  // Test comprehensive functionality of FlowSample with detailed
  // serialization verification
  sflow::FlowSample sample;
  sample.sequenceNumber = 12345; // 0x3039
  sample.sourceID = 67890; // 0x10932
  sample.samplingRate = 100; // 0x64
  sample.samplePool = 200; // 0xC8
  sample.drops = 5; // 0x5
  sample.input = 42; // 0x2A
  sample.output = 24; // 0x18

  // Add FlowRecord records with specific data for testing
  sflow::FlowRecord record1;
  record1.flowFormat = 1;
  record1.flowData = {0x11, 0x22, 0x33, 0x44}; // 4 bytes, no padding needed

  sflow::FlowRecord record2;
  record2.flowFormat = 1;
  record2.flowData = {
      0xAA, 0xBB}; // 2 bytes, will need 2 bytes padding for XDR alignment

  sample.flowRecords = {record1, record2};

  // Test size calculation
  uint32_t expectedSize = 4 /* sequenceNumber */ + 4 /* sourceID */ +
      4 /* samplingRate */ + 4 /* samplePool */ + 4 /* drops */ +
      4 /* input */ + 4 /* output */ + 4 /* flowRecordsCnt */ + record1.size() +
      record2.size();
  EXPECT_EQ(sample.size(), expectedSize);

  // Test serialization
  int bufSize = 1024;
  std::vector<uint8_t> buffer(bufSize);
  auto buf = folly::IOBuf::wrapBuffer(buffer.data(), bufSize);
  auto cursor = std::make_shared<folly::io::RWPrivateCursor>(buf.get());

  sample.serialize(cursor.get());
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
  EXPECT_EQ(sample.size(), serializedSize);
}

TEST(SflowStructsTest, FlowSampleSizeCalculation) {
  // Test size calculation with various numbers of flow records
  sflow::FlowSample sample;
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
  sflow::FlowRecord record1;
  record1.flowFormat = 1;
  record1.flowData = {0x01, 0x02, 0x03}; // 3 bytes
  sample.flowRecords.push_back(record1);
  EXPECT_EQ(sample.size(), 32 + record1.size());

  // Add another record
  sflow::FlowRecord record2;
  record2.flowFormat = 2;
  record2.flowData = {0x04, 0x05, 0x06, 0x07, 0x08, 0x09}; // 6 bytes
  sample.flowRecords.push_back(record2);
  EXPECT_EQ(sample.size(), 32 + record1.size() + record2.size());
}

TEST(SflowStructsTest, SampleRecordDeserialization) {
  // Test SampleRecord deserialization functionality

  // Create a buffer with serialized SampleRecord data containing a FlowSample
  std::vector<uint8_t> serializedData = {
      // sampleType (1) as big-endian
      0x00,
      0x00,
      0x00,
      0x01,
      // sampleDataLen (56 bytes) as big-endian - size of the FlowSample below
      0x00,
      0x00,
      0x00,
      0x38,

      // FlowSample data (56 bytes total):
      // sequenceNumber (12345) as big-endian
      0x00,
      0x00,
      0x30,
      0x39,
      // sourceID (67890) as big-endian
      0x00,
      0x01,
      0x09,
      0x32,
      // samplingRate (100) as big-endian
      0x00,
      0x00,
      0x00,
      0x64,
      // samplePool (200) as big-endian
      0x00,
      0x00,
      0x00,
      0xC8,
      // drops (5) as big-endian
      0x00,
      0x00,
      0x00,
      0x05,
      // input (42) as big-endian
      0x00,
      0x00,
      0x00,
      0x2A,
      // output (24) as big-endian
      0x00,
      0x00,
      0x00,
      0x18,
      // flowRecordsCnt (1) as big-endian
      0x00,
      0x00,
      0x00,
      0x01,

      // FlowRecord:
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
      0x44};

  auto buf =
      folly::IOBuf::wrapBuffer(serializedData.data(), serializedData.size());
  folly::io::Cursor cursor(buf.get());

  // Deserialize the SampleRecord
  sflow::SampleRecord sampleRecord = sflow::SampleRecord::deserialize(cursor);

  // Verify the deserialized data
  EXPECT_EQ(sampleRecord.sampleType, 1);
  EXPECT_EQ(sampleRecord.sampleData.size(), 1);

  // Verify the FlowSample data
  const auto& flowSample =
      std::get<sflow::FlowSample>(sampleRecord.sampleData[0]);
  EXPECT_EQ(flowSample.sequenceNumber, 12345);
  EXPECT_EQ(flowSample.sourceID, 67890);
  EXPECT_EQ(flowSample.samplingRate, 100);
  EXPECT_EQ(flowSample.samplePool, 200);
  EXPECT_EQ(flowSample.drops, 5);
  EXPECT_EQ(flowSample.input, 42);
  EXPECT_EQ(flowSample.output, 24);
  EXPECT_EQ(flowSample.flowRecords.size(), 1);

  // Verify the FlowRecord
  EXPECT_EQ(flowSample.flowRecords[0].flowFormat, 1);
  EXPECT_EQ(flowSample.flowRecords[0].flowData.size(), 4);
  EXPECT_THAT(
      flowSample.flowRecords[0].flowData, ElementsAre(0x11, 0x22, 0x33, 0x44));
}

TEST(SflowStructsTest, SampleRecordSerializeDeserializeRoundTrip) {
  // Test serialize-deserialize round trip for SampleRecord

  // Create original SampleRecord with FlowSample data
  sflow::SampleRecord original;
  original.sampleType = 1; // FlowSample type

  // Create a FlowSample
  sflow::FlowSample flowSample;
  flowSample.sequenceNumber = 99999;
  flowSample.sourceID = 88888;
  flowSample.samplingRate = 77777;
  flowSample.samplePool = 66666;
  flowSample.drops = 55555;
  flowSample.input = 44444;
  flowSample.output = 33333;

  // Add flow records with different data sizes
  sflow::FlowRecord record1;
  record1.flowFormat = 1;
  record1.flowData = {0xDE, 0xAD, 0xBE, 0xEF}; // 4 bytes, no padding

  sflow::FlowRecord record2;
  record2.flowFormat = 1;
  record2.flowData = {0xCA, 0xFE}; // 2 bytes, 2 bytes padding

  flowSample.flowRecords = {record1, record2};
  original.sampleData.emplace_back(std::move(flowSample));

  // Serialize
  int bufSize = 1024;
  std::vector<uint8_t> buffer(bufSize);
  auto buf = folly::IOBuf::wrapBuffer(buffer.data(), bufSize);
  auto rwCursor = std::make_shared<folly::io::RWPrivateCursor>(buf.get());

  original.serialize(rwCursor.get());
  size_t serializedSize = bufSize - rwCursor->length();

  // Deserialize
  auto readBuf = folly::IOBuf::wrapBuffer(buffer.data(), serializedSize);
  folly::io::Cursor readCursor(readBuf.get());

  sflow::SampleRecord deserialized =
      sflow::SampleRecord::deserialize(readCursor);

  // Verify round-trip correctness
  EXPECT_EQ(deserialized.sampleType, original.sampleType);
  EXPECT_EQ(deserialized.sampleData.size(), original.sampleData.size());

  // Verify the FlowSample data
  const auto& originalFlowSample =
      std::get<sflow::FlowSample>(original.sampleData[0]);
  const auto& deserializedFlowSample =
      std::get<sflow::FlowSample>(deserialized.sampleData[0]);

  EXPECT_EQ(
      deserializedFlowSample.sequenceNumber, originalFlowSample.sequenceNumber);
  EXPECT_EQ(deserializedFlowSample.sourceID, originalFlowSample.sourceID);
  EXPECT_EQ(
      deserializedFlowSample.samplingRate, originalFlowSample.samplingRate);
  EXPECT_EQ(deserializedFlowSample.samplePool, originalFlowSample.samplePool);
  EXPECT_EQ(deserializedFlowSample.drops, originalFlowSample.drops);
  EXPECT_EQ(deserializedFlowSample.input, originalFlowSample.input);
  EXPECT_EQ(deserializedFlowSample.output, originalFlowSample.output);
  EXPECT_EQ(
      deserializedFlowSample.flowRecords.size(),
      originalFlowSample.flowRecords.size());

  // Verify each flow record
  for (size_t i = 0; i < originalFlowSample.flowRecords.size(); ++i) {
    EXPECT_EQ(
        deserializedFlowSample.flowRecords[i].flowFormat,
        originalFlowSample.flowRecords[i].flowFormat)
        << "FlowFormat mismatch for record " << i;
    EXPECT_EQ(
        deserializedFlowSample.flowRecords[i].flowData.size(),
        originalFlowSample.flowRecords[i].flowData.size())
        << "FlowData size mismatch for record " << i;
    EXPECT_THAT(
        deserializedFlowSample.flowRecords[i].flowData,
        ContainerEq(originalFlowSample.flowRecords[i].flowData))
        << "FlowData content mismatch for record " << i;
  }
}

TEST(SflowStructsTest, SampleRecordDeserializeEmptyData) {
  // Test SampleRecord deserialization with empty sample data

  std::vector<uint8_t> serializedData = {// sampleType (1) as big-endian
                                         0x00,
                                         0x00,
                                         0x00,
                                         0x01,
                                         // sampleDataLen (0) as big-endian
                                         0x00,
                                         0x00,
                                         0x00,
                                         0x00};

  auto buf =
      folly::IOBuf::wrapBuffer(serializedData.data(), serializedData.size());
  folly::io::Cursor cursor(buf.get());

  // Deserialize the SampleRecord
  sflow::SampleRecord sampleRecord = sflow::SampleRecord::deserialize(cursor);

  // Verify the deserialized data
  EXPECT_EQ(sampleRecord.sampleType, 1);
  EXPECT_EQ(sampleRecord.sampleData.size(), 0);
}

TEST(SflowStructsTest, SampleRecordDeserializeUnknownSampleType) {
  // Test SampleRecord deserialization with unknown sample type

  std::vector<uint8_t> serializedData = {
      // sampleType (999 = unknown) as big-endian
      0x00,
      0x00,
      0x03,
      0xE7,
      // sampleDataLen (8) as big-endian
      0x00,
      0x00,
      0x00,
      0x08,
      // sample data (8 bytes of dummy data)
      0x11,
      0x22,
      0x33,
      0x44,
      0x55,
      0x66,
      0x77,
      0x88};

  auto buf =
      folly::IOBuf::wrapBuffer(serializedData.data(), serializedData.size());
  folly::io::Cursor cursor(buf.get());

  // Deserialize the SampleRecord
  sflow::SampleRecord sampleRecord = sflow::SampleRecord::deserialize(cursor);

  // Verify the deserialized data - should skip unknown sample type
  EXPECT_EQ(sampleRecord.sampleType, 999);
  EXPECT_EQ(
      sampleRecord.sampleData.size(),
      0); // Should be empty since we skip unknown types
}

TEST(SflowStructsTest, SampleRecord) {
  // Test comprehensive functionality of SampleRecord with detailed
  // serialization verification
  sflow::SampleRecord record;
  record.sampleType = 1; // Flow sample type

  // Create a FlowSample to add to the sample data
  sflow::FlowSample flowSample;
  flowSample.sequenceNumber = 98765; // 0x181CD
  flowSample.sourceID = 54321; // 0xD431
  flowSample.samplingRate = 1000; // 0x3E8
  flowSample.samplePool = 2000; // 0x7D0
  flowSample.drops = 10; // 0xA
  flowSample.input = 123; // 0x7B
  flowSample.output = 456; // 0x1C8

  // Add FlowRecords to the FlowSample
  sflow::FlowRecord record1;
  record1.flowFormat = 1;
  record1.flowData = {0xDE, 0xAD, 0xBE, 0xEF}; // 4 bytes, no padding needed

  sflow::FlowRecord record2;
  record2.flowFormat = 1;
  record2.flowData = {0xCA, 0xFE}; // 2 bytes, will need 2 bytes padding

  flowSample.flowRecords = {record1, record2};

  // Add the FlowSample to the SampleRecord
  record.sampleData.push_back(flowSample);

  uint32_t expectedSize = 4 /* sampleType */ + 4 /* sampleDataLen */ +
      std::get<FlowSample>(record.sampleData[0]).size();
  EXPECT_EQ(record.size(), expectedSize);

  // Test serialization
  int bufSize = 1024;
  std::vector<uint8_t> buffer(bufSize);
  auto buf = folly::IOBuf::wrapBuffer(buffer.data(), bufSize);
  auto cursor = std::make_shared<folly::io::RWPrivateCursor>(buf.get());

  record.serialize(cursor.get());
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
  EXPECT_EQ(record.size(), serializedSize);
}

TEST(SflowStructsTest, SampleRecordMultipleFlowSamples) {
  // Test SampleRecord with multiple FlowSample data
  sflow::SampleRecord record;
  record.sampleType = 1;

  // Create first FlowSample
  sflow::FlowSample flowSample1;
  flowSample1.sequenceNumber = 1111;
  flowSample1.sourceID = 2222;
  flowSample1.samplingRate = 3333;
  flowSample1.samplePool = 4444;
  flowSample1.drops = 5555;
  flowSample1.input = 6666;
  flowSample1.output = 7777;

  sflow::FlowRecord record1;
  record1.flowFormat = 1;
  record1.flowData = {0x11, 0x22}; // 2 bytes + 2 padding = 4 aligned
  flowSample1.flowRecords = {record1};

  // Create second FlowSample
  sflow::FlowSample flowSample2;
  flowSample2.sequenceNumber = 8888;
  flowSample2.sourceID = 9999;
  flowSample2.samplingRate = 1010;
  flowSample2.samplePool = 2020;
  flowSample2.drops = 3030;
  flowSample2.input = 4040;
  flowSample2.output = 5050;

  sflow::FlowRecord record2;
  record2.flowFormat = 1;
  record2.flowData = {0xAA, 0xBB, 0xCC}; // 3 bytes + 1 padding = 4 aligned
  flowSample2.flowRecords = {record2};

  // Add both FlowSamples to the SampleRecord
  record.sampleData.push_back(flowSample1);
  record.sampleData.push_back(flowSample2);

  // Test size calculation - should include both FlowSamples
  uint32_t expectedFlowSample1Size = 32 + 12; // 32 fixed + 12 for record1
  uint32_t expectedFlowSample2Size = 32 + 12; // 32 fixed + 12 for record2
  uint32_t expectedTotalSize = 4 /* sampleType */ + 4 /* sampleDataLen */ +
      expectedFlowSample1Size + expectedFlowSample2Size;
  EXPECT_EQ(record.size(), expectedTotalSize);

  // Test serialization
  int bufSize = 1024;
  std::vector<uint8_t> buffer(bufSize);
  auto buf = folly::IOBuf::wrapBuffer(buffer.data(), bufSize);
  auto cursor = std::make_shared<folly::io::RWPrivateCursor>(buf.get());

  record.serialize(cursor.get());
  size_t serializedSize = bufSize - cursor->length();

  // Verify that serialization size matches calculated size
  EXPECT_EQ(record.size(), serializedSize);
  EXPECT_EQ(record.sampleData.size(), 2);
}

TEST(SflowStructsTest, SampleRecordSizeCalculation) {
  // Test size calculation with various configurations
  sflow::SampleRecord record;
  record.sampleType = 1;

  // Test empty sample data
  EXPECT_EQ(record.size(), 8); // 4 + 4 + 0

  // Add one FlowSample
  sflow::FlowSample flowSample1;
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
  sflow::FlowSample flowSample2;
  flowSample2.sequenceNumber = 10;
  flowSample2.sourceID = 20;
  flowSample2.samplingRate = 30;
  flowSample2.samplePool = 40;
  flowSample2.drops = 50;
  flowSample2.input = 60;
  flowSample2.output = 70;

  // Add a FlowRecord to the second sample
  sflow::FlowRecord flowRecord;
  flowRecord.flowFormat = 1;
  flowRecord.flowData = {
      0x01, 0x02, 0x03, 0x04, 0x05}; // 5 bytes + 3 padding = 8
  flowSample2.flowRecords = {flowRecord};

  record.sampleData.emplace_back(flowSample2);
  uint32_t expectedSize2 = 8 + flowSample1.size() + flowSample2.size();
  EXPECT_EQ(record.size(), expectedSize2);
}

TEST(SflowStructsTest, SampleDatagramV5Deserialization) {
  // Test SampleDatagramV5 deserialization functionality

  // Create a buffer with serialized SampleDatagramV5 data
  std::vector<uint8_t> serializedData = {
      // IP address type (IPv6 = 2) as big-endian
      0x00,
      0x00,
      0x00,
      0x02,
      // IPv6 address: 2001:db8::1 (16 bytes)
      0x20,
      0x01,
      0x0d,
      0xb8,
      0x00,
      0x00,
      0x00,
      0x00,
      0x00,
      0x00,
      0x00,
      0x00,
      0x00,
      0x00,
      0x00,
      0x01,
      // subAgentID (12345) as big-endian
      0x00,
      0x00,
      0x30,
      0x39,
      // sequenceNumber (67890) as big-endian
      0x00,
      0x01,
      0x09,
      0x32,
      // uptime (98765) as big-endian
      0x00,
      0x01,
      0x81,
      0xcd,
      // samplesCount (1) as big-endian
      0x00,
      0x00,
      0x00,
      0x01,

      // SampleRecord:
      // sampleType (1) as big-endian
      0x00,
      0x00,
      0x00,
      0x01,
      // sampleDataLen (32 bytes) as big-endian - size of the FlowSample below
      0x00,
      0x00,
      0x00,
      0x20,

      // FlowSample data (32 bytes total - no flow records):
      // sequenceNumber (111) as big-endian
      0x00,
      0x00,
      0x00,
      0x6f,
      // sourceID (222) as big-endian
      0x00,
      0x00,
      0x00,
      0xde,
      // samplingRate (333) as big-endian
      0x00,
      0x00,
      0x01,
      0x4d,
      // samplePool (444) as big-endian
      0x00,
      0x00,
      0x01,
      0xbc,
      // drops (555) as big-endian
      0x00,
      0x00,
      0x02,
      0x2b,
      // input (666) as big-endian
      0x00,
      0x00,
      0x02,
      0x9a,
      // output (777) as big-endian
      0x00,
      0x00,
      0x03,
      0x09,
      // flowRecordsCnt (0) as big-endian
      0x00,
      0x00,
      0x00,
      0x00};

  auto buf =
      folly::IOBuf::wrapBuffer(serializedData.data(), serializedData.size());
  folly::io::Cursor cursor(buf.get());

  // Deserialize the SampleDatagramV5
  sflow::SampleDatagramV5 datagram =
      sflow::SampleDatagramV5::deserialize(cursor);

  // Verify the deserialized data
  EXPECT_EQ(datagram.agentAddress, folly::IPAddress("2001:db8::1"));
  EXPECT_EQ(datagram.subAgentID, 12345);
  EXPECT_EQ(datagram.sequenceNumber, 67890);
  EXPECT_EQ(datagram.uptime, 98765);
  EXPECT_EQ(datagram.samples.size(), 1);

  // Verify the SampleRecord
  const auto& sampleRecord = datagram.samples[0];
  EXPECT_EQ(sampleRecord.sampleType, 1);
  EXPECT_EQ(sampleRecord.sampleData.size(), 1);

  // Verify the FlowSample data
  const auto& flowSample =
      std::get<sflow::FlowSample>(sampleRecord.sampleData[0]);
  EXPECT_EQ(flowSample.sequenceNumber, 111);
  EXPECT_EQ(flowSample.sourceID, 222);
  EXPECT_EQ(flowSample.samplingRate, 333);
  EXPECT_EQ(flowSample.samplePool, 444);
  EXPECT_EQ(flowSample.drops, 555);
  EXPECT_EQ(flowSample.input, 666);
  EXPECT_EQ(flowSample.output, 777);
  EXPECT_EQ(flowSample.flowRecords.size(), 0);
}

TEST(SflowStructsTest, SampleDatagramV5DeserializationIPv4) {
  // Test SampleDatagramV5 deserialization with IPv4 address

  std::vector<uint8_t> serializedData = {
      // IP address type (IPv4 = 1) as big-endian
      0x00,
      0x00,
      0x00,
      0x01,
      // IPv4 address: 192.168.1.100 (4 bytes)
      0xc0,
      0xa8,
      0x01,
      0x64,
      // subAgentID (1000) as big-endian
      0x00,
      0x00,
      0x03,
      0xe8,
      // sequenceNumber (2000) as big-endian
      0x00,
      0x00,
      0x07,
      0xd0,
      // uptime (3000) as big-endian
      0x00,
      0x00,
      0x0b,
      0xb8,
      // samplesCount (0) as big-endian
      0x00,
      0x00,
      0x00,
      0x00};

  auto buf =
      folly::IOBuf::wrapBuffer(serializedData.data(), serializedData.size());
  folly::io::Cursor cursor(buf.get());

  // Deserialize the SampleDatagramV5
  sflow::SampleDatagramV5 datagram =
      sflow::SampleDatagramV5::deserialize(cursor);

  // Verify the deserialized data
  EXPECT_EQ(datagram.agentAddress, folly::IPAddress("192.168.1.100"));
  EXPECT_EQ(datagram.subAgentID, 1000);
  EXPECT_EQ(datagram.sequenceNumber, 2000);
  EXPECT_EQ(datagram.uptime, 3000);
  EXPECT_EQ(datagram.samples.size(), 0);
}

TEST(SflowStructsTest, SampleDatagramV5SerializeDeserializeRoundTrip) {
  // Test serialize-deserialize round trip for SampleDatagramV5

  // Create original SampleDatagramV5 with complex data
  sflow::SampleDatagramV5 original;
  original.agentAddress = folly::IPAddress("2401:db00:116:3016::1b");
  original.subAgentID = 99999;
  original.sequenceNumber = 88888;
  original.uptime = 77777;

  // Create first sample record
  sflow::SampleRecord record1;
  record1.sampleType = 1;

  sflow::FlowSample flowSample1;
  flowSample1.sequenceNumber = 1111;
  flowSample1.sourceID = 2222;
  flowSample1.samplingRate = 3333;
  flowSample1.samplePool = 4444;
  flowSample1.drops = 5555;
  flowSample1.input = 6666;
  flowSample1.output = 7777;

  sflow::FlowRecord flowRecord1;
  flowRecord1.flowFormat = 1;
  flowRecord1.flowData = {0xAA, 0xBB, 0xCC, 0xDD}; // 4 bytes, no padding

  flowSample1.flowRecords = {flowRecord1};
  record1.sampleData.emplace_back(std::move(flowSample1));

  // Create second sample record
  sflow::SampleRecord record2;
  record2.sampleType = 1;

  sflow::FlowSample flowSample2;
  flowSample2.sequenceNumber = 8888;
  flowSample2.sourceID = 9999;
  flowSample2.samplingRate = 1010;
  flowSample2.samplePool = 2020;
  flowSample2.drops = 3030;
  flowSample2.input = 4040;
  flowSample2.output = 5050;

  sflow::FlowRecord flowRecord2;
  flowRecord2.flowFormat = 1;
  flowRecord2.flowData = {0xEE, 0xFF}; // 2 bytes, 2 bytes padding

  flowSample2.flowRecords = {flowRecord2};
  record2.sampleData.emplace_back(std::move(flowSample2));

  original.samples = {record1, record2};

  // Serialize
  int bufSize = 1024;
  std::vector<uint8_t> buffer(bufSize);
  auto buf = folly::IOBuf::wrapBuffer(buffer.data(), bufSize);
  auto rwCursor = std::make_shared<folly::io::RWPrivateCursor>(buf.get());

  original.serialize(rwCursor.get());
  size_t serializedSize = bufSize - rwCursor->length();

  // Deserialize
  auto readBuf = folly::IOBuf::wrapBuffer(buffer.data(), serializedSize);
  folly::io::Cursor readCursor(readBuf.get());

  sflow::SampleDatagramV5 deserialized =
      sflow::SampleDatagramV5::deserialize(readCursor);

  // Verify round-trip correctness
  EXPECT_EQ(deserialized.agentAddress, original.agentAddress);
  EXPECT_EQ(deserialized.subAgentID, original.subAgentID);
  EXPECT_EQ(deserialized.sequenceNumber, original.sequenceNumber);
  EXPECT_EQ(deserialized.uptime, original.uptime);
  EXPECT_EQ(deserialized.samples.size(), original.samples.size());

  // Verify each sample record
  for (size_t i = 0; i < original.samples.size(); ++i) {
    const auto& originalSample = original.samples[i];
    const auto& deserializedSample = deserialized.samples[i];

    EXPECT_EQ(deserializedSample.sampleType, originalSample.sampleType)
        << "SampleType mismatch for sample " << i;
    EXPECT_EQ(
        deserializedSample.sampleData.size(), originalSample.sampleData.size())
        << "SampleData size mismatch for sample " << i;

    // Verify FlowSample data
    const auto& originalFlowSample =
        std::get<sflow::FlowSample>(originalSample.sampleData[0]);
    const auto& deserializedFlowSample =
        std::get<sflow::FlowSample>(deserializedSample.sampleData[0]);

    EXPECT_EQ(
        deserializedFlowSample.sequenceNumber,
        originalFlowSample.sequenceNumber)
        << "FlowSample sequenceNumber mismatch for sample " << i;
    EXPECT_EQ(deserializedFlowSample.sourceID, originalFlowSample.sourceID)
        << "FlowSample sourceID mismatch for sample " << i;
    EXPECT_EQ(
        deserializedFlowSample.samplingRate, originalFlowSample.samplingRate)
        << "FlowSample samplingRate mismatch for sample " << i;
    EXPECT_EQ(deserializedFlowSample.samplePool, originalFlowSample.samplePool)
        << "FlowSample samplePool mismatch for sample " << i;
    EXPECT_EQ(deserializedFlowSample.drops, originalFlowSample.drops)
        << "FlowSample drops mismatch for sample " << i;
    EXPECT_EQ(deserializedFlowSample.input, originalFlowSample.input)
        << "FlowSample input mismatch for sample " << i;
    EXPECT_EQ(deserializedFlowSample.output, originalFlowSample.output)
        << "FlowSample output mismatch for sample " << i;
    EXPECT_EQ(
        deserializedFlowSample.flowRecords.size(),
        originalFlowSample.flowRecords.size())
        << "FlowRecords count mismatch for sample " << i;

    // Verify each flow record
    for (size_t j = 0; j < originalFlowSample.flowRecords.size(); ++j) {
      EXPECT_EQ(
          deserializedFlowSample.flowRecords[j].flowFormat,
          originalFlowSample.flowRecords[j].flowFormat)
          << "FlowRecord format mismatch for sample " << i << ", record " << j;
      EXPECT_THAT(
          deserializedFlowSample.flowRecords[j].flowData,
          ContainerEq(originalFlowSample.flowRecords[j].flowData))
          << "FlowRecord data mismatch for sample " << i << ", record " << j;
    }
  }
}

TEST(SflowStructsTest, SampleDatagramV5DeserializeMultipleSamples) {
  // Test SampleDatagramV5 deserialization with multiple sample records

  // Create a complex SampleDatagramV5 with multiple samples
  sflow::SampleDatagramV5 original;
  original.agentAddress = folly::IPAddress("10.0.0.1");
  original.subAgentID = 12345;
  original.sequenceNumber = 67890;
  original.uptime = 98765;

  // Create first sample
  sflow::SampleRecord record1;
  record1.sampleType = 1;

  sflow::FlowSample flowSample1;
  flowSample1.sequenceNumber = 111;
  flowSample1.sourceID = 222;
  flowSample1.samplingRate = 333;
  flowSample1.samplePool = 444;
  flowSample1.drops = 555;
  flowSample1.input = 666;
  flowSample1.output = 777;

  sflow::FlowRecord flowRecord1;
  flowRecord1.flowFormat = 1;
  flowRecord1.flowData = {0x11, 0x22}; // 2 bytes + 2 padding
  flowSample1.flowRecords = {flowRecord1};

  record1.sampleData.emplace_back(std::move(flowSample1));

  // Create second sample
  sflow::SampleRecord record2;
  record2.sampleType = 1;

  sflow::FlowSample flowSample2;
  flowSample2.sequenceNumber = 888;
  flowSample2.sourceID = 999;
  flowSample2.samplingRate = 1010;
  flowSample2.samplePool = 2020;
  flowSample2.drops = 3030;
  flowSample2.input = 4040;
  flowSample2.output = 5050;

  sflow::FlowRecord flowRecord2;
  flowRecord2.flowFormat = 1;
  flowRecord2.flowData = {0xAA, 0xBB, 0xCC}; // 3 bytes + 1 padding
  flowSample2.flowRecords = {flowRecord2};

  record2.sampleData.emplace_back(std::move(flowSample2));

  original.samples = {record1, record2};

  // Serialize
  int bufSize = 1024;
  std::vector<uint8_t> buffer(bufSize);
  auto buf = folly::IOBuf::wrapBuffer(buffer.data(), bufSize);
  auto rwCursor = std::make_shared<folly::io::RWPrivateCursor>(buf.get());

  original.serialize(rwCursor.get());
  size_t serializedSize = bufSize - rwCursor->length();

  // Deserialize
  auto readBuf = folly::IOBuf::wrapBuffer(buffer.data(), serializedSize);
  folly::io::Cursor readCursor(readBuf.get());
  sflow::SampleDatagramV5 deserialized =
      sflow::SampleDatagramV5::deserialize(readCursor);

  // Verify the deserialized data
  EXPECT_EQ(deserialized.agentAddress, original.agentAddress);
  EXPECT_EQ(deserialized.subAgentID, original.subAgentID);
  EXPECT_EQ(deserialized.sequenceNumber, original.sequenceNumber);
  EXPECT_EQ(deserialized.uptime, original.uptime);
  EXPECT_EQ(deserialized.samples.size(), 2);

  // Verify first sample
  const auto& sample1 = deserialized.samples[0];
  EXPECT_EQ(sample1.sampleType, 1);
  EXPECT_EQ(sample1.sampleData.size(), 1);

  const auto& flow1 = std::get<sflow::FlowSample>(sample1.sampleData[0]);
  EXPECT_EQ(flow1.sequenceNumber, 111);
  EXPECT_EQ(flow1.sourceID, 222);
  EXPECT_EQ(flow1.flowRecords.size(), 1);
  EXPECT_THAT(flow1.flowRecords[0].flowData, ElementsAre(0x11, 0x22));

  // Verify second sample
  const auto& sample2 = deserialized.samples[1];
  EXPECT_EQ(sample2.sampleType, 1);
  EXPECT_EQ(sample2.sampleData.size(), 1);

  const auto& flow2 = std::get<sflow::FlowSample>(sample2.sampleData[0]);
  EXPECT_EQ(flow2.sequenceNumber, 888);
  EXPECT_EQ(flow2.sourceID, 999);
  EXPECT_EQ(flow2.flowRecords.size(), 1);
  EXPECT_THAT(flow2.flowRecords[0].flowData, ElementsAre(0xAA, 0xBB, 0xCC));
}

TEST(SflowStructsTest, SampleDatagramV5) {
  // Test comprehensive functionality of SampleDatagramV5 with detailed
  // serialization verification
  sflow::SampleDatagramV5 datagram;
  datagram.agentAddress = folly::IPAddress("2401:db00:116:3016::1b");
  datagram.subAgentID = 12345; // 0x3039
  datagram.sequenceNumber = 67890; // 0x10932
  datagram.uptime = 9876543; // 0x96B34F

  // Create a SampleRecord with FlowSample data
  sflow::SampleRecord sampleRecord;
  sampleRecord.sampleType = 1; // Flow sample type

  // Create a FlowSample
  sflow::FlowSample flowSample;
  flowSample.sequenceNumber = 111; // 0x6F
  flowSample.sourceID = 222; // 0xDE
  flowSample.samplingRate = 333; // 0x14D
  flowSample.samplePool = 444; // 0x1BC
  flowSample.drops = 555; // 0x22B
  flowSample.input = 666; // 0x29A
  flowSample.output = 777; // 0x309

  // Add FlowRecords to the FlowSample
  sflow::FlowRecord record1;
  record1.flowFormat = 1;
  record1.flowData = {0x11, 0x22, 0x33, 0x44}; // 4 bytes, no padding needed

  sflow::FlowRecord record2;
  record2.flowFormat = 1;
  record2.flowData = {0xAA, 0xBB}; // 2 bytes, will need 2 bytes padding

  flowSample.flowRecords = {record1, record2};

  // Add FlowSample to SampleRecord
  sampleRecord.sampleData.push_back(flowSample);

  // Add SampleRecord to SampleDatagramV5
  datagram.samples.push_back(std::move(sampleRecord));

  // Test size calculation
  // Calculate expected size: IP address + subAgentID + sequenceNumber + uptime
  // + samplesCnt + sample size
  uint32_t expectedSize = 4 /* IP address type */ +
      datagram.agentAddress.byteCount() + 4 /* subAgentID */ +
      4 /* sequenceNumber */ + 4 /* uptime */ + 4 /* samplesCnt */ +
      datagram.samples[0].size();
  EXPECT_EQ(datagram.size(), expectedSize);

  // Test serialization
  int bufSize = 1024;
  std::vector<uint8_t> buffer(bufSize);
  auto buf = folly::IOBuf::wrapBuffer(buffer.data(), bufSize);
  auto cursor = std::make_shared<folly::io::RWPrivateCursor>(buf.get());

  datagram.serialize(cursor.get());
  size_t serializedSize = bufSize - cursor->length();

  // Verify the complete serialized buffer content byte by byte
  std::vector<uint8_t> actualData(
      buffer.begin(), buffer.begin() + serializedSize);
  EXPECT_THAT(
      actualData,
      ElementsAre(
          // IP address type (IPv6 = 2) as big-endian
          0x00,
          0x00,
          0x00,
          0x02,
          // IPv6 address: 2401:db00:116:3016::1b (16 bytes)
          0x24,
          0x01,
          0xdb,
          0x00,
          0x01,
          0x16,
          0x30,
          0x16,
          0x00,
          0x00,
          0x00,
          0x00,
          0x00,
          0x00,
          0x00,
          0x1b,
          // subAgentID (12345 = 0x3039) as big-endian
          0x00,
          0x00,
          0x30,
          0x39,
          // sequenceNumber (67890 = 0x10932) as big-endian
          0x00,
          0x01,
          0x09,
          0x32,
          // uptime (9876543 = 0x96B34F) as big-endian
          0x00,
          0x96,
          0xB4,
          0x3F,
          // samplesCnt (1) as big-endian
          0x00,
          0x00,
          0x00,
          0x01,

          // SampleRecord:
          // sampleType (1) as big-endian
          0x00,
          0x00,
          0x00,
          0x01,
          // sampleDataLen (56 bytes) as big-endian
          0x00,
          0x00,
          0x00,
          0x38,

          // FlowSample data:
          // sequenceNumber (111 = 0x6F) as big-endian
          0x00,
          0x00,
          0x00,
          0x6F,
          // sourceID (222 = 0xDE) as big-endian
          0x00,
          0x00,
          0x00,
          0xDE,
          // samplingRate (333 = 0x14D) as big-endian
          0x00,
          0x00,
          0x01,
          0x4D,
          // samplePool (444 = 0x1BC) as big-endian
          0x00,
          0x00,
          0x01,
          0xBC,
          // drops (555 = 0x22B) as big-endian
          0x00,
          0x00,
          0x02,
          0x2B,
          // input (666 = 0x29A) as big-endian
          0x00,
          0x00,
          0x02,
          0x9A,
          // output (777 = 0x309) as big-endian
          0x00,
          0x00,
          0x03,
          0x09,
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
  EXPECT_EQ(datagram.size(), serializedSize);
}

TEST(SflowStructsTest, SampleDatagramV5SizeCalculation) {
  // Test size calculation with various numbers of sample records
  sflow::SampleDatagramV5 datagram;
  datagram.agentAddress = folly::IPAddress("192.168.1.100");
  datagram.subAgentID = 1;
  datagram.sequenceNumber = 2;
  datagram.uptime = 3;

  // Test empty samples
  uint32_t baseSize = 4 /* IP address type */ +
      datagram.agentAddress.byteCount() + 4 + 4 + 4 + 4;
  EXPECT_EQ(datagram.size(), baseSize);

  // Add one sample record
  sflow::SampleRecord record1;
  record1.sampleType = 1;

  // Add a FlowSample with FlowRecords
  sflow::FlowSample flowSample1;
  flowSample1.sequenceNumber = 100;
  flowSample1.sourceID = 200;
  flowSample1.samplingRate = 300;
  flowSample1.samplePool = 400;
  flowSample1.drops = 500;
  flowSample1.input = 600;
  flowSample1.output = 700;

  sflow::FlowRecord flowRecord1;
  flowRecord1.flowFormat = 1;
  flowRecord1.flowData = {0x01, 0x02, 0x03}; // 3 bytes + 1 padding = 4
  flowSample1.flowRecords = {flowRecord1};

  record1.sampleData.push_back(flowSample1);
  datagram.samples.push_back(std::move(record1));

  uint32_t expectedSize1 = baseSize + datagram.samples[0].size();
  EXPECT_EQ(datagram.size(), expectedSize1);

  // Add another sample record
  sflow::SampleRecord record2;
  record2.sampleType = 2;

  sflow::FlowSample flowSample2;
  flowSample2.sequenceNumber = 1000;
  flowSample2.sourceID = 2000;
  flowSample2.samplingRate = 3000;
  flowSample2.samplePool = 4000;
  flowSample2.drops = 5000;
  flowSample2.input = 6000;
  flowSample2.output = 7000;

  sflow::FlowRecord flowRecord2;
  flowRecord2.flowFormat = 1;
  flowRecord2.flowData = {
      0x04, 0x05, 0x06, 0x07, 0x08, 0x09}; // 6 bytes + 2 padding = 8
  flowSample2.flowRecords = {flowRecord2};

  record2.sampleData.push_back(flowSample2);
  datagram.samples.push_back(std::move(record2));

  uint32_t expectedSize2 =
      baseSize + datagram.samples[0].size() + datagram.samples[1].size();
  EXPECT_EQ(datagram.size(), expectedSize2);
}

TEST(SflowStructsTest, SampleDatagramV5MultipleSamples) {
  // Test SampleDatagramV5 with multiple sample records
  sflow::SampleDatagramV5 datagram;
  datagram.agentAddress = folly::IPAddress("10.0.0.1");
  datagram.subAgentID = 11111;
  datagram.sequenceNumber = 22222;
  datagram.uptime = 33333;

  // Create first sample record
  sflow::SampleRecord record1;
  record1.sampleType = 1;

  sflow::FlowSample flowSample1;
  flowSample1.sequenceNumber = 111;
  flowSample1.sourceID = 222;
  flowSample1.samplingRate = 333;
  flowSample1.samplePool = 444;
  flowSample1.drops = 555;
  flowSample1.input = 666;
  flowSample1.output = 777;

  sflow::FlowRecord flowRecord1;
  flowRecord1.flowFormat = 1;
  flowRecord1.flowData = {0x11, 0x22}; // 2 bytes + 2 padding = 4
  flowSample1.flowRecords = {flowRecord1};

  record1.sampleData.push_back(flowSample1);

  // Create second sample record
  sflow::SampleRecord record2;
  record2.sampleType = 2;

  sflow::FlowSample flowSample2;
  flowSample2.sequenceNumber = 888;
  flowSample2.sourceID = 999;
  flowSample2.samplingRate = 1010;
  flowSample2.samplePool = 2020;
  flowSample2.drops = 3030;
  flowSample2.input = 4040;
  flowSample2.output = 5050;

  sflow::FlowRecord flowRecord2;
  flowRecord2.flowFormat = 1;
  flowRecord2.flowData = {0xAA, 0xBB, 0xCC}; // 3 bytes + 1 padding = 4
  flowSample2.flowRecords = {flowRecord2};

  record2.sampleData.push_back(flowSample2);

  // Add both records to datagram
  datagram.samples.push_back(std::move(record1));
  datagram.samples.push_back(std::move(record2));

  // Test size calculation - should include both sample records
  uint32_t baseSize = 4 /* IP address type */ +
      datagram.agentAddress.byteCount() + 4 + 4 + 4 + 4;
  uint32_t expectedTotalSize =
      baseSize + datagram.samples[0].size() + datagram.samples[1].size();
  EXPECT_EQ(datagram.size(), expectedTotalSize);

  // Test serialization
  int bufSize = 1024;
  std::vector<uint8_t> buffer(bufSize);
  auto buf = folly::IOBuf::wrapBuffer(buffer.data(), bufSize);
  auto cursor = std::make_shared<folly::io::RWPrivateCursor>(buf.get());

  datagram.serialize(cursor.get());
  size_t serializedSize = bufSize - cursor->length();

  // Verify that serialization size matches calculated size
  EXPECT_EQ(datagram.size(), serializedSize);
  EXPECT_EQ(datagram.samples.size(), 2);

  // Verify basic structure integrity
  EXPECT_GT(
      serializedSize, 50); // Should be a substantial size with all the data
}

TEST(SflowStructsTest, SampleDatagram) {
  // Test comprehensive functionality of SampleDatagram with detailed
  // serialization verification
  sflow::SampleDatagram datagram;
  datagram.datagramV5.agentAddress = folly::IPAddress("2401:db00:116:3016::1b");
  datagram.datagramV5.subAgentID = 54321; // 0xD431
  datagram.datagramV5.sequenceNumber = 98765; // 0x181CD
  datagram.datagramV5.uptime = 1234567; // 0x12D687

  // Create a SampleRecord with FlowSample data
  sflow::SampleRecord sampleRecord;
  sampleRecord.sampleType = 1; // Flow sample type

  // Create a FlowSample
  sflow::FlowSample flowSample;
  flowSample.sequenceNumber = 555; // 0x22B
  flowSample.sourceID = 666; // 0x29A
  flowSample.samplingRate = 777; // 0x309
  flowSample.samplePool = 888; // 0x378
  flowSample.drops = 999; // 0x3E7
  flowSample.input = 1111; // 0x457
  flowSample.output = 2222; // 0x8AE

  // Add FlowRecords to the FlowSample
  sflow::FlowRecord record1;
  record1.flowFormat = 1;
  record1.flowData = {0xDE, 0xAD, 0xBE, 0xEF}; // 4 bytes, no padding needed

  sflow::FlowRecord record2;
  record2.flowFormat = 1;
  record2.flowData = {0xCA, 0xFE, 0xBA, 0xBE, 0x12}; // 5 bytes, 3 bytes padding

  flowSample.flowRecords = {record1, record2};

  // Add FlowSample to SampleRecord
  sampleRecord.sampleData.push_back(flowSample);

  // Add SampleRecord to SampleDatagramV5
  datagram.datagramV5.samples.push_back(std::move(sampleRecord));

  // Test size calculation
  // Calculate expected size: VERSION5 (4) + datagramV5 size
  uint32_t expectedSize = 4 /* VERSION5 */ + datagram.datagramV5.size();
  EXPECT_EQ(datagram.size(), expectedSize);

  // Test serialization
  int bufSize = 1024;
  std::vector<uint8_t> buffer(bufSize);
  auto buf = folly::IOBuf::wrapBuffer(buffer.data(), bufSize);
  auto cursor = std::make_shared<folly::io::RWPrivateCursor>(buf.get());

  datagram.serialize(cursor.get());
  size_t serializedSize = bufSize - cursor->length();

  // Verify basic structure - should include VERSION5 and all datagram data
  EXPECT_GT(serializedSize, 60); // At least VERSION5 + IP header + other fields

  // Verify the first 8 bytes (VERSION5 and IP address type)
  std::vector<uint8_t> headerData(buffer.begin(), buffer.begin() + 8);
  EXPECT_THAT(
      headerData,
      ElementsAre(
          // VERSION5 (5) as big-endian
          0x00,
          0x00,
          0x00,
          0x05,
          // IP address type (IPv6 = 2) as big-endian
          0x00,
          0x00,
          0x00,
          0x02));

  // Verify that the size calculation matches the actual serialized size
  EXPECT_EQ(datagram.size(), serializedSize);
}

TEST(SflowStructsTest, SampleDatagramSizeCalculation) {
  // Test size calculation with various configurations
  sflow::SampleDatagram datagram;
  datagram.datagramV5.agentAddress = folly::IPAddress("192.168.100.200");
  datagram.datagramV5.subAgentID = 11;
  datagram.datagramV5.sequenceNumber = 22;
  datagram.datagramV5.uptime = 33;

  // Test empty samples
  uint32_t expectedEmptySize = 4 /* VERSION5 */ + datagram.datagramV5.size();
  EXPECT_EQ(datagram.size(), expectedEmptySize);

  // Add one sample record with one FlowSample
  sflow::SampleRecord record1;
  record1.sampleType = 1;

  sflow::FlowSample flowSample1;
  flowSample1.sequenceNumber = 1000;
  flowSample1.sourceID = 2000;
  flowSample1.samplingRate = 3000;
  flowSample1.samplePool = 4000;
  flowSample1.drops = 5000;
  flowSample1.input = 6000;
  flowSample1.output = 7000;

  sflow::FlowRecord flowRecord1;
  flowRecord1.flowFormat = 1;
  flowRecord1.flowData = {0x01, 0x02, 0x03, 0x04}; // 4 bytes, no padding
  flowSample1.flowRecords = {flowRecord1};

  record1.sampleData.push_back(flowSample1);
  datagram.datagramV5.samples.push_back(std::move(record1));

  uint32_t expectedSize1 = 4 /* VERSION5 */ + datagram.datagramV5.size();
  EXPECT_EQ(datagram.size(), expectedSize1);

  // Add another sample record
  sflow::SampleRecord record2;
  record2.sampleType = 2;

  sflow::FlowSample flowSample2;
  flowSample2.sequenceNumber = 8000;
  flowSample2.sourceID = 9000;
  flowSample2.samplingRate = 10000;
  flowSample2.samplePool = 11000;
  flowSample2.drops = 12000;
  flowSample2.input = 13000;
  flowSample2.output = 14000;

  sflow::FlowRecord flowRecord2;
  flowRecord2.flowFormat = 1;
  flowRecord2.flowData = {0x10, 0x20, 0x30}; // 3 bytes + 1 padding = 4
  flowSample2.flowRecords = {flowRecord2};

  record2.sampleData.push_back(flowSample2);
  datagram.datagramV5.samples.push_back(std::move(record2));

  uint32_t expectedSize2 = 4 /* VERSION5 */ + datagram.datagramV5.size();
  EXPECT_EQ(datagram.size(), expectedSize2);
}

TEST(SflowStructsTest, SampleDatagramMultipleSamples) {
  // Test SampleDatagram with multiple sample records and complex structure
  sflow::SampleDatagram datagram;
  datagram.datagramV5.agentAddress = folly::IPAddress("10.1.2.3");
  datagram.datagramV5.subAgentID = 99999;
  datagram.datagramV5.sequenceNumber = 88888;
  datagram.datagramV5.uptime = 77777;

  // Create first sample record with multiple FlowRecords
  sflow::SampleRecord record1;
  record1.sampleType = 1;

  sflow::FlowSample flowSample1;
  flowSample1.sequenceNumber = 1111;
  flowSample1.sourceID = 2222;
  flowSample1.samplingRate = 3333;
  flowSample1.samplePool = 4444;
  flowSample1.drops = 5555;
  flowSample1.input = 6666;
  flowSample1.output = 7777;

  sflow::FlowRecord flowRecord1a;
  flowRecord1a.flowFormat = 1;
  flowRecord1a.flowData = {0xAA, 0xBB}; // 2 bytes + 2 padding = 4

  sflow::FlowRecord flowRecord1b;
  flowRecord1b.flowFormat = 1;
  flowRecord1b.flowData = {
      0xCC, 0xDD, 0xEE, 0xFF, 0x11, 0x22}; // 6 bytes + 2 padding = 8

  flowSample1.flowRecords = {flowRecord1a, flowRecord1b};
  record1.sampleData.push_back(flowSample1);

  // Create second sample record
  sflow::SampleRecord record2;
  record2.sampleType = 2;

  sflow::FlowSample flowSample2;
  flowSample2.sequenceNumber = 9999;
  flowSample2.sourceID = 8888;
  flowSample2.samplingRate = 7777;
  flowSample2.samplePool = 6666;
  flowSample2.drops = 5555;
  flowSample2.input = 4444;
  flowSample2.output = 3333;

  sflow::FlowRecord flowRecord2;
  flowRecord2.flowFormat = 1;
  flowRecord2.flowData = {
      0x33, 0x44, 0x55, 0x66, 0x77}; // 5 bytes + 3 padding = 8
  flowSample2.flowRecords = {flowRecord2};

  record2.sampleData.push_back(flowSample2);

  // Add both records to datagram
  datagram.datagramV5.samples.push_back(std::move(record1));
  datagram.datagramV5.samples.push_back(std::move(record2));

  // Test size calculation - should include both sample records
  uint32_t expectedTotalSize = 4 /* VERSION5 */ + datagram.datagramV5.size();
  EXPECT_EQ(datagram.size(), expectedTotalSize);

  // Test serialization
  int bufSize = 2048;
  std::vector<uint8_t> buffer(bufSize);
  auto buf = folly::IOBuf::wrapBuffer(buffer.data(), bufSize);
  auto cursor = std::make_shared<folly::io::RWPrivateCursor>(buf.get());

  datagram.serialize(cursor.get());
  size_t serializedSize = bufSize - cursor->length();

  // Verify that serialization size matches calculated size
  EXPECT_EQ(datagram.size(), serializedSize);
  EXPECT_EQ(datagram.datagramV5.samples.size(), 2);

  // Verify basic structure integrity - should be substantial with all the
  // nested data
  EXPECT_GT(
      serializedSize,
      100); // Should be a large size with complex nested structure

  // Verify the VERSION5 constant at the beginning
  EXPECT_EQ(datagram.VERSION5, 5);
  uint32_t actualVersion5 =
      (buffer[0] << 24) | (buffer[1] << 16) | (buffer[2] << 8) | buffer[3];
  EXPECT_EQ(actualVersion5, 5);
}

TEST(SflowStructsTest, SampleDatagramDeserialization) {
  // Test SampleDatagram deserialization functionality

  // Create a buffer with serialized SampleDatagram data
  std::vector<uint8_t> serializedData = {
      // sFlow version (5) as big-endian
      0x00,
      0x00,
      0x00,
      0x05,
      // IP address type (IPv4 = 1) as big-endian
      0x00,
      0x00,
      0x00,
      0x01,
      // IPv4 address: 192.168.1.100 (4 bytes)
      0xc0,
      0xa8,
      0x01,
      0x64,
      // subAgentID (12345) as big-endian
      0x00,
      0x00,
      0x30,
      0x39,
      // sequenceNumber (67890) as big-endian
      0x00,
      0x01,
      0x09,
      0x32,
      // uptime (98765) as big-endian
      0x00,
      0x01,
      0x81,
      0xcd,
      // samplesCount (1) as big-endian
      0x00,
      0x00,
      0x00,
      0x01,

      // SampleRecord:
      // sampleType (1) as big-endian
      0x00,
      0x00,
      0x00,
      0x01,
      // sampleDataLen (32 bytes) as big-endian - size of the FlowSample below
      0x00,
      0x00,
      0x00,
      0x20,

      // FlowSample data (32 bytes total - no flow records):
      // sequenceNumber (111) as big-endian
      0x00,
      0x00,
      0x00,
      0x6f,
      // sourceID (222) as big-endian
      0x00,
      0x00,
      0x00,
      0xde,
      // samplingRate (333) as big-endian
      0x00,
      0x00,
      0x01,
      0x4d,
      // samplePool (444) as big-endian
      0x00,
      0x00,
      0x01,
      0xbc,
      // drops (555) as big-endian
      0x00,
      0x00,
      0x02,
      0x2b,
      // input (666) as big-endian
      0x00,
      0x00,
      0x02,
      0x9a,
      // output (777) as big-endian
      0x00,
      0x00,
      0x03,
      0x09,
      // flowRecordsCnt (0) as big-endian
      0x00,
      0x00,
      0x00,
      0x00};

  auto buf =
      folly::IOBuf::wrapBuffer(serializedData.data(), serializedData.size());
  folly::io::Cursor cursor(buf.get());

  // Deserialize the SampleDatagram
  sflow::SampleDatagram datagram = sflow::SampleDatagram::deserialize(cursor);

  // Verify the deserialized data
  EXPECT_EQ(
      datagram.datagramV5.agentAddress, folly::IPAddress("192.168.1.100"));
  EXPECT_EQ(datagram.datagramV5.subAgentID, 12345);
  EXPECT_EQ(datagram.datagramV5.sequenceNumber, 67890);
  EXPECT_EQ(datagram.datagramV5.uptime, 98765);
  EXPECT_EQ(datagram.datagramV5.samples.size(), 1);

  // Verify the SampleRecord
  const auto& sampleRecord = datagram.datagramV5.samples[0];
  EXPECT_EQ(sampleRecord.sampleType, 1);
  EXPECT_EQ(sampleRecord.sampleData.size(), 1);

  // Verify the FlowSample data
  const auto& flowSample =
      std::get<sflow::FlowSample>(sampleRecord.sampleData[0]);
  EXPECT_EQ(flowSample.sequenceNumber, 111);
  EXPECT_EQ(flowSample.sourceID, 222);
  EXPECT_EQ(flowSample.samplingRate, 333);
  EXPECT_EQ(flowSample.samplePool, 444);
  EXPECT_EQ(flowSample.drops, 555);
  EXPECT_EQ(flowSample.input, 666);
  EXPECT_EQ(flowSample.output, 777);
  EXPECT_EQ(flowSample.flowRecords.size(), 0);
}

TEST(SflowStructsTest, SampleDatagramDeserializationIPv6) {
  // Test SampleDatagram deserialization with IPv6 address

  std::vector<uint8_t> serializedData = {
      // sFlow version (5) as big-endian
      0x00,
      0x00,
      0x00,
      0x05,
      // IP address type (IPv6 = 2) as big-endian
      0x00,
      0x00,
      0x00,
      0x02,
      // IPv6 address: 2001:db8::1 (16 bytes)
      0x20,
      0x01,
      0x0d,
      0xb8,
      0x00,
      0x00,
      0x00,
      0x00,
      0x00,
      0x00,
      0x00,
      0x00,
      0x00,
      0x00,
      0x00,
      0x01,
      // subAgentID (1000) as big-endian
      0x00,
      0x00,
      0x03,
      0xe8,
      // sequenceNumber (2000) as big-endian
      0x00,
      0x00,
      0x07,
      0xd0,
      // uptime (3000) as big-endian
      0x00,
      0x00,
      0x0b,
      0xb8,
      // samplesCount (0) as big-endian
      0x00,
      0x00,
      0x00,
      0x00};

  auto buf =
      folly::IOBuf::wrapBuffer(serializedData.data(), serializedData.size());
  folly::io::Cursor cursor(buf.get());

  // Deserialize the SampleDatagram
  sflow::SampleDatagram datagram = sflow::SampleDatagram::deserialize(cursor);

  // Verify the deserialized data
  EXPECT_EQ(datagram.datagramV5.agentAddress, folly::IPAddress("2001:db8::1"));
  EXPECT_EQ(datagram.datagramV5.subAgentID, 1000);
  EXPECT_EQ(datagram.datagramV5.sequenceNumber, 2000);
  EXPECT_EQ(datagram.datagramV5.uptime, 3000);
  EXPECT_EQ(datagram.datagramV5.samples.size(), 0);
}

TEST(SflowStructsTest, SampleDatagramSerializeDeserializeRoundTrip) {
  // Test serialize-deserialize round trip for SampleDatagram

  // Create original SampleDatagram with complex data
  sflow::SampleDatagram original;
  original.datagramV5.agentAddress = folly::IPAddress("2401:db00:116:3016::1b");
  original.datagramV5.subAgentID = 99999;
  original.datagramV5.sequenceNumber = 88888;
  original.datagramV5.uptime = 77777;

  // Create sample record with FlowSample
  sflow::SampleRecord record;
  record.sampleType = 1;

  sflow::FlowSample flowSample;
  flowSample.sequenceNumber = 1111;
  flowSample.sourceID = 2222;
  flowSample.samplingRate = 3333;
  flowSample.samplePool = 4444;
  flowSample.drops = 5555;
  flowSample.input = 6666;
  flowSample.output = 7777;

  // Add flow records with different data sizes
  sflow::FlowRecord flowRecord1;
  flowRecord1.flowFormat = 1;
  flowRecord1.flowData = {0xDE, 0xAD, 0xBE, 0xEF}; // 4 bytes, no padding

  sflow::FlowRecord flowRecord2;
  flowRecord2.flowFormat = 1;
  flowRecord2.flowData = {0xCA, 0xFE, 0xBA}; // 3 bytes, 1 byte padding

  flowSample.flowRecords = {flowRecord1, flowRecord2};
  record.sampleData.emplace_back(std::move(flowSample));

  original.datagramV5.samples = {record};

  // Serialize
  int bufSize = 1024;
  std::vector<uint8_t> buffer(bufSize);
  auto buf = folly::IOBuf::wrapBuffer(buffer.data(), bufSize);
  auto rwCursor = std::make_shared<folly::io::RWPrivateCursor>(buf.get());

  original.serialize(rwCursor.get());
  size_t serializedSize = bufSize - rwCursor->length();

  // Deserialize
  auto readBuf = folly::IOBuf::wrapBuffer(buffer.data(), serializedSize);
  folly::io::Cursor readCursor(readBuf.get());

  sflow::SampleDatagram deserialized =
      sflow::SampleDatagram::deserialize(readCursor);

  // Verify round-trip correctness
  EXPECT_EQ(
      deserialized.datagramV5.agentAddress, original.datagramV5.agentAddress);
  EXPECT_EQ(deserialized.datagramV5.subAgentID, original.datagramV5.subAgentID);
  EXPECT_EQ(
      deserialized.datagramV5.sequenceNumber,
      original.datagramV5.sequenceNumber);
  EXPECT_EQ(deserialized.datagramV5.uptime, original.datagramV5.uptime);
  EXPECT_EQ(
      deserialized.datagramV5.samples.size(),
      original.datagramV5.samples.size());

  // Verify the sample record
  const auto& originalSample = original.datagramV5.samples[0];
  const auto& deserializedSample = deserialized.datagramV5.samples[0];

  EXPECT_EQ(deserializedSample.sampleType, originalSample.sampleType);
  EXPECT_EQ(
      deserializedSample.sampleData.size(), originalSample.sampleData.size());

  // Verify FlowSample data
  const auto& originalFlowSample =
      std::get<sflow::FlowSample>(originalSample.sampleData[0]);
  const auto& deserializedFlowSample =
      std::get<sflow::FlowSample>(deserializedSample.sampleData[0]);

  EXPECT_EQ(
      deserializedFlowSample.sequenceNumber, originalFlowSample.sequenceNumber);
  EXPECT_EQ(deserializedFlowSample.sourceID, originalFlowSample.sourceID);
  EXPECT_EQ(
      deserializedFlowSample.samplingRate, originalFlowSample.samplingRate);
  EXPECT_EQ(deserializedFlowSample.samplePool, originalFlowSample.samplePool);
  EXPECT_EQ(deserializedFlowSample.drops, originalFlowSample.drops);
  EXPECT_EQ(deserializedFlowSample.input, originalFlowSample.input);
  EXPECT_EQ(deserializedFlowSample.output, originalFlowSample.output);
  EXPECT_EQ(
      deserializedFlowSample.flowRecords.size(),
      originalFlowSample.flowRecords.size());

  // Verify each flow record
  for (size_t i = 0; i < originalFlowSample.flowRecords.size(); ++i) {
    EXPECT_EQ(
        deserializedFlowSample.flowRecords[i].flowFormat,
        originalFlowSample.flowRecords[i].flowFormat)
        << "FlowFormat mismatch for record " << i;
    EXPECT_THAT(
        deserializedFlowSample.flowRecords[i].flowData,
        ContainerEq(originalFlowSample.flowRecords[i].flowData))
        << "FlowData content mismatch for record " << i;
  }
}

TEST(SflowStructsTest, SampleDatagramDeserializeInvalidVersion) {
  // Test SampleDatagram deserialization with invalid version - should throw
  // exception

  std::vector<uint8_t> serializedData = {
      // Invalid sFlow version (4) as big-endian
      0x00,
      0x00,
      0x00,
      0x04,
      // Rest of data doesn't matter since version check should fail
      0x00,
      0x00,
      0x00,
      0x01};

  auto buf =
      folly::IOBuf::wrapBuffer(serializedData.data(), serializedData.size());
  folly::io::Cursor cursor(buf.get());

  // Deserialize should throw exception for unsupported version
  EXPECT_THROW(sflow::SampleDatagram::deserialize(cursor), std::runtime_error);
}

TEST(SflowStructsTest, SampleDatagramDeserializeEmptyDatagramV5) {
  // Test SampleDatagram deserialization with empty SampleDatagramV5

  std::vector<uint8_t> serializedData = {
      // sFlow version (5) as big-endian
      0x00,
      0x00,
      0x00,
      0x05,
      // IP address type (IPv4 = 1) as big-endian
      0x00,
      0x00,
      0x00,
      0x01,
      // IPv4 address: 10.0.0.1 (4 bytes)
      0x0a,
      0x00,
      0x00,
      0x01,
      // subAgentID (100) as big-endian
      0x00,
      0x00,
      0x00,
      0x64,
      // sequenceNumber (200) as big-endian
      0x00,
      0x00,
      0x00,
      0xc8,
      // uptime (300) as big-endian
      0x00,
      0x00,
      0x01,
      0x2c,
      // samplesCount (0) as big-endian
      0x00,
      0x00,
      0x00,
      0x00};

  auto buf =
      folly::IOBuf::wrapBuffer(serializedData.data(), serializedData.size());
  folly::io::Cursor cursor(buf.get());

  // Deserialize the SampleDatagram
  sflow::SampleDatagram datagram = sflow::SampleDatagram::deserialize(cursor);

  // Verify the deserialized data
  EXPECT_EQ(datagram.datagramV5.agentAddress, folly::IPAddress("10.0.0.1"));
  EXPECT_EQ(datagram.datagramV5.subAgentID, 100);
  EXPECT_EQ(datagram.datagramV5.sequenceNumber, 200);
  EXPECT_EQ(datagram.datagramV5.uptime, 300);
  EXPECT_EQ(datagram.datagramV5.samples.size(), 0);
}

TEST(SflowStructsTest, SampleDatagramDeserializeComplexNested) {
  // Test SampleDatagram deserialization with complex nested data

  // Create original complex SampleDatagram
  sflow::SampleDatagram original;
  original.datagramV5.agentAddress = folly::IPAddress("10.0.0.1");
  original.datagramV5.subAgentID = 54321;
  original.datagramV5.sequenceNumber = 98765;
  original.datagramV5.uptime = 1234567;

  // Create first sample record
  sflow::SampleRecord record1;
  record1.sampleType = 1;

  sflow::FlowSample flowSample1;
  flowSample1.sequenceNumber = 1111;
  flowSample1.sourceID = 2222;
  flowSample1.samplingRate = 3333;
  flowSample1.samplePool = 4444;
  flowSample1.drops = 5555;
  flowSample1.input = 6666;
  flowSample1.output = 7777;

  sflow::FlowRecord flowRecord1;
  flowRecord1.flowFormat = 1;
  flowRecord1.flowData = {0x11, 0x22, 0x33, 0x44}; // 4 bytes, no padding

  flowSample1.flowRecords = {flowRecord1};
  record1.sampleData.emplace_back(std::move(flowSample1));

  // Create second sample record
  sflow::SampleRecord record2;
  record2.sampleType = 1;

  sflow::FlowSample flowSample2;
  flowSample2.sequenceNumber = 8888;
  flowSample2.sourceID = 9999;
  flowSample2.samplingRate = 1010;
  flowSample2.samplePool = 2020;
  flowSample2.drops = 3030;
  flowSample2.input = 4040;
  flowSample2.output = 5050;

  sflow::FlowRecord flowRecord2;
  flowRecord2.flowFormat = 1;
  flowRecord2.flowData = {0xAA, 0xBB, 0xCC}; // 3 bytes, 1 byte padding

  flowSample2.flowRecords = {flowRecord2};
  record2.sampleData.emplace_back(std::move(flowSample2));

  original.datagramV5.samples = {record1, record2};

  // Serialize
  int bufSize = 1024;
  std::vector<uint8_t> buffer(bufSize);
  auto buf = folly::IOBuf::wrapBuffer(buffer.data(), bufSize);
  auto rwCursor = std::make_shared<folly::io::RWPrivateCursor>(buf.get());

  original.serialize(rwCursor.get());
  size_t serializedSize = bufSize - rwCursor->length();

  // Deserialize
  auto readBuf = folly::IOBuf::wrapBuffer(buffer.data(), serializedSize);
  folly::io::Cursor readCursor(readBuf.get());

  sflow::SampleDatagram deserialized =
      sflow::SampleDatagram::deserialize(readCursor);

  // Verify complex deserialization
  EXPECT_EQ(
      deserialized.datagramV5.agentAddress, original.datagramV5.agentAddress);
  EXPECT_EQ(deserialized.datagramV5.subAgentID, original.datagramV5.subAgentID);
  EXPECT_EQ(
      deserialized.datagramV5.sequenceNumber,
      original.datagramV5.sequenceNumber);
  EXPECT_EQ(deserialized.datagramV5.uptime, original.datagramV5.uptime);
  EXPECT_EQ(deserialized.datagramV5.samples.size(), 2);

  // Verify first sample
  const auto& sample1 = deserialized.datagramV5.samples[0];
  EXPECT_EQ(sample1.sampleType, 1);
  EXPECT_EQ(sample1.sampleData.size(), 1);

  const auto& flow1 = std::get<sflow::FlowSample>(sample1.sampleData[0]);
  EXPECT_EQ(flow1.sequenceNumber, 1111);
  EXPECT_EQ(flow1.sourceID, 2222);
  EXPECT_EQ(flow1.flowRecords.size(), 1);
  EXPECT_THAT(
      flow1.flowRecords[0].flowData, ElementsAre(0x11, 0x22, 0x33, 0x44));

  // Verify second sample
  const auto& sample2 = deserialized.datagramV5.samples[1];
  EXPECT_EQ(sample2.sampleType, 1);
  EXPECT_EQ(sample2.sampleData.size(), 1);

  const auto& flow2 = std::get<sflow::FlowSample>(sample2.sampleData[0]);
  EXPECT_EQ(flow2.sequenceNumber, 8888);
  EXPECT_EQ(flow2.sourceID, 9999);
  EXPECT_EQ(flow2.flowRecords.size(), 1);
  EXPECT_THAT(flow2.flowRecords[0].flowData, ElementsAre(0xAA, 0xBB, 0xCC));
}

TEST(SflowStructsTest, SampleDatagramDeserializeWithMultipleFlowRecords) {
  // Test SampleDatagram deserialization with FlowSample containing multiple
  // FlowRecords

  // Create original SampleDatagram
  sflow::SampleDatagram original;
  original.datagramV5.agentAddress = folly::IPAddress("192.168.10.50");
  original.datagramV5.subAgentID = 777;
  original.datagramV5.sequenceNumber = 888;
  original.datagramV5.uptime = 999;

  // Create sample record with FlowSample
  sflow::SampleRecord record;
  record.sampleType = 1;

  sflow::FlowSample flowSample;
  flowSample.sequenceNumber = 123;
  flowSample.sourceID = 456;
  flowSample.samplingRate = 789;
  flowSample.samplePool = 101;
  flowSample.drops = 202;
  flowSample.input = 303;
  flowSample.output = 404;

  // Add multiple flow records with different sizes to test XDR padding
  sflow::FlowRecord flowRecord1;
  flowRecord1.flowFormat = 1;
  flowRecord1.flowData = {0x01}; // 1 byte, 3 bytes padding

  sflow::FlowRecord flowRecord2;
  flowRecord2.flowFormat = 2;
  flowRecord2.flowData = {0x02, 0x03}; // 2 bytes, 2 bytes padding

  sflow::FlowRecord flowRecord3;
  flowRecord3.flowFormat = 3;
  flowRecord3.flowData = {0x04, 0x05, 0x06}; // 3 bytes, 1 byte padding

  sflow::FlowRecord flowRecord4;
  flowRecord4.flowFormat = 4;
  flowRecord4.flowData = {0x07, 0x08, 0x09, 0x0A}; // 4 bytes, no padding

  sflow::FlowRecord flowRecord5;
  flowRecord5.flowFormat = 5;
  flowRecord5.flowData = {
      0x0B, 0x0C, 0x0D, 0x0E, 0x0F}; // 5 bytes, 3 bytes padding

  flowSample.flowRecords = {
      flowRecord1, flowRecord2, flowRecord3, flowRecord4, flowRecord5};
  record.sampleData.emplace_back(std::move(flowSample));

  original.datagramV5.samples = {record};

  // Serialize
  int bufSize = 2048;
  std::vector<uint8_t> buffer(bufSize);
  auto buf = folly::IOBuf::wrapBuffer(buffer.data(), bufSize);
  auto rwCursor = std::make_shared<folly::io::RWPrivateCursor>(buf.get());

  original.serialize(rwCursor.get());
  size_t serializedSize = bufSize - rwCursor->length();

  // Deserialize
  auto readBuf = folly::IOBuf::wrapBuffer(buffer.data(), serializedSize);
  folly::io::Cursor readCursor(readBuf.get());

  sflow::SampleDatagram deserialized =
      sflow::SampleDatagram::deserialize(readCursor);

  // Verify the deserialized data
  EXPECT_EQ(
      deserialized.datagramV5.agentAddress, original.datagramV5.agentAddress);
  EXPECT_EQ(deserialized.datagramV5.subAgentID, original.datagramV5.subAgentID);
  EXPECT_EQ(
      deserialized.datagramV5.sequenceNumber,
      original.datagramV5.sequenceNumber);
  EXPECT_EQ(deserialized.datagramV5.uptime, original.datagramV5.uptime);
  EXPECT_EQ(deserialized.datagramV5.samples.size(), 1);

  // Verify the sample record
  const auto& sampleRecord = deserialized.datagramV5.samples[0];
  EXPECT_EQ(sampleRecord.sampleType, 1);
  EXPECT_EQ(sampleRecord.sampleData.size(), 1);

  // Verify the FlowSample data
  const auto& deserializedFlowSampleMultiple =
      std::get<sflow::FlowSample>(sampleRecord.sampleData[0]);
  EXPECT_EQ(deserializedFlowSampleMultiple.sequenceNumber, 123);
  EXPECT_EQ(deserializedFlowSampleMultiple.sourceID, 456);
  EXPECT_EQ(deserializedFlowSampleMultiple.samplingRate, 789);
  EXPECT_EQ(deserializedFlowSampleMultiple.samplePool, 101);
  EXPECT_EQ(deserializedFlowSampleMultiple.drops, 202);
  EXPECT_EQ(deserializedFlowSampleMultiple.input, 303);
  EXPECT_EQ(deserializedFlowSampleMultiple.output, 404);
  EXPECT_EQ(deserializedFlowSampleMultiple.flowRecords.size(), 5);

  // Verify each flow record
  EXPECT_EQ(deserializedFlowSampleMultiple.flowRecords[0].flowFormat, 1);
  EXPECT_THAT(
      deserializedFlowSampleMultiple.flowRecords[0].flowData,
      ElementsAre(0x01));

  EXPECT_EQ(deserializedFlowSampleMultiple.flowRecords[1].flowFormat, 2);
  EXPECT_THAT(
      deserializedFlowSampleMultiple.flowRecords[1].flowData,
      ElementsAre(0x02, 0x03));

  EXPECT_EQ(deserializedFlowSampleMultiple.flowRecords[2].flowFormat, 3);
  EXPECT_THAT(
      deserializedFlowSampleMultiple.flowRecords[2].flowData,
      ElementsAre(0x04, 0x05, 0x06));

  EXPECT_EQ(deserializedFlowSampleMultiple.flowRecords[3].flowFormat, 4);
  EXPECT_THAT(
      deserializedFlowSampleMultiple.flowRecords[3].flowData,
      ElementsAre(0x07, 0x08, 0x09, 0x0A));

  EXPECT_EQ(deserializedFlowSampleMultiple.flowRecords[4].flowFormat, 5);
  EXPECT_THAT(
      deserializedFlowSampleMultiple.flowRecords[4].flowData,
      ElementsAre(0x0B, 0x0C, 0x0D, 0x0E, 0x0F));
}

} // namespace facebook::fboss::sflow
