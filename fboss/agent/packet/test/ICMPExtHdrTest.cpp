// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/agent/packet/ICMPExtHdr.h"

#include <gtest/gtest.h>

#include <folly/io/Cursor.h>
#include <folly/io/IOBuf.h>
#include "fboss/agent/packet/PktUtil.h"

using namespace facebook::fboss;

// --- ICMPExtInterfaceCType tests ---

TEST(ICMPExtHdrTest, cTypeDefaultsToZero) {
  ICMPExtInterfaceCType cType;
  EXPECT_EQ(static_cast<uint8_t>(cType), 0);
}

TEST(ICMPExtHdrTest, cTypeWithIpAddress) {
  ICMPExtInterfaceCType cType;
  cType.ipAddress = true;
  // ipAddress is bit 2 (0-indexed from right)
  EXPECT_EQ(static_cast<uint8_t>(cType), 0x04);
}

TEST(ICMPExtHdrTest, cTypeWithName) {
  ICMPExtInterfaceCType cType;
  cType.name = true;
  // name is bit 1
  EXPECT_EQ(static_cast<uint8_t>(cType), 0x02);
}

TEST(ICMPExtHdrTest, cTypeWithMtu) {
  ICMPExtInterfaceCType cType;
  cType.mtu = true;
  // mtu is bit 0
  EXPECT_EQ(static_cast<uint8_t>(cType), 0x01);
}

TEST(ICMPExtHdrTest, cTypeWithIfIndex) {
  ICMPExtInterfaceCType cType;
  cType.ifIndex = true;
  // ifIndex is bit 3
  EXPECT_EQ(static_cast<uint8_t>(cType), 0x08);
}

TEST(ICMPExtHdrTest, cTypeWithAllFields) {
  ICMPExtInterfaceCType cType;
  cType.interfaceRole =
      ICMPExtInterfaceRole::ICMP_EXT_INTERFACE_ROLE_IP_NEXT_HOP;
  cType.ifIndex = true;
  cType.ipAddress = true;
  cType.name = true;
  cType.mtu = true;
  // role=3 (bits 7-6 = 0xC0), ifIndex=0x08, ipAddress=0x04, name=0x02,
  // mtu=0x01
  EXPECT_EQ(static_cast<uint8_t>(cType), 0xCF);
}

TEST(ICMPExtHdrTest, cTypeEgressInterfaceRole) {
  ICMPExtInterfaceCType cType;
  cType.interfaceRole =
      ICMPExtInterfaceRole::ICMP_EXT_INTERFACE_ROLE_EGRESS_INTERFACE;
  // role=2 (bits 7-6 = 0x80)
  EXPECT_EQ(static_cast<uint8_t>(cType), 0x80);
}

TEST(ICMPExtHdrTest, cTypeSubIpArrivedRole) {
  ICMPExtInterfaceCType cType;
  cType.interfaceRole =
      ICMPExtInterfaceRole::ICMP_EXT_INTERFACE_ROLE_SUB_IP_ARRIVED;
  // role=1 (bits 7-6 = 0x40)
  EXPECT_EQ(static_cast<uint8_t>(cType), 0x40);
}

// --- ICMPExtIfaceNameSubObject tests ---

TEST(ICMPExtHdrTest, ifaceNameSubObjectLength) {
  ICMPExtIfaceNameSubObject nameObj("eth0");
  // "eth0" = 4 chars, +2 for length and null = 6, padded to 8 (4-byte
  // boundary)
  EXPECT_EQ(nameObj.length(), 8);
}

TEST(ICMPExtHdrTest, ifaceNameSubObjectLengthNoPadding) {
  // "ab" = 2 chars, +2 = 4, already 4-byte aligned
  ICMPExtIfaceNameSubObject nameObj("ab");
  EXPECT_EQ(nameObj.length(), 4);
}

TEST(ICMPExtHdrTest, ifaceNameSubObjectLengthSingleChar) {
  // "a" = 1 char, +2 = 3, padded to 4
  ICMPExtIfaceNameSubObject nameObj("a");
  EXPECT_EQ(nameObj.length(), 4);
}

