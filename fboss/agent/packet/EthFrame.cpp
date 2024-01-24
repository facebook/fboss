// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/agent/packet/EthFrame.h"

#include <folly/io/Cursor.h>
#include <folly/io/IOBuf.h>

#include "fboss/agent/HwSwitch.h"
#include "fboss/agent/TxPacket.h"

namespace facebook::fboss::utility {

EthFrame::EthFrame(folly::io::Cursor& cursor) {
  hdr_ = EthHdr(cursor);
  switch (hdr_.etherType) {
    case static_cast<uint16_t>(ETHERTYPE::ETHERTYPE_IPV4):
      v4PayLoad_ = IPPacket<folly::IPAddressV4>(cursor);
      break;
    case static_cast<uint16_t>(ETHERTYPE::ETHERTYPE_IPV6):
      v6PayLoad_ = IPPacket<folly::IPAddressV6>(cursor);
      break;
    case static_cast<uint16_t>(ETHERTYPE::ETHERTYPE_MPLS):
      mplsPayLoad_ = MPLSPacket(cursor);
      break;
    case static_cast<uint16_t>(ETHERTYPE::ETHERTYPE_ARP):
      arpHdr_ = ArpHdr(cursor);
      break;
    case static_cast<uint16_t>(ETHERTYPE::ETHERTYPE_LLDP):
    case static_cast<uint16_t>(ETHERTYPE::ETHERTYPE_SLOW_PROTOCOLS):
    case static_cast<uint16_t>(ETHERTYPE::ETHERTYPE_EAPOL):
      // TODO - add support for parsing respective protocol PDUs
      break;
    default:
      throw FbossError("Unhandled etherType: ", hdr_.etherType);
  }
}

std::unique_ptr<facebook::fboss::TxPacket> EthFrame::getTxPacket(
    const HwSwitch* hw) const {
  auto txPacket = hw->allocatePacket(length());

  folly::io::RWPrivateCursor rwCursor(txPacket->buf());

  rwCursor.push(hdr_.dstAddr.bytes(), folly::MacAddress::SIZE);
  rwCursor.push(hdr_.srcAddr.bytes(), folly::MacAddress::SIZE);
  if (!hdr_.vlanTags.empty()) {
    for (const auto& vlanTag : hdr_.vlanTags) {
      rwCursor.template writeBE<uint32_t>(vlanTag.value);
    }
  }
  if (v4PayLoad_) {
    rwCursor.template writeBE<uint16_t>(
        static_cast<uint16_t>(ETHERTYPE::ETHERTYPE_IPV4));
    auto v4Packet = v4PayLoad_->getTxPacket(hw);
    folly::io::Cursor cursor(v4Packet->buf());
    rwCursor.push(cursor, v4PayLoad_->length());
  } else if (v6PayLoad_) {
    rwCursor.template writeBE<uint16_t>(
        static_cast<uint16_t>(ETHERTYPE::ETHERTYPE_IPV6));
    auto v6Packet = v6PayLoad_->getTxPacket(hw);
    folly::io::Cursor cursor(v6Packet->buf());
    rwCursor.push(cursor, v6PayLoad_->length());
  } else if (mplsPayLoad_) {
    rwCursor.template writeBE<uint16_t>(
        static_cast<uint16_t>(ETHERTYPE::ETHERTYPE_MPLS));
    auto mplsPacket = mplsPayLoad_->getTxPacket(hw);
    folly::io::Cursor cursor(mplsPacket->buf());
    rwCursor.push(cursor, mplsPayLoad_->length());
  } else if (arpHdr_) {
    arpHdr_->serialize(&rwCursor);
  }
  return txPacket;
}

void EthFrame::serialize(folly::io::RWPrivateCursor& cursor) const {
  CHECK_GE(cursor.totalLength(), length())
      << "Insufficient room to serialize packet";

  cursor.push(hdr_.dstAddr.bytes(), folly::MacAddress::SIZE);
  cursor.push(hdr_.srcAddr.bytes(), folly::MacAddress::SIZE);

  if (!hdr_.vlanTags.empty()) {
    for (const auto& vlanTag : hdr_.vlanTags) {
      cursor.template writeBE<uint32_t>(vlanTag.value);
    }
  }

  if (v4PayLoad_) {
    cursor.template writeBE<uint16_t>(
        static_cast<uint16_t>(ETHERTYPE::ETHERTYPE_IPV4));
    v4PayLoad_->serialize(cursor);
  } else if (v6PayLoad_) {
    cursor.template writeBE<uint16_t>(
        static_cast<uint16_t>(ETHERTYPE::ETHERTYPE_IPV6));
    v6PayLoad_->serialize(cursor);
  } else if (mplsPayLoad_) {
    cursor.template writeBE<uint16_t>(
        static_cast<uint16_t>(ETHERTYPE::ETHERTYPE_MPLS));
    mplsPayLoad_->serialize(cursor);
  } else if (arpHdr_) {
    cursor.template writeBE<uint16_t>(
        static_cast<uint16_t>(ETHERTYPE::ETHERTYPE_ARP));
    arpHdr_->serialize(&cursor);
  }
}

} // namespace facebook::fboss::utility
