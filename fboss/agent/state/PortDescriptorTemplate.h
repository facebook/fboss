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

#include <folly/Conv.h>
#include <folly/dynamic.h>
#include <folly/logging/xlog.h>

namespace {
constexpr auto kPortId = "portId";
constexpr auto kPortType = "portType";
} // namespace

namespace facebook::fboss {

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
    return type_ == PortType::PHYSICAL && physicalPortID_ == portID;
  }
  bool operator!=(const PortIdType portID) const {
    return !(*this == portID);
  }
  bool operator==(const TrunkIdType aggPortID) const {
    return type_ == PortType::AGGREGATE && aggregatePortID_ == aggPortID;
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
    folly::dynamic entry = folly::dynamic::object;
    switch (type()) {
      case PortType::PHYSICAL:
        // encode using old format for downgrade scenarios
        return folly::dynamic(static_cast<uint16_t>(physicalPortID_));
      case PortType::AGGREGATE:
        entry[kPortType] = static_cast<uint16_t>(PortType::AGGREGATE);
        entry[kPortId] = static_cast<uint16_t>(aggregatePortID_);
        return entry;
    }
    XLOG(FATAL) << "Unknown port type";
  }
  static PortDescriptorTemplate fromFollyDynamic(
      const folly::dynamic& descJSON) {
    // For backward compatibility, try to decode using
    // old format if type is not object. This can be removed later
    // after physical port also switch to new format
    if (descJSON.type() == folly::dynamic::Type::OBJECT) {
      switch ((PortType)descJSON[kPortType].asInt()) {
        case PortType::PHYSICAL:
          return PortDescriptorTemplate(PortIdType(descJSON[kPortId].asInt()));
        case PortType::AGGREGATE:
          return PortDescriptorTemplate(TrunkIdType(descJSON[kPortId].asInt()));
      }
      XLOG(FATAL) << "Unknown port type";
    } else {
      return PortDescriptorTemplate(PortIdType(descJSON.asInt()));
    }
  }
  PortIdType getPhysicalPortOrThrow() const {
    if (type() != PortType::PHYSICAL) {
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

template <typename PortIdT, typename TrunkIdT>
inline std::ostream& operator<<(
    std::ostream& out,
    const PortDescriptorTemplate<PortIdT, TrunkIdT>& pd) {
  return out << pd.str();
}

} // namespace facebook::fboss
