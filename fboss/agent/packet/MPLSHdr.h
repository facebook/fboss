// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once

#include <folly/io/Cursor.h>
#include <optional>
#include <vector>

#include "fboss/agent/packet/Ethertype.h"

namespace facebook::fboss {

struct MPLSHdr {
 public:
  struct Label {
    static auto constexpr kSizeBytes = 4;
    uint8_t timeToLive;
    uint8_t bottomOfStack : 1;
    uint8_t trafficClass : 3;
    uint32_t label : 20;
    Label(uint32_t _val, uint8_t _tc, bool _bos, uint8_t _ttl)
        : timeToLive(_ttl),
          bottomOfStack(int(_bos)),
          trafficClass(_tc),
          label(_val) {}
    bool operator==(const Label& otherLabel) const {
      return otherLabel.label == label &&
          otherLabel.trafficClass == trafficClass &&
          otherLabel.bottomOfStack == bottomOfStack &&
          otherLabel.timeToLive == timeToLive;
    }
    uint32_t getLabelValue() const {
      return label;
    }
    bool isbottomOfStack() const {
      return bottomOfStack;
    }
    uint8_t getTrafficClass() const {
      return trafficClass;
    }
    uint8_t getTTL() const {
      return timeToLive;
    }
    std::string toString() const;
  };

  void decrementTTL() {
    for (auto& label : stack_) {
      if (label.isbottomOfStack()) {
        label.timeToLive =
            label.timeToLive > 0 ? label.timeToLive - 1 : label.timeToLive;
        break;
      }
    }
  }

 public:
  explicit MPLSHdr(Label mplsLabel) : stack_{mplsLabel} {}
  explicit MPLSHdr(std::vector<Label> mplsLabels)
      : stack_{std::move(mplsLabels)} {}
  explicit MPLSHdr(folly::io::Cursor* cursor);
  void serialize(folly::io::RWPrivateCursor* cursor) const;
  size_t size() const {
    return stack_.size() * Label::kSizeBytes;
  }
  std::vector<Label>& stack() {
    return stack_;
  }
  const std::vector<Label>& stack() const {
    return stack_;
  }
  bool operator==(const MPLSHdr& other) const {
    return stack_ == other.stack_;
  }

  MPLSHdr::Label getLookupLabel() const {
    return stack_[0];
  }
  std::string toString() const;

 private:
  union MPLSLabel {
    uint32_t u32;
    Label label;
    MPLSLabel() : u32{0} {}
  };
  std::vector<Label> stack_;
};

inline std::ostream& operator<<(std::ostream& os, const MPLSHdr& hdr) {
  os << hdr.toString();
  return os;
}
namespace utility {
std::optional<ETHERTYPE> decapsulateMplsPacket(folly::IOBuf* buf);
}

} // namespace facebook::fboss
