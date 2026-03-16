// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

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

// --- Constants ---

TEST(SflowStructsExtTest, xdrBasicBlockSize) {
  EXPECT_EQ(XDR_BASIC_BLOCK_SIZE, 4);
}

TEST(SflowStructsExtTest, sampleDatagramVersion5) {
  EXPECT_EQ(SampleDatagram::VERSION5, 5);
}

// --- serializeIP / deserializeIP / sizeIP ---

TEST(SflowStructsExtTest, serializeDeserializeIPv4) {
  auto ip = folly::IPAddress("10.0.0.1");

  // Serialize
  std::vector<uint8_t> buffer(64, 0);
  auto buf = folly::IOBuf::wrapBuffer(buffer.data(), buffer.size());
  folly::io::RWPrivateCursor cursor(buf.get());
  serializeIP(&cursor, ip);
  size_t written = buffer.size() - cursor.length();

  // 4 (address type) + 4 (IPv4 bytes) = 8
  EXPECT_EQ(written, 8);

  // Verify raw bytes: type=1 (IP_V4), then 10.0.0.1
  EXPECT_THAT(
      std::vector<uint8_t>(buffer.begin(), buffer.begin() + 8),
      ElementsAre(0x00, 0x00, 0x00, 0x01, 0x0a, 0x00, 0x00, 0x01));

  // Deserialize
  auto readBuf = folly::IOBuf::wrapBuffer(buffer.data(), written);
  folly::io::Cursor readCursor(readBuf.get());
  auto deserialized = deserializeIP(readCursor);
  EXPECT_EQ(deserialized, ip);
  EXPECT_TRUE(deserialized.isV4());
}

TEST(SflowStructsExtTest, serializeDeserializeIPv6) {
  auto ip = folly::IPAddress("2001:db8::1");

  // Serialize
  std::vector<uint8_t> buffer(64, 0);
  auto buf = folly::IOBuf::wrapBuffer(buffer.data(), buffer.size());
  folly::io::RWPrivateCursor cursor(buf.get());
  serializeIP(&cursor, ip);
  size_t written = buffer.size() - cursor.length();

  // 4 (address type) + 16 (IPv6 bytes) = 20
  EXPECT_EQ(written, 20);

  // Verify address type is IP_V6 (2)
  EXPECT_EQ(buffer[0], 0x00);
  EXPECT_EQ(buffer[1], 0x00);
  EXPECT_EQ(buffer[2], 0x00);
  EXPECT_EQ(buffer[3], 0x02);

  // Deserialize
  auto readBuf = folly::IOBuf::wrapBuffer(buffer.data(), written);
  folly::io::Cursor readCursor(readBuf.get());
  auto deserialized = deserializeIP(readCursor);
  EXPECT_EQ(deserialized, ip);
  EXPECT_TRUE(deserialized.isV6());
}

TEST(SflowStructsExtTest, deserializeIPUnknownTypeThrows) {
  // Create buffer with unknown address type (99)
  std::vector<uint8_t> data = {0x00, 0x00, 0x00, 0x63}; // type = 99
  auto buf = folly::IOBuf::wrapBuffer(data.data(), data.size());
  folly::io::Cursor cursor(buf.get());
  EXPECT_THROW(deserializeIP(cursor), std::runtime_error);
}

// --- serializeDataFormat ---

TEST(SflowStructsExtTest, serializeDataFormat) {
  std::vector<uint8_t> buffer(8, 0);
  auto buf = folly::IOBuf::wrapBuffer(buffer.data(), buffer.size());
  folly::io::RWPrivateCursor cursor(buf.get());

  serializeDataFormat(&cursor, 42);
  EXPECT_THAT(
      std::vector<uint8_t>(buffer.begin(), buffer.begin() + 4),
      ElementsAre(0x00, 0x00, 0x00, 0x2a));
}

// --- serializeSflowDataSource ---

