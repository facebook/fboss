/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/packet/bcm/XgsPsampMod.h"
#include "fboss/agent/packet/IpfixHeader.h"

#include <cstdint>
#include <vector>

#include <folly/container/Array.h>
#include <gtest/gtest.h>

#include "fboss/agent/packet/HdrParseError.h"

namespace facebook::fboss::psamp {

TEST(XgsPsampModTest, IpfixHeaderSerializeDeserialize) {
  IpfixHeader header;
  header.version = IPFIX_VERSION;
  header.length = 618; // arbitrary value to verify round-trip serialization
  header.exportTime = 0x692F800E;
  header.sequenceNumber = 1;
  header.observationDomainId = 42;

  EXPECT_EQ(header.size(), 16);

  constexpr int bufSize = 1024;
  std::vector<uint8_t> buffer(bufSize);
  auto buf = folly::IOBuf::wrapBuffer(buffer.data(), bufSize);
  auto cursor = std::make_shared<folly::io::RWPrivateCursor>(buf.get());

  header.serialize(cursor.get());
  size_t serializedSize = bufSize - cursor->length();
  EXPECT_EQ(serializedSize, 16);

  // clang-format off
  std::vector<uint8_t> expected = {
      0x00, 0x0A,                         // version = 10
      0x02, 0x6A,                         // length = 618
      0x69, 0x2F, 0x80, 0x0E,             // exportTime
      0x00, 0x00, 0x00, 0x01,             // sequenceNumber = 1
      0x00, 0x00, 0x00, 0x2A,             // observationDomainId = 42
  };
  // clang-format on
  std::vector<uint8_t> actual(buffer.begin(), buffer.begin() + serializedSize);
  EXPECT_EQ(actual, expected);

  auto deserializeBuf = folly::IOBuf::wrapBuffer(buffer.data(), serializedSize);
  folly::io::Cursor deserializeCursor(deserializeBuf.get());
  auto deserialized = IpfixHeader::deserialize(deserializeCursor);

  EXPECT_EQ(deserialized.version, header.version);
  EXPECT_EQ(deserialized.length, header.length);
  EXPECT_EQ(deserialized.exportTime, header.exportTime);
  EXPECT_EQ(deserialized.sequenceNumber, header.sequenceNumber);
  EXPECT_EQ(deserialized.observationDomainId, header.observationDomainId);
}

TEST(XgsPsampModTest, IpfixHeaderTruncatedBuffer) {
  std::vector<uint8_t> smallBuf(10); // < 16 bytes
  auto buf = folly::IOBuf::wrapBuffer(smallBuf.data(), smallBuf.size());
  folly::io::Cursor cursor(buf.get());
  EXPECT_THROW(IpfixHeader::deserialize(cursor), HdrParseError);
}

TEST(XgsPsampModTest, IpfixHeaderWrongVersion) {
  // clang-format off
  std::vector<uint8_t> buffer = {
      0x00, 0x05,                         // version = 5 (wrong)
      0x00, 0x10,                         // length = 16
      0x00, 0x00, 0x00, 0x00,             // exportTime
      0x00, 0x00, 0x00, 0x00,             // sequenceNumber
      0x00, 0x00, 0x00, 0x00,             // observationDomainId
  };
  // clang-format on
  auto buf = folly::IOBuf::wrapBuffer(buffer.data(), buffer.size());
  folly::io::Cursor cursor(buf.get());
  EXPECT_THROW(IpfixHeader::deserialize(cursor), HdrParseError);
}

TEST(XgsPsampModTest, XgsPsampTemplateHeaderSerializeDeserialize) {
  XgsPsampTemplateHeader header;
  header.templateId = XGS_PSAMP_TEMPLATE_ID;
  header.psampLength = 602;

  EXPECT_EQ(header.size(), 4);

  constexpr int bufSize = 1024;
  std::vector<uint8_t> buffer(bufSize);
  auto buf = folly::IOBuf::wrapBuffer(buffer.data(), bufSize);
  auto cursor = std::make_shared<folly::io::RWPrivateCursor>(buf.get());

  header.serialize(cursor.get());
  size_t serializedSize = bufSize - cursor->length();
  EXPECT_EQ(serializedSize, 4);

  // clang-format off
  std::vector<uint8_t> expected = {
      0x12, 0x34,                         // templateId = 0x1234
      0x02, 0x5A,                         // psampLength = 602
  };
  // clang-format on
  std::vector<uint8_t> actual(buffer.begin(), buffer.begin() + serializedSize);
  EXPECT_EQ(actual, expected);

  auto deserializeBuf = folly::IOBuf::wrapBuffer(buffer.data(), serializedSize);
  folly::io::Cursor deserializeCursor(deserializeBuf.get());
  auto deserialized = XgsPsampTemplateHeader::deserialize(deserializeCursor);

  EXPECT_EQ(deserialized.templateId, header.templateId);
  EXPECT_EQ(deserialized.psampLength, header.psampLength);
}

TEST(XgsPsampModTest, XgsPsampTemplateHeaderTruncatedBuffer) {
  std::vector<uint8_t> smallBuf(3); // < 4 bytes
  auto buf = folly::IOBuf::wrapBuffer(smallBuf.data(), smallBuf.size());
  folly::io::Cursor cursor(buf.get());
  EXPECT_THROW(XgsPsampTemplateHeader::deserialize(cursor), HdrParseError);
}

TEST(XgsPsampModTest, XgsPsampTemplateHeaderWrongTemplateId) {
  // clang-format off
  std::vector<uint8_t> buffer = {
      0xAB, 0xCD,                         // templateId = 0xABCD (wrong)
      0x00, 0x30,                         // psampLength = 48
  };
  // clang-format on
  auto buf = folly::IOBuf::wrapBuffer(buffer.data(), buffer.size());
  folly::io::Cursor cursor(buf.get());
  EXPECT_THROW(XgsPsampTemplateHeader::deserialize(cursor), HdrParseError);
}

TEST(XgsPsampModTest, XgsPsampDataSerializeDeserialize) {
  XgsPsampData data;
  data.observationTimeNs = 0x692F800B0B4769CE;
  data.switchId = 7;
  data.egressModPortId = 3;
  data.ingressPort = 1;
  data.dropReasonIngress = 0x1A;
  data.dropReasonMmu = 0;
  data.userMetaField = 0x1234;
  data.cosColorProb = 0;
  data.varLenIndicator = XGS_PSAMP_VAR_LEN_INDICATOR;
  data.packetSampledLength = 4;
  data.sampledPacketData = {0xDE, 0xAD, 0xBE, 0xEF};

  EXPECT_EQ(
      data.size(), 28); // 24 fixed-field bytes + 4 sampledPacketData bytes

  constexpr int bufSize = 1024;
  std::vector<uint8_t> buffer(bufSize);
  auto buf = folly::IOBuf::wrapBuffer(buffer.data(), bufSize);
  auto cursor = std::make_shared<folly::io::RWPrivateCursor>(buf.get());

  data.serialize(cursor.get());
  size_t serializedSize = bufSize - cursor->length();
  EXPECT_EQ(serializedSize, 28);

  // clang-format off
  std::vector<uint8_t> expected = {
      0x69, 0x2F, 0x80, 0x0B, 0x0B, 0x47, 0x69, 0xCE, // observationTimeNs
      0x00, 0x00, 0x00, 0x07,             // switchId = 7
      0x00, 0x03,                         // egressModPortId = 3
      0x00, 0x01,                         // ingressPort = 1
      0x1A,                               // dropReasonIngress
      0x00,                               // dropReasonMmu
      0x12, 0x34,                         // userMetaField
      0x00,                               // cosColorProb
      0xFF,                               // varLenIndicator
      0x00, 0x04,                         // packetSampledLength = 4
      0xDE, 0xAD, 0xBE, 0xEF,            // sampledPacketData
  };
  // clang-format on
  std::vector<uint8_t> actual(buffer.begin(), buffer.begin() + serializedSize);
  EXPECT_EQ(actual, expected);

  auto deserializeBuf = folly::IOBuf::wrapBuffer(buffer.data(), serializedSize);
  folly::io::Cursor deserializeCursor(deserializeBuf.get());
  auto deserialized = XgsPsampData::deserialize(deserializeCursor);

  EXPECT_EQ(deserialized.observationTimeNs, data.observationTimeNs);
  EXPECT_EQ(deserialized.switchId, data.switchId);
  EXPECT_EQ(deserialized.egressModPortId, data.egressModPortId);
  EXPECT_EQ(deserialized.ingressPort, data.ingressPort);
  EXPECT_EQ(deserialized.dropReasonIngress, data.dropReasonIngress);
  EXPECT_EQ(deserialized.dropReasonMmu, data.dropReasonMmu);
  EXPECT_EQ(deserialized.userMetaField, data.userMetaField);
  EXPECT_EQ(deserialized.cosColorProb, data.cosColorProb);
  EXPECT_EQ(deserialized.varLenIndicator, data.varLenIndicator);
  EXPECT_EQ(deserialized.packetSampledLength, data.packetSampledLength);
  EXPECT_EQ(deserialized.sampledPacketData, data.sampledPacketData);
}

// XgsPsampData has 24 bytes of fixed fields (observationTimeNs(8) +
// switchId(4) + egressModPortId(2) + ingressPort(2) + dropReasonIngress(1) +
// dropReasonMmu(1) + userMetaField(2) + cosColorProb(1) + varLenIndicator(1) +
// packetSampledLength(2)) before the variable-length sampled packet payload.
// Deserialize must throw when the buffer is shorter than this fixed prefix.
TEST(XgsPsampModTest, XgsPsampDataTruncatedBuffer) {
  std::vector<uint8_t> smallBuf(20); // 20 < 24 fixed-field bytes
  auto buf = folly::IOBuf::wrapBuffer(smallBuf.data(), smallBuf.size());
  folly::io::Cursor cursor(buf.get());
  EXPECT_THROW(XgsPsampData::deserialize(cursor), HdrParseError);
}

// The fixed fields parse successfully but packetSampledLength claims 100 bytes
// of sampled packet data while only 2 bytes remain in the buffer. Deserialize
// must throw because the cursor cannot satisfy the advertised payload length.
TEST(XgsPsampModTest, XgsPsampDataInsufficientSampledData) {
  // packetSampledLength=100 but only 2 data bytes follow
  // clang-format off
  std::vector<uint8_t> buffer = {
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // observationTimeNs
      0x00, 0x00, 0x00, 0x00,             // switchId
      0x00, 0x00,                         // egressModPortId
      0x00, 0x00,                         // ingressPort
      0x00,                               // dropReasonIngress
      0x00,                               // dropReasonMmu
      0x00, 0x00,                         // userMetaField
      0x00,                               // cosColorProb
      0xFF,                               // varLenIndicator
      0x00, 0x64,                         // packetSampledLength = 100
      0xAA, 0xBB,                         // only 2 bytes of data
  };
  // clang-format on
  auto buf = folly::IOBuf::wrapBuffer(buffer.data(), buffer.size());
  folly::io::Cursor cursor(buf.get());
  EXPECT_THROW(XgsPsampData::deserialize(cursor), HdrParseError);
}

TEST(XgsPsampModTest, XgsPsampDataInvalidVarLenIndicator) {
  // clang-format off
  std::vector<uint8_t> buffer = {
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // observationTimeNs
      0x00, 0x00, 0x00, 0x00,             // switchId
      0x00, 0x00,                         // egressModPortId
      0x00, 0x00,                         // ingressPort
      0x00,                               // dropReasonIngress
      0x00,                               // dropReasonMmu
      0x00, 0x00,                         // userMetaField
      0x00,                               // cosColorProb
      0xAA,                               // varLenIndicator = 0xAA (wrong)
      0x00, 0x00,                         // packetSampledLength
  };
  // clang-format on
  auto buf = folly::IOBuf::wrapBuffer(buffer.data(), buffer.size());
  folly::io::Cursor cursor(buf.get());
  EXPECT_THROW(XgsPsampData::deserialize(cursor), HdrParseError);
}

TEST(XgsPsampModTest, XgsPsampModPacketRoundTrip) {
  XgsPsampModPacket pkt;
  pkt.data.observationTimeNs = 0x692F800B0B4769CE;
  pkt.data.switchId = 0;
  pkt.data.egressModPortId = 0;
  pkt.data.ingressPort = 1;
  pkt.data.dropReasonIngress = 0x1A;
  pkt.data.dropReasonMmu = 0;
  pkt.data.userMetaField = 0x1234;
  pkt.data.cosColorProb = 0;
  pkt.data.varLenIndicator = XGS_PSAMP_VAR_LEN_INDICATOR;
  pkt.data.packetSampledLength = 4;
  pkt.data.sampledPacketData = {0xCA, 0xFE, 0xBA, 0xBE};

  pkt.templateHeader.templateId = XGS_PSAMP_TEMPLATE_ID;
  pkt.templateHeader.psampLength = pkt.templateHeader.size() + pkt.data.size();

  pkt.ipfixHeader.version = IPFIX_VERSION;
  pkt.ipfixHeader.length = pkt.size();
  pkt.ipfixHeader.exportTime = 0x692F800E;
  pkt.ipfixHeader.sequenceNumber = 1;
  pkt.ipfixHeader.observationDomainId = 1;

  // 16 + 4 + 24 + 4 = 48
  EXPECT_EQ(pkt.size(), 48);

  constexpr int bufSize = 1024;
  std::vector<uint8_t> buffer(bufSize);
  auto buf = folly::IOBuf::wrapBuffer(buffer.data(), bufSize);
  auto cursor = std::make_shared<folly::io::RWPrivateCursor>(buf.get());

  pkt.serialize(cursor.get());
  size_t serializedSize = bufSize - cursor->length();
  EXPECT_EQ(serializedSize, 48);

  auto deserializeBuf = folly::IOBuf::wrapBuffer(buffer.data(), serializedSize);
  folly::io::Cursor deserializeCursor(deserializeBuf.get());
  auto deserialized = XgsPsampModPacket::deserialize(deserializeCursor);

  EXPECT_EQ(deserialized.ipfixHeader.version, pkt.ipfixHeader.version);
  EXPECT_EQ(deserialized.ipfixHeader.length, pkt.ipfixHeader.length);
  EXPECT_EQ(deserialized.ipfixHeader.exportTime, pkt.ipfixHeader.exportTime);
  EXPECT_EQ(
      deserialized.ipfixHeader.sequenceNumber, pkt.ipfixHeader.sequenceNumber);
  EXPECT_EQ(
      deserialized.ipfixHeader.observationDomainId,
      pkt.ipfixHeader.observationDomainId);
  EXPECT_EQ(
      deserialized.templateHeader.templateId, pkt.templateHeader.templateId);
  EXPECT_EQ(
      deserialized.templateHeader.psampLength, pkt.templateHeader.psampLength);
  EXPECT_EQ(deserialized.data.switchId, pkt.data.switchId);
  EXPECT_EQ(deserialized.data.ingressPort, pkt.data.ingressPort);
  EXPECT_EQ(
      deserialized.data.packetSampledLength, pkt.data.packetSampledLength);
  EXPECT_EQ(deserialized.data.sampledPacketData, pkt.data.sampledPacketData);
}

// Serialize a valid packet but with an intentionally wrong IPFIX length.
// deserialize should parse all sub-headers successfully, then throw
// HdrParseError when the final length cross-check fails.
TEST(XgsPsampModTest, XgsPsampModPacketLengthMismatch) {
  XgsPsampModPacket pkt;
  pkt.ipfixHeader.version = IPFIX_VERSION;
  pkt.data.varLenIndicator = XGS_PSAMP_VAR_LEN_INDICATOR;
  pkt.data.packetSampledLength = 2;
  pkt.data.sampledPacketData = {0xAA, 0xBB};
  pkt.templateHeader.psampLength = pkt.templateHeader.size() + pkt.data.size();
  pkt.ipfixHeader.length = 9999; // intentionally wrong

  constexpr int bufSize = 1024;
  std::vector<uint8_t> buffer(bufSize);
  auto buf = folly::IOBuf::wrapBuffer(buffer.data(), bufSize);
  auto cursor = std::make_shared<folly::io::RWPrivateCursor>(buf.get());

  pkt.serialize(cursor.get());
  size_t serializedSize = bufSize - cursor->length();

  auto deserializeBuf = folly::IOBuf::wrapBuffer(buffer.data(), serializedSize);
  folly::io::Cursor deserializeCursor(deserializeBuf.get());
  EXPECT_THROW(
      XgsPsampModPacket::deserialize(deserializeCursor), HdrParseError);
}

TEST(XgsPsampModTest, DeserializeRealCapturedPacket) {
  // Packet captured from MoD test: Eth+VLAN(18) + IPv6(40) + UDP(8) +
  // IPFIX(16) + PSAMP Template(4) + PSAMP Data(24+20 inner bytes).
  // Inner payload truncated to 20 bytes for test brevity; lengths adjusted.
  // clang-format off
  auto fullPacket = folly::make_array<uint8_t>(
      // Outer Ethernet + VLAN (18 bytes)
      0x02, 0x88, 0x88, 0x88, 0x88, 0x88, // dst MAC
      0x02, 0x00, 0x00, 0x00, 0x00, 0x01, // src MAC
      0x81, 0x00, 0x07, 0xD2,             // VLAN tag
      0x86, 0xDD,                         // EtherType IPv6
      // IPv6 header (40 bytes)
      0x60, 0x00, 0x00, 0x00,             // version, TC, flow label
      0x00, 0x48,                         // payload length = 72
      0x11,                               // next header = UDP
      0x0F,                               // hop limit
      0x24, 0x01, 0xFA, 0xCE, 0xB0, 0x0C, 0x00, 0x00, // src IP
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01,
      0x24, 0x01, 0x99, 0x99, 0x99, 0x99, 0x99, 0x99, // dst IP
      0x99, 0x99, 0x99, 0x99, 0x99, 0x99, 0x99, 0x99,
      // UDP header (8 bytes)
      0x66, 0x66,                         // src port = 0x6666
      0x77, 0x77,                         // dst port = 0x7777
      0x00, 0x48,                         // length = 72
      0x00, 0x00,                         // checksum = 0
      // IPFIX header (16 bytes)
      0x00, 0x0A,                         // version = 10
      0x00, 0x40,                         // length = 64
      0x69, 0x2F, 0x80, 0x0E,             // export time
      0x00, 0x00, 0x00, 0x01,             // sequence number = 1
      0x00, 0x00, 0x00, 0x01,             // observation domain ID = 1
      // PSAMP template header (4 bytes)
      0x12, 0x34,                         // template ID = 0x1234
      0x00, 0x30,                         // psamp length = 48
      // PSAMP data fixed fields (24 bytes)
      0x69, 0x2F, 0x80, 0x0B, 0x0B, 0x47, 0x69, 0xCE, // observationTimeNs
      0x00, 0x00, 0x00, 0x00,             // switchId = 0
      0x00, 0x00,                         // egressModPortId = 0
      0x00, 0x01,                         // ingressPort = 1
      0x1A,                               // dropReasonIngress
      0x00,                               // dropReasonMmu
      0x12, 0x34,                         // userMetaField
      0x00,                               // cosColorProb
      0xFF,                               // varLenIndicator
      0x00, 0x14,                         // packetSampledLength = 20
      // Inner sampled packet (20 bytes: Eth header + start of IPv6)
      0x02, 0x00, 0x00, 0x00, 0x00, 0x01, // inner dst MAC
      0x02, 0x00, 0x00, 0x00, 0x00, 0x01, // inner src MAC
      0x86, 0xDD,                         // inner EtherType IPv6
      0x60, 0x00, 0x00, 0x00, 0x02, 0x08  // inner IPv6 version+TC+flow+plen
  );
  // clang-format on

  // IPFIX+PSAMP starts at offset 66 (after Eth+VLAN+IPv6+UDP)
  constexpr size_t ipfixOffset = 66;
  size_t ipfixLen = fullPacket.size() - ipfixOffset;

  auto buf =
      folly::IOBuf::wrapBuffer(fullPacket.data() + ipfixOffset, ipfixLen);
  folly::io::Cursor cursor(buf.get());
  auto pkt = XgsPsampModPacket::deserialize(cursor);

  EXPECT_EQ(pkt.ipfixHeader.version, 10);
  EXPECT_EQ(pkt.ipfixHeader.length, 64);
  EXPECT_EQ(pkt.ipfixHeader.exportTime, 0x692F800E);
  EXPECT_EQ(pkt.ipfixHeader.sequenceNumber, 1);
  EXPECT_EQ(pkt.ipfixHeader.observationDomainId, 1);

  EXPECT_EQ(pkt.templateHeader.templateId, 0x1234);
  EXPECT_EQ(pkt.templateHeader.psampLength, 48);

  EXPECT_EQ(pkt.data.observationTimeNs, 0x692F800B0B4769CE);
  EXPECT_EQ(pkt.data.switchId, 0);
  EXPECT_EQ(pkt.data.egressModPortId, 0);
  EXPECT_EQ(pkt.data.ingressPort, 1);
  EXPECT_EQ(pkt.data.dropReasonIngress, 0x1A);
  EXPECT_EQ(pkt.data.dropReasonMmu, 0);
  EXPECT_EQ(pkt.data.userMetaField, 0x1234);
  EXPECT_EQ(pkt.data.cosColorProb, 0);
  EXPECT_EQ(pkt.data.varLenIndicator, 0xFF);
  EXPECT_EQ(pkt.data.packetSampledLength, 20);
  EXPECT_EQ(pkt.data.sampledPacketData.size(), 20);

  // Inner sampled packet starts with Ethernet header (IPv6 EtherType)
  EXPECT_EQ(pkt.data.sampledPacketData[0], 0x02);
  EXPECT_EQ(pkt.data.sampledPacketData[5], 0x01);
  EXPECT_EQ(pkt.data.sampledPacketData[12], 0x86);
  EXPECT_EQ(pkt.data.sampledPacketData[13], 0xDD);

  EXPECT_EQ(pkt.size(), ipfixLen);
}

} // namespace facebook::fboss::psamp
