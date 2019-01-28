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

class PortDescriptor {
 public:
  enum class PortType {
    PHYSICAL,
    AGGREGATE,
  };
  explicit PortDescriptor(PortID p)
      : type_(PortType::PHYSICAL), physicalPortID_(p) {}
  explicit PortDescriptor(AggregatePortID p)
      : type_(PortType::AGGREGATE), aggregatePortID_(p) {}

  static PortDescriptor fromRxPacket(const RxPacket& pkt) {
    if (pkt.isFromAggregatePort()) {
      return PortDescriptor(AggregatePortID(pkt.getSrcAggregatePort()));
    } else {
      return PortDescriptor(PortID(pkt.getSrcPort()));
    }
  }

  PortType type() const {
    return type_;
  }
  bool isPhysicalPort() const {
    return type() == PortType::PHYSICAL;
  }
  bool isAggregatePort() const {
    return type() == PortType::AGGREGATE;
  }

  PortID phyPortID() const {
    CHECK(type() == PortType::PHYSICAL);
    return physicalPortID_;
  }
  AggregatePortID aggPortID() const {
    CHECK(type() == PortType::AGGREGATE);
    return aggregatePortID_;
  }

  bool operator==(const PortDescriptor& rhs) const {
    return type_ == rhs.type_ &&
        ((type_ == PortType::PHYSICAL &&
          physicalPortID_ == rhs.physicalPortID_) ||
         (type_ == PortType::AGGREGATE &&
          aggregatePortID_ == rhs.aggregatePortID_));
  }
  bool operator<(const PortDescriptor& rhs) const {
    return std::tie(type_, physicalPortID_, aggregatePortID_) <
           std::tie(rhs.type_, rhs.physicalPortID_, rhs.aggregatePortID_);
  }
  bool operator!=(const PortDescriptor& rhs) const {
    return !(*this == rhs);
  }
  bool operator==(const PortID portID) const {
    return type_ == PortType::PHYSICAL &&
        physicalPortID_ == portID;
  }
  bool operator!=(const PortID portID) const {
    return !(*this == portID);
  }
  bool operator==(const AggregatePortID aggPortID) const {
    return type_ == PortType::AGGREGATE &&
        aggregatePortID_ == aggPortID;
  }
  bool operator!=(const AggregatePortID aggPortID) const {
    return !(*this == aggPortID);
  }
  int32_t asThriftPort() const {
    // TODO(samank): Until PortDescriptors are exposed in ctrl.ctrl
    // (tracked in t18482977), we simply multiplex AggregatePortID's
    // and PortID's over the same field.
    switch (type()) {
      case PortType::PHYSICAL:
        return static_cast<int32_t>(physicalPortID_);
      case PortType::AGGREGATE:
        return static_cast<int32_t>(aggregatePortID_);
      default:
        XLOG(FATAL) << "PortDescriptor matching not exhaustive";
    }
  }
  folly::dynamic toFollyDynamic() const {
    switch (type()) {
      case PortType::PHYSICAL:
        return folly::dynamic(static_cast<uint16_t>(physicalPortID_));
      case PortType::AGGREGATE:
        return folly::dynamic(static_cast<uint16_t>(aggregatePortID_));
      default:
        XLOG(FATAL) << "PortDescriptor matching not exhaustive";
    }
  }
  static PortDescriptor fromFollyDynamic(const folly::dynamic& descJSON) {
    // Until warm-boot support is implemented for aggregate ports, we assume
    // it is undefined behavior to warm-boot across a state with configured
    // aggregate ports
    return PortDescriptor(PortID(descJSON.asInt()));
  }
  PortID getPhysicalPortOrThrow() const {
    if(type() != PortType::PHYSICAL) {
      throw FbossError("PortDescriptor is not physical");
    }

    return physicalPortID_;
  }
  std::string str() const {
    if (type_ == PortType::AGGREGATE) {
      return folly::to<std::string>("AggregatePort-", aggregatePortID_);
    } else {
      return folly::to<std::string>("PhysicalPort-", physicalPortID_);
    }
  }

 private:
  PortType type_;
  PortID physicalPortID_;
  AggregatePortID aggregatePortID_;
};

// helper so port descriptors work directly in folly::to<string> expressions.
inline void toAppend(const PortDescriptor& pd, std::string* result) {
  folly::toAppend(pd.str(), result);
}
inline std::ostream& operator<<(std::ostream& out, const PortDescriptor& pd) {
  return out << pd.str();
}

}} // facebook::fboss