TEST(SflowStructsExtTest, serializeSflowDataSource) {
  std::vector<uint8_t> buffer(8, 0);
  auto buf = folly::IOBuf::wrapBuffer(buffer.data(), buffer.size());
  folly::io::RWPrivateCursor cursor(buf.get());

  serializeSflowDataSource(&cursor, 0x12345678);
  EXPECT_THAT(
      std::vector<uint8_t>(buffer.begin(), buffer.begin() + 4),
      ElementsAre(0x12, 0x34, 0x56, 0x78));
}

// --- serializeSflowPort ---

TEST(SflowStructsExtTest, serializeSflowPort) {
  std::vector<uint8_t> buffer(8, 0);
  auto buf = folly::IOBuf::wrapBuffer(buffer.data(), buffer.size());
  folly::io::RWPrivateCursor cursor(buf.get());

  serializeSflowPort(&cursor, 65535);
  EXPECT_THAT(
      std::vector<uint8_t>(buffer.begin(), buffer.begin() + 4),
      ElementsAre(0x00, 0x00, 0xff, 0xff));
}

// --- HeaderProtocol enum round-trip ---

TEST(SflowStructsExtTest, allHeaderProtocolValues) {
  // Verify all HeaderProtocol enum values survive serialize/deserialize
  std::vector<HeaderProtocol> protocols = {
      HeaderProtocol::ETHERNET_ISO88023,
      HeaderProtocol::ISO88024_TOKENBUS,
      HeaderProtocol::ISO88025_TOKENRING,
      HeaderProtocol::FDDI,
      HeaderProtocol::FRAME_RELAY,
      HeaderProtocol::X25,
      HeaderProtocol::PPP,
      HeaderProtocol::SMDS,
      HeaderProtocol::AAL5,
      HeaderProtocol::AAL5_IP,
      HeaderProtocol::IPV4,
      HeaderProtocol::IPV6,
      HeaderProtocol::MPLS,
      HeaderProtocol::POS,
  };

  for (auto proto : protocols) {
    SampledHeader hdr;
    hdr.protocol = proto;
    hdr.frameLength = 100;
    hdr.stripped = 0;
    hdr.header = {0xaa, 0xbb, 0xcc, 0xdd}; // 4 bytes, no padding

    std::vector<uint8_t> buffer(64, 0);
    auto buf = folly::IOBuf::wrapBuffer(buffer.data(), buffer.size());
    folly::io::RWPrivateCursor cursor(buf.get());
    hdr.serialize(&cursor);
    size_t written = buffer.size() - cursor.length();

    auto readBuf = folly::IOBuf::wrapBuffer(buffer.data(), written);
    folly::io::Cursor readCursor(readBuf.get());
    auto deserialized = SampledHeader::deserialize(readCursor);

    EXPECT_EQ(deserialized.protocol, proto)
        << "Protocol mismatch for value " << static_cast<uint32_t>(proto);
  }
}

// --- FlowRecord: unsupported format throws ---

TEST(SflowStructsExtTest, flowRecordUnsupportedFormatThrows) {
  // flowFormat = 99 (unsupported), flowDataLen = 4, 4 bytes of data
  std::vector<uint8_t> data = {
      0x00,
      0x00,
      0x00,
      0x63, // flowFormat = 99
      0x00,
      0x00,
      0x00,
      0x04, // flowDataLen = 4
      0x00,
      0x00,
      0x00,
      0x00, // 4 bytes of data
  };
  auto buf = folly::IOBuf::wrapBuffer(data.data(), data.size());
  folly::io::Cursor cursor(buf.get());
  EXPECT_THROW(FlowRecord::deserialize(cursor), std::runtime_error);
}

// --- SampledHeader: large header near MTU ---

