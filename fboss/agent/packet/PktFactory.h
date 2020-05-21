// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once

#include "fboss/agent/TxPacket.h"
#include "fboss/agent/packet/Ethertype.h"
#include "fboss/agent/types.h"

#include "fboss/agent/packet/EthHdr.h"
#include "fboss/agent/packet/IPv4Hdr.h"
#include "fboss/agent/packet/IPv6Hdr.h"
#include "fboss/agent/packet/MPLSHdr.h"
#include "fboss/agent/packet/UDPHeader.h"

namespace facebook::fboss {

class HwSwitch;

namespace utility {
class UDPDatagram {
 public:
  // read entire udp datagram, and populate payloads, useful to parse RxPacket
  explicit UDPDatagram(folly::io::Cursor& cursor);

  // set header fields, useful to construct TxPacket
  UDPDatagram(const UDPHeader& udpHdr, std::vector<uint8_t> payload)
      : udpHdr_(udpHdr), payload_(payload) {
    udpHdr_.length = udpHdr_.size() + payload_.size();
  }

  size_t length() const {
    return UDPHeader::size() + payload_.size();
  }

  UDPHeader header() const {
    return udpHdr_;
  }

  std::vector<uint8_t> payload() const {
    return payload_;
  }

  // construct TxPacket by encapsulating rabdom byte payload
  std::unique_ptr<facebook::fboss::TxPacket> getTxPacket(
      const HwSwitch* hw) const;

  void serialize(folly::io::RWPrivateCursor& cursor) const;

  bool operator==(const UDPDatagram& that) const {
    /* ignore checksum, */
    return std::tie(
               udpHdr_.srcPort, udpHdr_.dstPort, udpHdr_.length, payload_) ==
        std::tie(
               that.udpHdr_.srcPort,
               that.udpHdr_.dstPort,
               that.udpHdr_.length,
               that.payload_);
  }

 private:
  UDPHeader udpHdr_;
  std::vector<uint8_t> payload_{};
};

template <typename AddrT>
class IPPacket {
 public:
  using HdrT = std::conditional_t<
      std::is_same<AddrT, folly::IPAddressV4>::value,
      IPv4Hdr,
      IPv6Hdr>;
  // read entire ip packet, and populate payloads, useful to parse RxPacket
  explicit IPPacket(folly::io::Cursor& cursor);

  // set header fields, useful to construct TxPacket
  explicit IPPacket(const HdrT& hdr) : hdr_{hdr} {}

  IPPacket(const HdrT& hdr, UDPDatagram payload)
      : hdr_{hdr}, udpPayLoad_(payload) {
    if constexpr (std::is_same_v<HdrT, IPv4Hdr>) {
      hdr_.version = 4;
      hdr_.length = length();
      if (!hdr_.ttl) {
        hdr_.ttl = 128;
      }
      hdr_.ihl = (5 > hdr_.ihl) ? 5 : hdr_.ihl;
      hdr_.computeChecksum();
      hdr_.protocol = 17; /* udp */
    } else {
      hdr_.version = 6;
      hdr_.payloadLength = udpPayLoad_->length();
      if (!hdr_.hopLimit) {
        hdr_.hopLimit = 128;
      }
      hdr_.nextHeader = 17; /* udp */
    }
  }

  size_t length() const {
    return hdr_.size() + (udpPayLoad_ ? udpPayLoad_->length() : 0);
  }

  HdrT header() const {
    return hdr_;
  }

  std::optional<UDPDatagram> payload() const {
    return udpPayLoad_;
  }

  // construct TxPacket by encapsulating udp payload
  std::unique_ptr<facebook::fboss::TxPacket> getTxPacket(
      const HwSwitch* hw) const;

  void serialize(folly::io::RWPrivateCursor& cursor) const;

  bool operator==(const IPPacket<AddrT>& that) const {
    return std::tie(hdr_, udpPayLoad_) == std::tie(that.hdr_, that.udpPayLoad_);
  }

 private:
  void setUDPCheckSum(folly::IOBuf* buffer) const;
  HdrT hdr_;
  std::optional<UDPDatagram> udpPayLoad_;
  // TODO: support TCP segment
};

using IPv4Packet = IPPacket<folly::IPAddressV4>;
using IPv6Packet = IPPacket<folly::IPAddressV6>;

class MPLSPacket {
 public:
  // read entire label stack, and populate payloads, useful to parse RxPacket
  explicit MPLSPacket(folly::io::Cursor& cursor);

  // set header fields, useful to construct TxPacket
  explicit MPLSPacket(MPLSHdr hdr) : hdr_(std::move(hdr)) {}
  template <typename AddrT>
  MPLSPacket(MPLSHdr hdr, IPPacket<AddrT> payload) : hdr_(std::move(hdr)) {
    setPayLoad(payload);
  }

  MPLSHdr header() const {
    return hdr_;
  }

