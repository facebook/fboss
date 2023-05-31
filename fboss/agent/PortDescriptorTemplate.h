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

#include "fboss/agent/types.h"

#include <folly/Conv.h>
#include <folly/dynamic.h>
#include <folly/logging/xlog.h>

namespace {
constexpr auto kPortId = "portId";
constexpr auto kPortType = "portType";
} // namespace

namespace facebook::fboss {

template <typename PortIdType, typename TrunkIdType, typename SystemPortIdType>
class PortDescriptorTemplate {
 public:
  enum class PortType {
    PHYSICAL,
    AGGREGATE,
    SYSTEM_PORT,
  };
  PortDescriptorTemplate() {}
  explicit PortDescriptorTemplate(PortIdType p)
      : type_(PortType::PHYSICAL), physicalPortID_(p) {}
  explicit PortDescriptorTemplate(TrunkIdType p)
      : type_(PortType::AGGREGATE), aggregatePortID_(p) {}
  explicit PortDescriptorTemplate(SystemPortIdType p)
      : type_(PortType::SYSTEM_PORT), systemPortID_(p) {}

  PortType type() const {
    return type_;
  }
  bool isPhysicalPort() const {
    return type() == PortType::PHYSICAL;
  }
  bool isAggregatePort() const {
    return type() == PortType::AGGREGATE;
  }

  bool isSystemPort() const {
    return type() == PortType::SYSTEM_PORT;
  }
  PortIdType phyPortID() const {
    CHECK(isPhysicalPort());
    return physicalPortID_;
  }
  std::optional<PortIdType> phyPortIDIf() const {
    return isPhysicalPort() ? std::optional<PortIdType>(physicalPortID_)
                            : std::nullopt;
  }
  TrunkIdType aggPortID() const {
    CHECK(isAggregatePort());
    return aggregatePortID_;
  }
  std::optional<TrunkIdType> aggPortIDIf() const {
    return isAggregatePort() ? std::optional<TrunkIdType>(aggregatePortID_)
                             : std::nullopt;
  }
  SystemPortIdType sysPortID() const {
    CHECK(isSystemPort());
    return systemPortID_;
  }
  std::optional<SystemPortIdType> sysPortIDIf() const {
    return isSystemPort() ? std::optional<SystemPortIdType>(systemPortID_)
                          : std::nullopt;
  }

  uint64_t intID() const {
    switch (type()) {
      case PortType::PHYSICAL:
        return phyPortID();
      case PortType::AGGREGATE:
        return aggPortID();
      case PortType::SYSTEM_PORT:
        return sysPortID();
    }
    XLOG(FATAL) << "Unknown port type ";
  }
  bool operator==(const PortDescriptorTemplate& rhs) const {
    return std::tie(type_, physicalPortID_, aggregatePortID_, systemPortID_) ==
        std::tie(
               rhs.type_,
               rhs.physicalPortID_,
               rhs.aggregatePortID_,
               rhs.systemPortID_);
  }
  bool operator<(const PortDescriptorTemplate& rhs) const {
    return std::tie(type_, physicalPortID_, aggregatePortID_, systemPortID_) <
        std::tie(
               rhs.type_,
               rhs.physicalPortID_,
               rhs.aggregatePortID_,
               rhs.systemPortID_);
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
      case PortType::SYSTEM_PORT:
        return static_cast<int32_t>(systemPortID_);
    }
    XLOG(FATAL) << "Unknown port type";
  }

  cfg::PortDescriptor toThrift() const {
    cfg::PortDescriptor portTh;
    portTh.portId() = static_cast<int16_t>(asThriftPort());
    portTh.portType() = cfg::PortDescriptorType(type_);
    return portTh;
  }

  static PortDescriptorTemplate fromThrift(const cfg::PortDescriptor& portTh) {
    switch (portTh.get_portType()) {
      case cfg::PortDescriptorType::Physical:
        return PortDescriptorTemplate(PortIdType(portTh.get_portId()));
      case cfg::PortDescriptorType::Aggregate:
        return PortDescriptorTemplate(TrunkIdType(portTh.get_portId()));
      case cfg::PortDescriptorType::SystemPort:
        return PortDescriptorTemplate(SystemPortIdType(portTh.get_portId()));
    }
    XLOG(FATAL) << "Unknown port type";
  }

  folly::dynamic toFollyDynamic() const {
    folly::dynamic entry = folly::dynamic::object;
    switch (type()) {
      case PortType::PHYSICAL:
        entry[kPortType] = static_cast<uint16_t>(PortType::PHYSICAL);
        entry[kPortId] = static_cast<uint16_t>(physicalPortID_);
        break;
      case PortType::AGGREGATE:
        entry[kPortType] = static_cast<uint16_t>(PortType::AGGREGATE);
        entry[kPortId] = static_cast<uint16_t>(aggregatePortID_);
        break;
      case PortType::SYSTEM_PORT:
        entry[kPortType] = static_cast<uint16_t>(PortType::SYSTEM_PORT);
        entry[kPortId] = static_cast<uint16_t>(systemPortID_);
        break;
      default:
        XLOG(FATAL) << "Unknown port type";
    }
    return entry;
  }
  static PortDescriptorTemplate fromFollyDynamic(
      const folly::dynamic& descJSON) {
    switch ((PortType)descJSON[kPortType].asInt()) {
      case PortType::PHYSICAL:
        return PortDescriptorTemplate(PortIdType(descJSON[kPortId].asInt()));
      case PortType::AGGREGATE:
        return PortDescriptorTemplate(TrunkIdType(descJSON[kPortId].asInt()));
      case PortType::SYSTEM_PORT:
        return PortDescriptorTemplate(
            SystemPortIdType(descJSON[kPortId].asInt()));
    }
    XLOG(FATAL) << "Unknown port type";
  }
  std::string str() const {
    switch (type()) {
      case PortType::PHYSICAL:
        return folly::to<std::string>("PhysicalPort-", physicalPortID_);
      case PortType::AGGREGATE:
        return folly::to<std::string>("AggregatePort-", aggregatePortID_);
      case PortType::SYSTEM_PORT:
        return folly::to<std::string>("SystemPort-", systemPortID_);
    }
    XLOG(FATAL) << "Unknown port type";
  }

 private:
  PortType type_{PortType::PHYSICAL};
  PortIdType physicalPortID_{0};
  TrunkIdType aggregatePortID_{0};
  SystemPortIdType systemPortID_{0};
};

template <typename PortIdT, typename TrunkIdT, typename SystemPortIdTypeT>
inline std::ostream& operator<<(
    std::ostream& out,
    const PortDescriptorTemplate<PortIdT, TrunkIdT, SystemPortIdTypeT>& pd) {
  return out << pd.str();
}

using BasePortDescriptor =
    PortDescriptorTemplate<PortID, AggregatePortID, SystemPortID>;

} // namespace facebook::fboss

namespace std {
template <>
struct hash<facebook::fboss::BasePortDescriptor> {
  size_t operator()(const facebook::fboss::BasePortDescriptor& key) const;
};
} // namespace std