TEST(SflowStructsExtTest, sampledHeaderLargeData) {
  SampledHeader hdr;
  hdr.protocol = HeaderProtocol::ETHERNET_ISO88023;
  hdr.frameLength = 1518;
  hdr.stripped = 4;
  // 128 bytes of header data (no XDR padding needed since 128 % 4 == 0)
  hdr.header.resize(128, 0xab);

  EXPECT_EQ(hdr.size(), 4 + 4 + 4 + 4 + 128);

  std::vector<uint8_t> buffer(256, 0);
  auto buf = folly::IOBuf::wrapBuffer(buffer.data(), buffer.size());
  folly::io::RWPrivateCursor cursor(buf.get());
  hdr.serialize(&cursor);
  size_t written = buffer.size() - cursor.length();
  EXPECT_EQ(written, hdr.size());

  auto readBuf = folly::IOBuf::wrapBuffer(buffer.data(), written);
  folly::io::Cursor readCursor(readBuf.get());
  auto deserialized = SampledHeader::deserialize(readCursor);

  EXPECT_EQ(deserialized.protocol, HeaderProtocol::ETHERNET_ISO88023);
  EXPECT_EQ(deserialized.frameLength, 1518);
  EXPECT_EQ(deserialized.stripped, 4);
  EXPECT_EQ(deserialized.header.size(), 128);
  EXPECT_EQ(deserialized.header[0], 0xab);
  EXPECT_EQ(deserialized.header[127], 0xab);
}

// --- SampledHeader: header size requiring 1 byte padding ---

TEST(SflowStructsExtTest, sampledHeaderXdrPadding1Byte) {
  SampledHeader hdr;
  hdr.protocol = HeaderProtocol::FDDI;
  hdr.frameLength = 500;
  hdr.stripped = 0;
  hdr.header = {0x01, 0x02, 0x03}; // 3 bytes -> 1 byte padding

  // 4+4+4+4 + 4(3 data + 1 pad) = 20
  EXPECT_EQ(hdr.size(), 20);

  std::vector<uint8_t> buffer(64, 0);
  auto buf = folly::IOBuf::wrapBuffer(buffer.data(), buffer.size());
  folly::io::RWPrivateCursor cursor(buf.get());
  hdr.serialize(&cursor);
  size_t written = buffer.size() - cursor.length();
  EXPECT_EQ(written, 20);

  // Verify padding byte is zero
  EXPECT_EQ(buffer[19], 0x00);

  // Round-trip
  auto readBuf = folly::IOBuf::wrapBuffer(buffer.data(), written);
  folly::io::Cursor readCursor(readBuf.get());
  auto deserialized = SampledHeader::deserialize(readCursor);
  EXPECT_EQ(deserialized.header.size(), 3);
  EXPECT_THAT(deserialized.header, ElementsAre(0x01, 0x02, 0x03));
}

// --- FlowSample: empty flow records round-trip ---

TEST(SflowStructsExtTest, flowSampleEmptyRecordsRoundTrip) {
  FlowSample original;
  original.sequenceNumber = 1;
  original.sourceID = 2;
  original.samplingRate = 1000;
  original.samplePool = 5000;
  original.drops = 0;
  original.input = 10;
  original.output = 20;
  // No flow records

  // size should be 8 * 4 = 32 bytes (8 uint32_t fields)
  EXPECT_EQ(original.size(), 32);

  std::vector<uint8_t> buffer(64, 0);
  auto buf = folly::IOBuf::wrapBuffer(buffer.data(), buffer.size());
  folly::io::RWPrivateCursor cursor(buf.get());
  original.serialize(&cursor);
  size_t written = buffer.size() - cursor.length();
  EXPECT_EQ(written, 32);

  auto readBuf = folly::IOBuf::wrapBuffer(buffer.data(), written);
  folly::io::Cursor readCursor(readBuf.get());
  auto deserialized = FlowSample::deserialize(readCursor);

  EXPECT_EQ(deserialized.sequenceNumber, 1);
  EXPECT_EQ(deserialized.sourceID, 2);
  EXPECT_EQ(deserialized.samplingRate, 1000);
  EXPECT_EQ(deserialized.samplePool, 5000);
  EXPECT_EQ(deserialized.drops, 0);
  EXPECT_EQ(deserialized.input, 10);
  EXPECT_EQ(deserialized.output, 20);
  EXPECT_TRUE(deserialized.flowRecords.empty());
}

// --- SampleRecord: unknown sample type skips data ---