  size_t length() const {
    return hdr_.size() +
        (v4PayLoad_ ? v4PayLoad_->length()
                    : (v6PayLoad_ ? v6PayLoad_->length() : 0));
  }

  std::optional<IPPacket<folly::IPAddressV4>> v4PayLoad() const {
    return v4PayLoad_;
  }

  std::optional<IPPacket<folly::IPAddressV6>> v6PayLoad() const {
    return v6PayLoad_;
  }

  // construct TxPacket by encapsulating l3 payload
  std::unique_ptr<facebook::fboss::TxPacket> getTxPacket(
      const HwSwitch* hw) const;

  void serialize(folly::io::RWPrivateCursor& cursor) const;

  bool operator==(const MPLSPacket& that) const {
    return std::tie(hdr_, v4PayLoad_, v6PayLoad_) ==
        std::tie(that.hdr_, that.v4PayLoad_, that.v6PayLoad_);
  }

 private:
  void setPayLoad(IPPacket<folly::IPAddressV6> payload) {
    v6PayLoad_ = payload;
  }

  void setPayLoad(IPPacket<folly::IPAddressV4> payload) {
    v4PayLoad_ = payload;
  }

  MPLSHdr hdr_{MPLSHdr::Label{0, 0, 0, 0}};
  std::optional<IPPacket<folly::IPAddressV4>> v4PayLoad_;
  std::optional<IPPacket<folly::IPAddressV6>> v6PayLoad_;
};

class EthFrame {
 public:
  // read entire ethernet frame, and populate payloads, useful to parse RxPacket
  explicit EthFrame(folly::io::Cursor& cursor);

  // set header fields, useful to construct TxPacket
  explicit EthFrame(EthHdr hdr) : hdr_(std::move(hdr)) {}

  EthFrame(EthHdr hdr, MPLSPacket payload) : hdr_(std::move(hdr)) {
    mplsPayLoad_ = std::move(payload);
    hdr_.etherType = static_cast<uint16_t>(ETHERTYPE::ETHERTYPE_MPLS);
  }

  EthFrame(EthHdr hdr, IPPacket<folly::IPAddressV4> payload)
      : hdr_(std::move(hdr)), v4PayLoad_(payload) {
    hdr_.etherType = static_cast<uint16_t>(ETHERTYPE::ETHERTYPE_IPV4);
  }

  EthFrame(EthHdr hdr, IPPacket<folly::IPAddressV6> payload)
      : hdr_(std::move(hdr)), v6PayLoad_(payload) {
    hdr_.etherType = static_cast<uint16_t>(ETHERTYPE::ETHERTYPE_IPV6);
  }

  EthHdr header() const {
    return hdr_;
  }

  size_t length() const {
    auto len = 0;
    if (v4PayLoad_) {
      len += v4PayLoad_->length();
    } else if (v6PayLoad_) {
      len += v6PayLoad_->length();
    } else if (mplsPayLoad_) {
      len += mplsPayLoad_->length();
    }
    len += EthHdr::SIZE;
    return len;
  }
  // construct TxPacket by encapsulating payload
  std::unique_ptr<facebook::fboss::TxPacket> getTxPacket(
      const HwSwitch* hw) const;

  std::optional<IPPacket<folly::IPAddressV4>> v4PayLoad() const {
    return v4PayLoad_;
  }

  std::optional<IPPacket<folly::IPAddressV6>> v6PayLoad() const {
    return v6PayLoad_;
  }

  std::optional<MPLSPacket> mplsPayLoad() const {
    return mplsPayLoad_;
  }

  void serialize(folly::io::RWPrivateCursor& cursor) const;

  bool operator==(const EthFrame& that) const {
    return std::tie(hdr_, v4PayLoad_, v6PayLoad_, mplsPayLoad_) ==
        std::tie(
               that.hdr_, that.v4PayLoad_, that.v6PayLoad_, that.mplsPayLoad_);
  }

 private:
  EthHdr hdr_;
  std::optional<IPPacket<folly::IPAddressV4>> v4PayLoad_;
  std::optional<IPPacket<folly::IPAddressV6>> v6PayLoad_;
  std::optional<MPLSPacket> mplsPayLoad_;
};

template <typename AddrT>
EthFrame getEthFrame(
    folly::MacAddress srcMac,
    folly::MacAddress dstMac,
    AddrT srcIp,
    AddrT dstIp,
    uint16_t sPort,
    uint16_t dPort,
    uint16_t vlanId = 1,
    size_t payloadSize = 256);

template <typename AddrT>
EthFrame getEthFrame(
    folly::MacAddress srcMac,
    folly::MacAddress dstMac,
    std::vector<MPLSHdr::Label> labels,
    AddrT srcIp,
    AddrT dstIp,
    uint16_t sPort,
    uint16_t dPort);

} // namespace utility

} // namespace facebook::fboss
