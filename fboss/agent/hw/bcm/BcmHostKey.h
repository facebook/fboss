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

#include <folly/poly/Regular.h>

extern "C" {
#include <bcm/types.h>
}

#include "fboss/agent/state/RouteNextHop.h"

namespace facebook::fboss {

struct IHostKey : folly::PolyExtends<
                      folly::poly::IEqualityComparable,
                      folly::poly::IStrictlyOrderable> {
  template <class Base>
  struct Interface : Base {
    bcm_vrf_t getVrf() const {
      return folly::poly_call<0>(*this);
    }

    folly::IPAddress addr() const {
      return folly::poly_call<1>(*this);
    }

    std::optional<InterfaceID> intfID() const {
      return folly::poly_call<2>(*this);
    }

    bool hasLabel() const {
      return folly::poly_call<3>(*this);
    }

    uint32_t getLabel() const {
      return folly::poly_call<4>(*this);
    }

    std::string str() const {
      return folly::poly_call<5>(*this);
    }

    bool needsMplsTunnel() const {
      return folly::poly_call<6>(*this);
    }

    LabelForwardingAction::LabelStack tunnelLabelStack() const {
      return folly::poly_call<7>(*this);
    }
  }; // struct Interface

  template <class T>
  using Members = FOLLY_POLY_MEMBERS(
      &T::getVrf,
      &T::addr,
      &T::intfID,
      &T::hasLabel,
      &T::getLabel,
      &T::str,
      &T::needsMplsTunnel,
      &T::tunnelLabelStack);
}; // struct IHostKey

using HostKey = folly::Poly<IHostKey>;

class BcmHostKey {
 public:
  // Constructor based on the forward info
  BcmHostKey(bcm_vrf_t vrf, const NextHop& fwd)
      : BcmHostKey(vrf, fwd.addr(), fwd.intfID()) {}

  // Constructor based on the IP address
  BcmHostKey(
      bcm_vrf_t vrf,
      folly::IPAddress addr,
      std::optional<InterfaceID> intfID = std::nullopt);

  bcm_vrf_t getVrf() const {
    return vrf_;
  }

  folly::IPAddress addr() const {
    return addr_;
  }

  std::optional<InterfaceID> intfID() const {
    return intfID_;
  }

  InterfaceID intf() const {
    // could throw if intfID_ does not have value
    return intfID_.value();
  }

  bool hasLabel() const {
    return false;
  }

  uint32_t getLabel() const {
    throw FbossError("unlabeled host key has no label");
    return 0;
  }

  bool needsMplsTunnel() const {
    return false;
  }

  LabelForwardingAction::LabelStack tunnelLabelStack() const {
    throw FbossError("unlabeled host key has no label stack");
    folly::assume_unreachable();
  }

  std::string str() const;

  friend bool operator==(const BcmHostKey& a, const BcmHostKey& b);

  friend bool operator<(const BcmHostKey& a, const BcmHostKey& b);

 private:
  bcm_vrf_t vrf_;
  folly::IPAddress addr_;
  std::optional<InterfaceID> intfID_;
};

class BcmLabeledHostKey {
 public:
  BcmLabeledHostKey(
      bcm_vrf_t vrf,
      uint32_t label,
      folly::IPAddress addr,
      InterfaceID intfID)
      : vrf_(vrf), label_(label), addr_(addr), intfID_(intfID) {}

  BcmLabeledHostKey(
      bcm_vrf_t vrf,
      LabelForwardingAction::LabelStack labels,
      folly::IPAddress addr,
      InterfaceID intfID)
      : vrf_(vrf), labels_(std::move(labels)), addr_(addr), intfID_(intfID) {
    if (labels_.size() == 0) {
      throw FbossError("invalid label stack for labeled host key");
    }
  }

  bcm_vrf_t getVrf() const {
    return vrf_;
  }

  folly::IPAddress addr() const {
    return addr_;
  }

  std::optional<InterfaceID> intfID() const {
    return intfID_;
  }

  InterfaceID intf() const {
    // could throw if intfID_ does not have value
    return intfID_;
  }

  bool hasLabel() const {
    return true;
  }

  uint32_t getLabel() const {
    return labels_.size() == 0 ? label_ : labels_.front();
  }

  bool needsMplsTunnel() const {
    return labels_.size() > 0;
  }

  LabelForwardingAction::LabelStack tunnelLabelStack() const {
    if (!needsMplsTunnel()) {
      throw FbossError(
          "tunnel label stack requested for next hop that doesn't require "
          "labels");
    }
    return labels_;
  }

  std::string str() const;

  friend bool operator==(
      const BcmLabeledHostKey& rhs,
      const BcmLabeledHostKey& lhs);

  friend bool operator<(
      const BcmLabeledHostKey& rhs,
      const BcmLabeledHostKey& lhs);

 private:
  bcm_vrf_t vrf_;
  uint32_t label_{0};
  LabelForwardingAction::LabelStack labels_;
  folly::IPAddress addr_;
  InterfaceID intfID_;
};

HostKey getNextHopKey(bcm_vrf_t vrf, const NextHop& nexthop);
void toAppend(const HostKey& key, std::string* result);
std::ostream& operator<<(std::ostream& os, const HostKey& key);

} // namespace facebook::fboss