TEST(SflowStructsExtTest, sampleRecordUnknownTypeSkipsData) {
  // sampleType = 99 (unknown), sampleDataLen = 8, 8 bytes of opaque data
  std::vector<uint8_t> data = {
      0x00,
      0x00,
      0x00,
      0x63, // sampleType = 99
      0x00,
      0x00,
      0x00,
      0x08, // sampleDataLen = 8
      0xaa,
      0xbb,
      0xcc,
      0xdd, // opaque data
      0xee,
      0xff,
      0x11,
      0x22, // opaque data
  };
  auto buf = folly::IOBuf::wrapBuffer(data.data(), data.size());
  folly::io::Cursor cursor(buf.get());

  auto record = SampleRecord::deserialize(cursor);
  EXPECT_EQ(record.sampleType, 99);
  // Unknown type: data is skipped, sampleData should be empty
  EXPECT_TRUE(record.sampleData.empty());
}

// --- SampleDatagram: invalid version throws ---

TEST(SflowStructsExtTest, sampleDatagramInvalidVersionThrows) {
  // Version = 4 (unsupported)
  std::vector<uint8_t> data = {0x00, 0x00, 0x00, 0x04};
  auto buf = folly::IOBuf::wrapBuffer(data.data(), data.size());
  folly::io::Cursor cursor(buf.get());
  EXPECT_THROW(SampleDatagram::deserialize(cursor), std::runtime_error);
}

TEST(SflowStructsExtTest, sampleDatagramVersionZeroThrows) {
  std::vector<uint8_t> data = {0x00, 0x00, 0x00, 0x00};
  auto buf = folly::IOBuf::wrapBuffer(data.data(), data.size());
  folly::io::Cursor cursor(buf.get());
  EXPECT_THROW(SampleDatagram::deserialize(cursor), std::runtime_error);
}

// --- Full end-to-end: SampleDatagram with IPv6 agent, FlowSample, headers ---

TEST(SflowStructsExtTest, fullEndToEndIPv6Agent) {
  // Build a complete datagram from scratch
  SampledHeader sampledHdr;
  sampledHdr.protocol = HeaderProtocol::IPV6;
  sampledHdr.frameLength = 1280;
  sampledHdr.stripped = 0;
  sampledHdr.header = {0x60, 0x00, 0x00, 0x00, 0x00, 0x08, 0x3a, 0xff};

  FlowRecord record;
  record.flowFormat = 1;
  record.flowData = sampledHdr;

  FlowSample flowSample;
  flowSample.sequenceNumber = 42;
  flowSample.sourceID = 100;
  flowSample.samplingRate = 512;
  flowSample.samplePool = 10000;
  flowSample.drops = 3;
  flowSample.input = 1;
  flowSample.output = 2;
  flowSample.flowRecords = {record};

  SampleRecord sampleRecord;
  sampleRecord.sampleType = 1;
  sampleRecord.sampleData.emplace_back(flowSample);

  SampleDatagramV5 dgV5;
  dgV5.agentAddress = folly::IPAddress("fe80::1");
  dgV5.subAgentID = 0;
  dgV5.sequenceNumber = 1000;
  dgV5.uptime = 3600000;
  dgV5.samples = {sampleRecord};

  SampleDatagram datagram;
  datagram.datagramV5 = dgV5;

  // Serialize
  auto totalSize = datagram.size();
  std::vector<uint8_t> buffer(totalSize, 0);
  auto buf = folly::IOBuf::wrapBuffer(buffer.data(), buffer.size());
  folly::io::RWPrivateCursor cursor(buf.get());
  datagram.serialize(&cursor);
  size_t written = buffer.size() - cursor.length();
  EXPECT_EQ(written, totalSize);

  // Deserialize
  auto readBuf = folly::IOBuf::wrapBuffer(buffer.data(), written);
  folly::io::Cursor readCursor(readBuf.get());
  auto deserialized = SampleDatagram::deserialize(readCursor);

  // Verify all fields
  EXPECT_EQ(deserialized.datagramV5.agentAddress, folly::IPAddress("fe80::1"));
  EXPECT_EQ(deserialized.datagramV5.subAgentID, 0);
  EXPECT_EQ(deserialized.datagramV5.sequenceNumber, 1000);
  EXPECT_EQ(deserialized.datagramV5.uptime, 3600000);
  ASSERT_EQ(deserialized.datagramV5.samples.size(), 1);
  EXPECT_EQ(deserialized.datagramV5.samples[0].sampleType, 1);
  ASSERT_EQ(deserialized.datagramV5.samples[0].sampleData.size(), 1);

  const auto& fs =
      std::get<FlowSample>(deserialized.datagramV5.samples[0].sampleData[0]);
  EXPECT_EQ(fs.sequenceNumber, 42);
  EXPECT_EQ(fs.sourceID, 100);
  EXPECT_EQ(fs.samplingRate, 512);
  EXPECT_EQ(fs.samplePool, 10000);
  EXPECT_EQ(fs.drops, 3);
  EXPECT_EQ(fs.input, 1);
  EXPECT_EQ(fs.output, 2);
  ASSERT_EQ(fs.flowRecords.size(), 1);

  const auto& hdr = std::get<SampledHeader>(fs.flowRecords[0].flowData);
  EXPECT_EQ(hdr.protocol, HeaderProtocol::IPV6);
  EXPECT_EQ(hdr.frameLength, 1280);
  EXPECT_THAT(
      hdr.header, ElementsAre(0x60, 0x00, 0x00, 0x00, 0x00, 0x08, 0x3a, 0xff));
}

