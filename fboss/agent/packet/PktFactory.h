// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once

#include "fboss/agent/FbossError.h"
#include "fboss/agent/TxPacket.h"
#include "fboss/agent/packet/Ethertype.h"
#include "fboss/agent/types.h"

#include "fboss/agent/packet/EthHdr.h"
#include "fboss/agent/packet/IPv4Hdr.h"
#include "fboss/agent/packet/IPv6Hdr.h"
#include "fboss/agent/packet/MPLSHdr.h"
#include "fboss/agent/packet/TCPPacket.h"
#include "fboss/agent/packet/UDPDatagram.h"

#include <optional>

namespace facebook::fboss {

class HwSwitch;

namespace utility {

using AllocatePktFn = std::function<std::unique_ptr<TxPacket>(uint32_t)>;

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

  IPPacket(const HdrT& hdr, const UDPDatagram& payload)
      : hdr_{hdr}, udpPayLoad_(payload) {
    fillIPHdr();
  }
  IPPacket(const HdrT& hdr, const TCPPacket& payload)
      : hdr_{hdr}, tcpPayLoad_(payload) {
    fillIPHdr();
  }
  size_t length() const {
    return hdr_.size() + payloadLength();
  }
  HdrT header() const {
    return hdr_;
  }

  const std::optional<UDPDatagram>& udpPayload() const {
    return udpPayLoad_;
  }
  const std::optional<TCPPacket>& tcpPayload() const {
    return tcpPayLoad_;
  }

  // construct TxPacket by encapsulating udp payload
  std::unique_ptr<facebook::fboss::TxPacket> getTxPacket(
      const HwSwitch* hw) const;

  void serialize(folly::io::RWPrivateCursor& cursor) const;

  bool operator==(const IPPacket<AddrT>& that) const {
    return std::tie(hdr_, udpPayLoad_, tcpPayLoad_) ==
        std::tie(that.hdr_, that.udpPayLoad_, that.tcpPayLoad_);
  }
  void decrementTTL() {
    hdr_.decrementTTL();
  }
  std::string toString() const;

 private:
  size_t payloadLength() const {
    if (udpPayLoad_) {
      return udpPayLoad_->length();
    } else if (tcpPayLoad_) {
      return tcpPayLoad_->length();
    }
    return 0;
  }

  size_t protocol() const {
    if (udpPayLoad_) {
      return 17;
    } else if (tcpPayLoad_) {
      return 6;
    }
    throw FbossError("No payload set");
  }

  void fillIPHdr() {
    if constexpr (std::is_same_v<HdrT, IPv4Hdr>) {
      hdr_.version = 4;
      hdr_.length = length();
      hdr_.ihl = (5 > hdr_.ihl) ? 5 : hdr_.ihl;
      if (!hdr_.ttl) {
        hdr_.ttl = 128;
      }
      hdr_.computeChecksum();
      hdr_.protocol = protocol();
    } else {
      hdr_.version = 6;
      hdr_.payloadLength = payloadLength();
      if (!hdr_.hopLimit) {
        hdr_.hopLimit = 128;
      }
      hdr_.nextHeader = protocol();
    }
  }

  void setUDPCheckSum(folly::IOBuf* buffer) const;
  HdrT hdr_;
  std::optional<UDPDatagram> udpPayLoad_;
  std::optional<TCPPacket> tcpPayLoad_;
};

template <typename AddrT>
inline std::ostream& operator<<(
    std::ostream& os,
    const IPPacket<AddrT>& ipPkt) {
  os << ipPkt.toString();
  return os;
}

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

  void decrementTTL() {
    hdr_.decrementTTL();
    if (v6PayLoad_.has_value()) {
      v6PayLoad_->decrementTTL();
    } else if (v4PayLoad_.has_value()) {
      v4PayLoad_->decrementTTL();
    }
  }

  std::string toString() const;

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

inline std::ostream& operator<<(std::ostream& os, const MPLSPacket& mplsPkt) {
  os << mplsPkt.toString();
  return os;
}

class EthFrame {
 public:
  static constexpr size_t FCS_SIZE = 4;
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
    len += hdr_.size();
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
  bool operator!=(const EthFrame& that) const {
    return !(*this == that);
  }
  void decrementTTL() {
    if (mplsPayLoad_.has_value()) {
      mplsPayLoad_->decrementTTL();
    } else if (v6PayLoad_.has_value()) {
      v6PayLoad_->decrementTTL();
    } else if (v4PayLoad_.has_value()) {
      v4PayLoad_->decrementTTL();
    }
  }
  void setDstMac(const folly::MacAddress& dstMac) {
    hdr_.setDstMac(dstMac);
  }
  void setVlan(VlanID vlan) {
    hdr_.setVlan(vlan);
  }
  std::string toString() const;

 private:
  EthHdr hdr_;
  std::optional<IPPacket<folly::IPAddressV4>> v4PayLoad_;
  std::optional<IPPacket<folly::IPAddressV6>> v6PayLoad_;
  std::optional<MPLSPacket> mplsPayLoad_;
};

inline std::ostream& operator<<(std::ostream& os, const EthFrame& ethFrame) {
  os << ethFrame.toString();
  return os;
}
template <typename AddrT>
EthFrame getEthFrame(
    folly::MacAddress srcMac,
    folly::MacAddress dstMac,
    AddrT srcIp,
    AddrT dstIp,
    uint16_t sPort,
    uint16_t dPort,
    VlanID vlanId,
    size_t payloadSize = 256);

template <typename AddrT>
EthFrame getEthFrame(
    folly::MacAddress srcMac,
    folly::MacAddress dstMac,
    std::vector<MPLSHdr::Label> labels,
    AddrT srcIp,
    AddrT dstIp,
    uint16_t sPort,
    uint16_t dPort,
    VlanID vlanId = VlanID(1));

EthFrame makeEthFrame(const TxPacket& txPkt, bool skipTtlDecrement = false);

EthFrame makeEthFrame(const TxPacket& txPkt, folly::MacAddress dstMac);

EthFrame
makeEthFrame(const TxPacket& txPkt, folly::MacAddress dstMac, VlanID vlan);
EthHdr makeEthHdr(
    folly::MacAddress srcMac,
    folly::MacAddress dstMac,
    std::optional<VlanID> vlan,
    ETHERTYPE etherType);

std::unique_ptr<TxPacket> makeEthTxPacket(
    const AllocatePktFn& allocateTxPkt,
    std::optional<VlanID> vlan,
    folly::MacAddress srcMac,
    folly::MacAddress dstMac,
    facebook::fboss::ETHERTYPE etherType,
    std::optional<std::vector<uint8_t>> payload);
} // namespace utility

template <typename CursorType>
void writeEthHeader(
    const std::unique_ptr<TxPacket>& txPacket,
    CursorType* cursor,
    folly::MacAddress dst,
    folly::MacAddress src,
    std::vector<VlanTag> vlanTags,
    uint16_t protocol) {
  if (vlanTags.size() != 0) {
    txPacket->writeEthHeader(
        cursor, dst, src, VlanID(vlanTags[0].vid()), protocol);
  } else {
    txPacket->writeEthHeader(cursor, dst, src, protocol);
  }
}

} // namespace facebook::fboss