TEST(ICMPExtHdrTest, ifaceNameSubObjectSerialize) {
  ICMPExtIfaceNameSubObject nameObj("eth0");
  auto buf = folly::IOBuf::create(nameObj.length());
  buf->append(nameObj.length());
  folly::io::RWPrivateCursor cursor(buf.get());
  nameObj.serialize(&cursor);

  folly::io::Cursor reader(buf.get());
  // First byte is length
  auto len = reader.read<uint8_t>();
  EXPECT_EQ(len, nameObj.length());
  // Next bytes are "eth0" + null + padding
  auto e = reader.read<uint8_t>();
  EXPECT_EQ(e, 'e');
  auto t = reader.read<uint8_t>();
  EXPECT_EQ(t, 't');
  auto h = reader.read<uint8_t>();
  EXPECT_EQ(h, 'h');
  auto zero = reader.read<uint8_t>();
  EXPECT_EQ(zero, '0');
}

TEST(ICMPExtHdrTest, ifaceNameSubObjectDefaultEmpty) {
  ICMPExtIfaceNameSubObject nameObj;
  // Empty name: 0 chars + 2 = 2, padded to 4
  EXPECT_EQ(nameObj.length(), 4);
}

// --- ICMPExtIpSubObjectV4 tests ---

TEST(ICMPExtHdrTest, ipSubObjectV4Length) {
  ICMPExtIpSubObjectV4 ipObj(folly::IPAddressV4("10.0.0.1"));
  // 4 bytes AFI + reserved, 4 bytes IPv4
  EXPECT_EQ(ipObj.length(), 8);
}

TEST(ICMPExtHdrTest, ipSubObjectV4Serialize) {
  ICMPExtIpSubObjectV4 ipObj(folly::IPAddressV4("10.0.0.1"));
  auto buf = folly::IOBuf::create(ipObj.length());
  buf->append(ipObj.length());
  folly::io::RWPrivateCursor cursor(buf.get());
  ipObj.serialize(&cursor);

  folly::io::Cursor reader(buf.get());
  // AFI = 1 (IPv4)
  auto afi = reader.readBE<uint16_t>();
  EXPECT_EQ(afi, 1);
  // Reserved = 0
  auto reserved = reader.readBE<uint16_t>();
  EXPECT_EQ(reserved, 0);
  // IPv4 address 10.0.0.1
  auto addr = reader.read<uint32_t>();
  EXPECT_EQ(addr, folly::IPAddressV4("10.0.0.1").toLong());
}

// --- ICMPExtIpSubObjectV6 tests ---

TEST(ICMPExtHdrTest, ipSubObjectV6Length) {
  ICMPExtIpSubObjectV6 ipObj(folly::IPAddressV6("2001:db8::1"));
  // 4 bytes AFI + reserved, 16 bytes IPv6
  EXPECT_EQ(ipObj.length(), 20);
}

TEST(ICMPExtHdrTest, ipSubObjectV6Serialize) {
  ICMPExtIpSubObjectV6 ipObj(folly::IPAddressV6("2001:db8::1"));
  auto buf = folly::IOBuf::create(ipObj.length());
  buf->append(ipObj.length());
  folly::io::RWPrivateCursor cursor(buf.get());
  ipObj.serialize(&cursor);

  folly::io::Cursor reader(buf.get());
  // AFI = 2 (IPv6)
  auto afi = reader.readBE<uint16_t>();
  EXPECT_EQ(afi, 2);
  // Reserved = 0
  auto reserved = reader.readBE<uint16_t>();
  EXPECT_EQ(reserved, 0);
  // IPv6 address bytes
  uint8_t addrBytes[16];
  reader.pull(addrBytes, 16);
  auto expectedAddr = folly::IPAddressV6("2001:db8::1");
  EXPECT_EQ(memcmp(addrBytes, expectedAddr.bytes(), 16), 0);
}

// --- ICMPExtInterfaceInformationObject tests ---