// --- SampleDatagramV5: empty samples round-trip ---

TEST(SflowStructsExtTest, sampleDatagramV5EmptySamplesRoundTrip) {
  SampleDatagramV5 original;
  original.agentAddress = folly::IPAddress("10.1.1.1");
  original.subAgentID = 7;
  original.sequenceNumber = 999;
  original.uptime = 12345;
  // No samples

  auto totalSize = original.size();
  std::vector<uint8_t> buffer(totalSize, 0);
  auto buf = folly::IOBuf::wrapBuffer(buffer.data(), buffer.size());
  folly::io::RWPrivateCursor cursor(buf.get());
  original.serialize(&cursor);
  size_t written = buffer.size() - cursor.length();
  EXPECT_EQ(written, totalSize);

  auto readBuf = folly::IOBuf::wrapBuffer(buffer.data(), written);
  folly::io::Cursor readCursor(readBuf.get());
  auto deserialized = SampleDatagramV5::deserialize(readCursor);

  EXPECT_EQ(deserialized.agentAddress, folly::IPAddress("10.1.1.1"));
  EXPECT_EQ(deserialized.subAgentID, 7);
  EXPECT_EQ(deserialized.sequenceNumber, 999);
  EXPECT_EQ(deserialized.uptime, 12345);
  EXPECT_TRUE(deserialized.samples.empty());
}

// --- SampleRecord: empty data (sampleDataLen = 0) ---

TEST(SflowStructsExtTest, sampleRecordEmptyDataRoundTrip) {
  // sampleType = 1, sampleDataLen = 0
  std::vector<uint8_t> data = {
      0x00,
      0x00,
      0x00,
      0x01, // sampleType = 1
      0x00,
      0x00,
      0x00,
      0x00, // sampleDataLen = 0
  };
  auto buf = folly::IOBuf::wrapBuffer(data.data(), data.size());
  folly::io::Cursor cursor(buf.get());

  auto record = SampleRecord::deserialize(cursor);
  EXPECT_EQ(record.sampleType, 1);
  EXPECT_TRUE(record.sampleData.empty());
}

// --- FlowSample: size matches serialized bytes ---

TEST(SflowStructsExtTest, flowSampleSizeMatchesSerialized) {
  FlowSample fs;
  fs.sequenceNumber = 100;
  fs.sourceID = 200;
  fs.samplingRate = 300;
  fs.samplePool = 400;
  fs.drops = 0;
  fs.input = 5;
  fs.output = 6;

  SampledHeader hdr;
  hdr.protocol = HeaderProtocol::ETHERNET_ISO88023;
  hdr.frameLength = 64;
  hdr.stripped = 0;
  hdr.header = {0x01, 0x02, 0x03, 0x04, 0x05}; // 5 bytes, 3 pad

  FlowRecord rec;
  rec.flowFormat = 1;
  rec.flowData = hdr;
  fs.flowRecords = {rec};

  std::vector<uint8_t> buffer(512, 0);
  auto buf = folly::IOBuf::wrapBuffer(buffer.data(), buffer.size());
  folly::io::RWPrivateCursor cursor(buf.get());
  fs.serialize(&cursor);
  size_t written = buffer.size() - cursor.length();

  EXPECT_EQ(fs.size(), written);
}

