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
#include "fboss/agent/types.h"

#include <folly/dynamic.h>

namespace facebook { namespace fboss {

class PortDescriptor {
 public:
  enum class PortType {
    PHYSICAL,
  };
  explicit PortDescriptor(PortID p)
      : type_(PortType::PHYSICAL), physicalPortID_(p) {}

  PortType type() const {
    return type_;
  }
  PortID phyPortID() const {
    DCHECK(type() == PortType::PHYSICAL);
    return physicalPortID_;
  }
  bool operator==(const PortDescriptor& rhs) const {
    return type_ == rhs.type_ && physicalPortID_ == rhs.physicalPortID_;
  }
  bool operator==(const PortID portID) const {
    return type_ == PortDescriptor::PortType::PHYSICAL &&
        physicalPortID_ == portID;
  }
  bool operator!=(const PortID portID) const {
    return !(*this == portID);
  }
  int32_t asThriftPort() const {
    switch (type()) {
      case PortDescriptor::PortType::PHYSICAL:
        return static_cast<int32_t>(physicalPortID_);
      default:
        LOG(FATAL) << "PortDescriptor matching not exhaustive";
    }
  }
  folly::dynamic toFollyDynamic() const {
    switch (type()) {
      case PortDescriptor::PortType::PHYSICAL:
        return folly::dynamic(static_cast<uint16_t>(physicalPortID_));
      default:
        LOG(FATAL) << "PortDescriptor matching not exhaustive";
    }
  }
  static PortDescriptor fromFollyDynamic(const folly::dynamic& descJSON) {
    return PortDescriptor(PortID(descJSON.asInt()));
  }
  PortID getPhysicalPortOrThrow() const {
    if(type() != PortDescriptor::PortType::PHYSICAL) {
      throw FbossError("PortDescriptor is not physical");
    }

    return physicalPortID_;
  }

 private:
  PortType type_;
  PortID physicalPortID_;
};

}} // facebook::fboss
