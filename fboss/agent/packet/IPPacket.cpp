// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/agent/packet/IPPacket.h"

#include <folly/io/Cursor.h>
#include <folly/io/IOBuf.h>

#include <folly/IPAddressV4.h>
#include <folly/IPAddressV6.h>

#include "fboss/agent/HwSwitch.h"
#include "fboss/agent/TxPacket.h"
#include "fboss/agent/packet/IPv4Hdr.h"
#include "fboss/agent/packet/IPv6Hdr.h"
#include "fboss/agent/packet/PktUtil.h"

namespace facebook::fboss::utility {

namespace {
uint8_t nextHeader(const IPv6Hdr& hdr) {
  return hdr.nextHeader;
}

uint8_t nextHeader(const IPv4Hdr& hdr) {
  return hdr.protocol;
}
} // namespace
template <typename AddrT>
IPPacket<AddrT>::IPPacket(folly::io::Cursor& cursor) {
  hdr_ = HdrT(cursor);
  if (nextHeader(hdr_) == static_cast<uint8_t>(IP_PROTO::IP_PROTO_UDP)) {
    // if proto is udp, encapsulate udp
    udpPayLoad_ = UDPDatagram(cursor);
  } else if (nextHeader(hdr_) == static_cast<uint8_t>(IP_PROTO::IP_PROTO_TCP)) {
    tcpPayLoad_ = TCPPacket(cursor, hdr_.payloadSize());
  } else {
    ipPayload_ = std::vector<uint8_t>(hdr_.payloadSize());
    for (auto i = 0; i < hdr_.payloadSize(); ++i) {
      (*ipPayload_)[i] = cursor.read<uint8_t>();
    }
  }
}

template <typename AddrT>
void IPPacket<AddrT>::setUDPCheckSum(folly::IOBuf* buffer) const {
  CHECK(udpPayLoad_.has_value());
  folly::io::Cursor start(buffer);
  // jump to  payloag start.
  // skip ipv4 header and udp header to get to the start of payload
  start += (hdr_.size() + udpPayLoad_->header().size());
  // compute checksum
  auto udpHdr = udpPayLoad_->header();
  auto csum = udpHdr.computeChecksum(hdr_, start);
  folly::io::RWPrivateCursor rwCursor(buffer);
  rwCursor +=
      (hdr_.size() + sizeof(udpHdr.srcPort) + sizeof(udpHdr.dstPort) +
       sizeof(udpHdr.length));
  rwCursor.writeBE<uint16_t>(csum);
}

template <typename AddrT>
std::unique_ptr<facebook::fboss::TxPacket> IPPacket<AddrT>::getTxPacket(
    std::function<std::unique_ptr<facebook::fboss::TxPacket>(uint32_t)>
        allocatePacket) const {
  auto txPacket = allocatePacket(length());
  folly::io::RWPrivateCursor rwCursor(txPacket->buf());
  hdr_.serialize(&rwCursor);
  if (udpPayLoad_) {
    auto udpPkt = udpPayLoad_->getTxPacket(allocatePacket);
    folly::io::Cursor cursor(udpPkt->buf());
    rwCursor.push(cursor, udpPayLoad_->length());
    setUDPCheckSum(txPacket->buf());
  } else if (tcpPayLoad_) {
    auto tcpPkt = tcpPayLoad_->getTxPacket(allocatePacket);
    folly::io::Cursor cursor(tcpPkt->buf());
    rwCursor.push(cursor, tcpPayLoad_->length());
  } else if (ipPayload_) {
    for (auto byte : *ipPayload_) {
      rwCursor.write<uint8_t>(byte);
    }
  }
  return txPacket;
}

template <typename AddrT>
void IPPacket<AddrT>::serialize(folly::io::RWPrivateCursor& cursor) const {
  CHECK_GE(cursor.totalLength(), length())
      << "Insufficient room to serialize packet";

  hdr_.serialize(&cursor);
  if (udpPayLoad_) {
    udpPayLoad_->serialize(cursor);
  } else if (tcpPayLoad_) {
    tcpPayLoad_->serialize(cursor);
  } else if (ipPayload_) {
    for (auto byte : *ipPayload_) {
      cursor.write<uint8_t>(byte);
    }
  }
}

template <typename AddrT>
std::string IPPacket<AddrT>::toString() const {
  std::stringstream ss;
  ss << "IP hdr: " << hdr_
     << " UDP : " << (udpPayLoad_.has_value() ? udpPayLoad_->toString() : "")
     << " TCP: " << (tcpPayLoad_.has_value() ? tcpPayLoad_->toString() : "")
     << " L3 only payload: "
     << (ipPayload_.has_value()
             ? PktUtil::hexDump(
                   folly::IOBuf(
                       folly::IOBuf::CopyBufferOp::COPY_BUFFER,
                       ipPayload_->data(),
                       ipPayload_->size()))
             : "");
  return ss.str();
}
template class IPPacket<folly::IPAddressV4>;
template class IPPacket<folly::IPAddressV6>;

} // namespace facebook::fboss::utility