TEST(ICMPExtHdrTest, interfaceInfoObjectWithIpV4Only) {
  auto ipObj =
      std::make_unique<ICMPExtIpSubObjectV4>(folly::IPAddressV4("10.0.0.1"));
  ICMPExtInterfaceInformationObject obj(std::move(ipObj), nullptr);

  // Header (4) + IPv4 sub-object (8)
  EXPECT_EQ(obj.length(), 12);
  EXPECT_EQ(
      obj.classNum,
      ICMPExtObjectClassNum::
          ICMP_EXT_OBJECT_CLASS_INTERFACE_INFORMATION_OBJECT);
  // cType should have ipAddress bit set (0x04)
  EXPECT_EQ(obj.cType() & 0x04, 0x04);
  EXPECT_EQ(obj.cType() & 0x02, 0x00); // name not set
}

TEST(ICMPExtHdrTest, interfaceInfoObjectWithNameOnly) {
  auto nameObj = std::make_unique<ICMPExtIfaceNameSubObject>("eth0");
  ICMPExtInterfaceInformationObject obj(nullptr, std::move(nameObj));

  // Header (4) + name sub-object (8)
  EXPECT_EQ(obj.length(), 12);
  // cType should have name bit set (0x02)
  EXPECT_EQ(obj.cType() & 0x02, 0x02);
  EXPECT_EQ(obj.cType() & 0x04, 0x00); // ipAddress not set
}

TEST(ICMPExtHdrTest, interfaceInfoObjectWithBothIpAndName) {
  auto ipObj =
      std::make_unique<ICMPExtIpSubObjectV6>(folly::IPAddressV6("2001:db8::1"));
  auto nameObj = std::make_unique<ICMPExtIfaceNameSubObject>("eth0");
  ICMPExtInterfaceInformationObject obj(std::move(ipObj), std::move(nameObj));

  // Header (4) + IPv6 sub-object (20) + name sub-object (8)
  EXPECT_EQ(obj.length(), 32);
  // cType should have both ipAddress and name bits set
  EXPECT_EQ(obj.cType() & 0x04, 0x04);
  EXPECT_EQ(obj.cType() & 0x02, 0x02);
}

TEST(ICMPExtHdrTest, interfaceInfoObjectWithNeitherIpNorName) {
  ICMPExtInterfaceInformationObject obj(nullptr, nullptr);

  // Header only (4)
  EXPECT_EQ(obj.length(), 4);
  EXPECT_EQ(obj.cType(), 0x00);
}

TEST(ICMPExtHdrTest, interfaceInfoObjectSerialize) {
  auto ipObj =
      std::make_unique<ICMPExtIpSubObjectV4>(folly::IPAddressV4("10.0.0.1"));
  ICMPExtInterfaceInformationObject obj(std::move(ipObj), nullptr);

  auto buf = folly::IOBuf::create(obj.length());
  buf->append(obj.length());
  folly::io::RWPrivateCursor cursor(buf.get());
  obj.serialize(&cursor);

  folly::io::Cursor reader(buf.get());
  // Object header: length (2 bytes)
  auto objLen = reader.readBE<uint16_t>();
  EXPECT_EQ(objLen, 12);
  // Class-Num (1 byte) = 2 (Interface Information Object)
  auto classNum = reader.read<uint8_t>();
  EXPECT_EQ(classNum, 2);
  // C-Type (1 byte) = 0x04 (ipAddress bit)
  auto ctype = reader.read<uint8_t>();
  EXPECT_EQ(ctype & 0x04, 0x04);
  // AFI = 1 (IPv4)
  auto afi = reader.readBE<uint16_t>();
  EXPECT_EQ(afi, 1);
}

// --- ICMPExtHdr tests ---

TEST(ICMPExtHdrTest, hdrLengthNoObjects) {
  ICMPExtHdr hdr;
  // Header only: 4 bytes
  EXPECT_EQ(hdr.length(), 4);
}

TEST(ICMPExtHdrTest, hdrLengthWithObjects) {
  auto ipObj =
      std::make_unique<ICMPExtIpSubObjectV4>(folly::IPAddressV4("10.0.0.1"));
  ICMPExtInterfaceInformationObject obj(std::move(ipObj), nullptr);

  ICMPExtHdr hdr;
  hdr.objects.push_back(&obj);
  // Header (4) + object (12)
  EXPECT_EQ(hdr.length(), 16);
}

