// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once
#include "fboss/agent/packet/IPPacket.h"
#include "fboss/agent/packet/MPLSHdr.h"

namespace facebook::fboss {

class HwSwitch;

namespace utility {
class MPLSPacket {
 public:
  // read entire label stack, and populate payloads, useful to parse RxPacket
  explicit MPLSPacket(folly::io::Cursor& cursor);

  // set header fields, useful to construct TxPacket
  explicit MPLSPacket(MPLSHdr hdr) : hdr_(std::move(hdr)) {}
  template <typename AddrT>
  MPLSPacket(MPLSHdr hdr, IPPacket<AddrT> payload) : hdr_(std::move(hdr)) {
    setPayLoad(std::move(payload));
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
      std::function<std::unique_ptr<facebook::fboss::TxPacket>(uint32_t)>
          allocatePacket) const;

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
    v6PayLoad_ = std::move(payload);
  }

  void setPayLoad(IPPacket<folly::IPAddressV4> payload) {
    v4PayLoad_ = std::move(payload);
  }

  MPLSHdr hdr_{MPLSHdr::Label{0, 0, false, 0}};
  std::optional<IPPacket<folly::IPAddressV4>> v4PayLoad_;
  std::optional<IPPacket<folly::IPAddressV6>> v6PayLoad_;
};

inline std::ostream& operator<<(std::ostream& os, const MPLSPacket& mplsPkt) {
  os << mplsPkt.toString();
  return os;
}

} // namespace utility
} // namespace facebook::fboss
