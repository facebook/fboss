/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#pragma once

#include "fboss/agent/FbossError.h"
#include "fboss/agent/RxPacket.h"
#include "fboss/agent/types.h"

#include <folly/Conv.h>
#include <folly/dynamic.h>
#include <folly/logging/xlog.h>

namespace facebook { namespace fboss {

template <typename PortIdType, typename TrunkIdType>
class PortDescriptorTemplate {
 public:
  enum class PortType {
    PHYSICAL,
    AGGREGATE,
  };
  explicit PortDescriptorTemplate(PortIdType p)
      : type_(PortType::PHYSICAL), physicalPortID_(p) {}
  explicit PortDescriptorTemplate(TrunkIdType p)
      : type_(PortType::AGGREGATE), aggregatePortID_(p) {}

  PortType type() const {
    return type_;
  }
  bool isPhysicalPort() const {
    return type() == PortType::PHYSICAL;
  }
  bool isAggregatePort() const {
    return type() == PortType::AGGREGATE;
  }

  PortIdType phyPortID() const {
    CHECK(type() == PortType::PHYSICAL);
    return physicalPortID_;
  }
  TrunkIdType aggPortID() const {
    CHECK(type() == PortType::AGGREGATE);
    return aggregatePortID_;
  }

  bool operator==(const PortDescriptorTemplate& rhs) const {
    return std::tie(type_, physicalPortID_, aggregatePortID_) ==
        std::tie(rhs.type_, rhs.physicalPortID_, rhs.aggregatePortID_);
  }
  bool operator<(const PortDescriptorTemplate& rhs) const {
    return std::tie(type_, physicalPortID_, aggregatePortID_) <
           std::tie(rhs.type_, rhs.physicalPortID_, rhs.aggregatePortID_);
  }
  bool operator!=(const PortDescriptorTemplate& rhs) const {
    return !(*this == rhs);
  }
  bool operator==(const PortIdType portID) const {
    return type_ == PortType::PHYSICAL &&
        physicalPortID_ == portID;
  }
  bool operator!=(const PortIdType portID) const {
    return !(*this == portID);
  }
  bool operator==(const TrunkIdType aggPortID) const {
    return type_ == PortType::AGGREGATE &&
        aggregatePortID_ == aggPortID;
  }
  bool operator!=(const TrunkIdType aggPortID) const {
    return !(*this == aggPortID);
  }
  int32_t asThriftPort() const {
    // TODO(samank): Until PortDescriptorTemplates are exposed in ctrl.ctrl
    // (tracked in t18482977), we simply multiplex TrunkIdType's
    // and PortID's over the same field.
    switch (type()) {
      case PortType::PHYSICAL:
        return static_cast<int32_t>(physicalPortID_);
      case PortType::AGGREGATE:
        return static_cast<int32_t>(aggregatePortID_);
    }
    XLOG(FATAL) << "Unknown port type";
  }
  folly::dynamic toFollyDynamic() const {
    switch (type()) {
      case PortType::PHYSICAL:
        return folly::dynamic(static_cast<uint16_t>(physicalPortID_));
      case PortType::AGGREGATE:
        return folly::dynamic(static_cast<uint16_t>(aggregatePortID_));
    }
    XLOG(FATAL) << "Unknown port type";
  }
  static PortDescriptorTemplate fromFollyDynamic(
      const folly::dynamic& descJSON) {
    // Until warm-boot support is implemented for aggregate ports, we assume
    // it is undefined behavior to warm-boot across a state with configured
    // aggregate ports
    return PortDescriptorTemplate(PortIdType(descJSON.asInt()));
  }
  PortIdType getPhysicalPortOrThrow() const {
    if(type() != PortType::PHYSICAL) {
      throw FbossError("PortDescriptor is not physical");
    }

    return physicalPortID_;
  }
  std::string str() const {
    return isPhysicalPort()
        ? folly::to<std::string>("PhysicalPort-", physicalPortID_)
        : folly::to<std::string>("AggregatePort-", aggregatePortID_);
  }
 private:
  PortType type_;
  PortIdType physicalPortID_{0};
  TrunkIdType aggregatePortID_{0};
};

class PortDescriptor : public PortDescriptorTemplate<PortID, AggregatePortID> {
 public:
  using BaseT = PortDescriptorTemplate<PortID, AggregatePortID>;
  explicit PortDescriptor(PortID p) : BaseT(p) {}
  explicit PortDescriptor(AggregatePortID p) : BaseT(p) {}

  static PortDescriptor fromRxPacket(const RxPacket& pkt) {
    if (pkt.isFromAggregatePort()) {
      return PortDescriptor(AggregatePortID(pkt.getSrcAggregatePort()));
    } else {
      return PortDescriptor(PortID(pkt.getSrcPort()));
    }
  }

  static PortDescriptor fromFollyDynamic(const folly::dynamic& descJSON) {
    return PortDescriptor(BaseT::fromFollyDynamic(descJSON));
  }

 private:
  explicit PortDescriptor(const BaseT& b) : BaseT(b) {}
};

// helper so port descriptors work directly in folly::to<string> expressions.
inline void toAppend(const PortDescriptor& pd, std::string* result) {
  folly::toAppend(pd.str(), result);
}
inline std::ostream& operator<<(std::ostream& out, const PortDescriptor& pd) {
  return out << pd.str();
}
}} // facebook::fboss
