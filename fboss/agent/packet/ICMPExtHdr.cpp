// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/agent/packet/ICMPExtHdr.h"
#include <fboss/agent/packet/PTPHeader.h>
#include <cstdint>
#include <cstring>
#include "fboss/agent/packet/PktUtil.h"

namespace facebook::fboss {

void ICMPExtObjectHdr::serialize(folly::io::RWPrivateCursor* cursor) {
  cursor->template writeBE<uint16_t>(length());
  cursor->template writeBE<uint8_t>(static_cast<uint8_t>(classNum));
  cursor->template writeBE<uint8_t>(cType());
}

uint16_t ICMPExtInterfaceInformationObject::length() const {
  uint16_t ret = ICMPExtObjectHdr::SIZE;
  if (ifaceCType_.ipAddress) {
    ret += this->ipObject_->length();
  }
  if (ifaceCType_.name) {
    ret += this->nameObject_->length();
  }

  return ret;
}

void ICMPExtInterfaceInformationObject::serialize(
    folly::io::RWPrivateCursor* cursor) {
  // Write the ICMP Extension Object Header
  ICMPExtObjectHdr::serialize(cursor);

  // Write all of the Sub Objects we're including in our cType.
  // Ordering matters here!  See RFC 5837 4.1
  // "information order is thus: ifIndex, IP Address Sub-Object, Interface
  // Name Sub-Object, and MTU"
  if (ifaceCType_.ipAddress) {
    this->ipObject_->serialize(cursor);
  }
  if (ifaceCType_.name) {
    this->nameObject_->serialize(cursor);
  }
}

void ICMPExtHdr::serialize(folly::io::RWPrivateCursor* cursor) {
  // Keep track of where we are to calculate the checksum
  folly::io::RWPrivateCursor csumCursor(*cursor);

  // Write the version (should be value 2) into the first 4 bits
  // of the header
  uint8_t version = 2 << 4;
  cursor->template write<uint8_t>(version);

  // Skip the reserved 12 bits (4 bits were borrowed from the version)
  cursor->template write<uint8_t>(0);

  // Write a placeholder for the checksum
  cursor->write<uint16_t>(0);

  for (auto obj : objects) {
    obj->serialize(cursor);
  }

  uint64_t extHdrLength =
      cursor->getCurrentPosition() - csumCursor.getCurrentPosition();

  // Calcualte the checksum,
  uint16_t checksum =
      PktUtil::internetChecksum(csumCursor.data(), extHdrLength);

  // Skip ahead past the version and reserved bits, re-write the checksum
  csumCursor.skip(2);
  csumCursor.template writeBE<uint16_t>(checksum);
}

uint32_t ICMPExtHdr::length() const {
  uint32_t ret = 4; // 4 bytes for ICMP Ext Header Size
  // Add the length of each Extended Object
  for (auto obj : objects) {
    ret += obj->length();
  }
  return ret;
}

void ICMPExtIpSubObjectV4::serialize(folly::io::RWPrivateCursor* cursor) {
  cursor->template writeBE<uint16_t>(
      static_cast<uint16_t>(ICMPExtInterfaceIPSubObjectAFI::
                                ICMP_EXT_INTERFACE_IP_SUB_OBJECT_AFI_IPV4));
  cursor->template writeBE<uint16_t>(0); // Reserved
  cursor->template write<uint32_t>(this->ipAddress_.toLong());
}

void ICMPExtIpSubObjectV6::serialize(folly::io::RWPrivateCursor* cursor) {
  cursor->template writeBE<uint16_t>(
      static_cast<uint16_t>(ICMPExtInterfaceIPSubObjectAFI::
                                ICMP_EXT_INTERFACE_IP_SUB_OBJECT_AFI_IPV6));
  cursor->template writeBE<uint16_t>(0); // Reserved
  // IPv6 address
  cursor->push(this->ipAddress_.bytes(), 16);
}

ICMPExtIfaceNameSubObject::ICMPExtIfaceNameSubObject(const char* name) {
  strncpy(this->ifaceName_, name, sizeof(this->ifaceName_) - 1);
}

uint32_t ICMPExtIfaceNameSubObject::length() const {
  uint8_t size = 2 +
      strlen(this->ifaceName_); // Allow 2 bytes for size and null-terminating
  uint8_t padding = (4 - (size % 4)) % 4;
  return size + padding;
}

void ICMPExtIfaceNameSubObject::serialize(
    folly::io::RWPrivateCursor* cursor) const {
  // Work out the size to write, ensure that we're padded to a 4-byte boundry.
  // (including length octet).
  cursor->template write<uint8_t>(length());
  cursor->push(
      (const uint8_t*)this->ifaceName_, length() - 1); // -1 to exclude length
}

} // namespace facebook::fboss