TEST(ICMPExtHdrTest, hdrLengthWithMultipleObjects) {
  auto ipObj1 =
      std::make_unique<ICMPExtIpSubObjectV4>(folly::IPAddressV4("10.0.0.1"));
  ICMPExtInterfaceInformationObject obj1(std::move(ipObj1), nullptr);

  auto ipObj2 =
      std::make_unique<ICMPExtIpSubObjectV6>(folly::IPAddressV6("2001:db8::1"));
  ICMPExtInterfaceInformationObject obj2(std::move(ipObj2), nullptr);

  ICMPExtHdr hdr;
  hdr.objects.push_back(&obj1);
  hdr.objects.push_back(&obj2);
  // Header (4) + obj1 (4+8=12) + obj2 (4+20=24)
  EXPECT_EQ(hdr.length(), 40);
}

TEST(ICMPExtHdrTest, hdrSerialize) {
  auto ipObj =
      std::make_unique<ICMPExtIpSubObjectV4>(folly::IPAddressV4("10.0.0.1"));
  ICMPExtInterfaceInformationObject obj(std::move(ipObj), nullptr);

  ICMPExtHdr hdr;
  hdr.objects.push_back(&obj);

  auto buf = folly::IOBuf::create(hdr.length());
  buf->append(hdr.length());
  folly::io::RWPrivateCursor cursor(buf.get());
  hdr.serialize(&cursor);

  folly::io::Cursor reader(buf.get());
  // First byte: version (2) in upper 4 bits
  auto versionByte = reader.read<uint8_t>();
  EXPECT_EQ(versionByte >> 4, 2);
  EXPECT_EQ(versionByte & 0x0F, 0); // lower 4 bits reserved

  // Second byte: reserved
  auto reserved = reader.read<uint8_t>();
  EXPECT_EQ(reserved, 0);

  // Checksum (2 bytes) - should be non-zero after calculation
  auto checksum = reader.readBE<uint16_t>();
  EXPECT_NE(checksum, 0);
}

TEST(ICMPExtHdrTest, hdrSerializeChecksumValidates) {
  auto ipObj =
      std::make_unique<ICMPExtIpSubObjectV4>(folly::IPAddressV4("10.0.0.1"));
  ICMPExtInterfaceInformationObject obj(std::move(ipObj), nullptr);

  ICMPExtHdr hdr;
  hdr.objects.push_back(&obj);

  auto buf = folly::IOBuf::create(hdr.length());
  buf->append(hdr.length());
  folly::io::RWPrivateCursor cursor(buf.get());
  hdr.serialize(&cursor);

  // Verify checksum by running internetChecksum over the whole buffer
  // A valid checksum should produce 0 when re-computed over the data
  folly::io::Cursor reader(buf.get());
  auto csum = PktUtil::internetChecksum(reader.data(), hdr.length());
  EXPECT_EQ(csum, 0);
}

TEST(ICMPExtHdrTest, hdrSerializeWithNameAndIpV6) {
  auto ipObj =
      std::make_unique<ICMPExtIpSubObjectV6>(folly::IPAddressV6("fd00::1"));
  auto nameObj = std::make_unique<ICMPExtIfaceNameSubObject>("port1");
  ICMPExtInterfaceInformationObject obj(std::move(ipObj), std::move(nameObj));

  ICMPExtHdr hdr;
  hdr.objects.push_back(&obj);

  auto buf = folly::IOBuf::create(hdr.length());
  buf->append(hdr.length());
  folly::io::RWPrivateCursor cursor(buf.get());
  hdr.serialize(&cursor);

  // Verify checksum validates
  folly::io::Cursor reader(buf.get());
  auto csum = PktUtil::internetChecksum(reader.data(), hdr.length());
  EXPECT_EQ(csum, 0);

  // Verify total length
  // Header (4) + obj header (4) + IPv6 sub-obj (20) + name sub-obj
  // "port1" = 5 chars, +2 = 7, padded to 8
  EXPECT_EQ(hdr.length(), 4 + 4 + 20 + 8);
}