// --- SampleRecord: size matches serialized bytes ---

TEST(SflowStructsExtTest, sampleRecordSizeMatchesSerialized) {
  FlowSample fs;
  fs.sequenceNumber = 1;
  fs.sourceID = 2;
  fs.samplingRate = 3;
  fs.samplePool = 4;
  fs.drops = 5;
  fs.input = 6;
  fs.output = 7;
  // empty flow records

  SampleRecord rec;
  rec.sampleType = 1;
  rec.sampleData.emplace_back(fs);

  std::vector<uint8_t> buffer(512, 0);
  auto buf = folly::IOBuf::wrapBuffer(buffer.data(), buffer.size());
  folly::io::RWPrivateCursor cursor(buf.get());
  rec.serialize(&cursor);
  size_t written = buffer.size() - cursor.length();

  EXPECT_EQ(rec.size(), written);
}

// --- SampleDatagram: size matches serialized bytes ---

TEST(SflowStructsExtTest, sampleDatagramSizeMatchesSerialized) {
  SampleDatagram dg;
  dg.datagramV5.agentAddress = folly::IPAddress("192.168.0.1");
  dg.datagramV5.subAgentID = 0;
  dg.datagramV5.sequenceNumber = 1;
  dg.datagramV5.uptime = 100;

  std::vector<uint8_t> buffer(512, 0);
  auto buf = folly::IOBuf::wrapBuffer(buffer.data(), buffer.size());
  folly::io::RWPrivateCursor cursor(buf.get());
  dg.serialize(&cursor);
  size_t written = buffer.size() - cursor.length();

  EXPECT_EQ(dg.size(), written);
}

// --- AddressType enum values ---

TEST(SflowStructsExtTest, addressTypeEnumValues) {
  EXPECT_EQ(static_cast<uint32_t>(AddressType::UNKNOWN), 0);
  EXPECT_EQ(static_cast<uint32_t>(AddressType::IP_V4), 1);
  EXPECT_EQ(static_cast<uint32_t>(AddressType::IP_V6), 2);
}

// --- HeaderProtocol enum values ---

TEST(SflowStructsExtTest, headerProtocolEnumValues) {
  EXPECT_EQ(static_cast<uint32_t>(HeaderProtocol::ETHERNET_ISO88023), 1);
  EXPECT_EQ(static_cast<uint32_t>(HeaderProtocol::ISO88024_TOKENBUS), 2);
  EXPECT_EQ(static_cast<uint32_t>(HeaderProtocol::ISO88025_TOKENRING), 3);
  EXPECT_EQ(static_cast<uint32_t>(HeaderProtocol::FDDI), 4);
  EXPECT_EQ(static_cast<uint32_t>(HeaderProtocol::FRAME_RELAY), 5);
  EXPECT_EQ(static_cast<uint32_t>(HeaderProtocol::X25), 6);
  EXPECT_EQ(static_cast<uint32_t>(HeaderProtocol::PPP), 7);
  EXPECT_EQ(static_cast<uint32_t>(HeaderProtocol::SMDS), 8);
  EXPECT_EQ(static_cast<uint32_t>(HeaderProtocol::AAL5), 9);
  EXPECT_EQ(static_cast<uint32_t>(HeaderProtocol::AAL5_IP), 10);
  EXPECT_EQ(static_cast<uint32_t>(HeaderProtocol::IPV4), 11);
  EXPECT_EQ(static_cast<uint32_t>(HeaderProtocol::IPV6), 12);
  EXPECT_EQ(static_cast<uint32_t>(HeaderProtocol::MPLS), 13);
  EXPECT_EQ(static_cast<uint32_t>(HeaderProtocol::POS), 14);
}

} // namespace facebook::fboss::sflow
