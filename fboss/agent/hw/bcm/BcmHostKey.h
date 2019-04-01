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

extern "C" {
#include <opennsl/types.h>
}

#include "fboss/agent/state/RouteNextHop.h"

namespace facebook { namespace fboss {

struct IHostKey {
  template <class Base>
  struct Interface : Base {
    opennsl_vrf_t getVrf() const {
      return folly::poly_call<0>(*this);
    }

    folly::IPAddress addr() const {
      return folly::poly_call<1>(*this);
    }

    folly::Optional<InterfaceID> intfID() const {
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

    template <class SelfType>
    friend bool operator<(SelfType const& lhs, SelfType const& rhs);

    template <class SelfType>
    friend bool operator==(SelfType const& lhs, SelfType const& rhs);
  }; // struct Interface

  template <class T>
  static bool eq(T const& lhs, T const& rhs) {
    return lhs == rhs;
  }

  template <class T>
  static bool lt(T const& lhs, T const& rhs) {
    return lhs < rhs;
  }

  template <class T>
  using Members = FOLLY_POLY_MEMBERS(
      &T::getVrf,
      &T::addr,
      &T::intfID,
      &T::hasLabel,
      &T::getLabel,
      &T::str,
      &eq<T>,
      &lt<T>);
}; // struct IHostKey

using HostKey = folly::Poly<IHostKey>;

class BcmHostKey {

 public:
  // Constructor based on the forward info
  BcmHostKey(opennsl_vrf_t vrf, const NextHop& fwd)
    : BcmHostKey(vrf, fwd.addr(), fwd.intfID()) {}

  // Constructor based on the IP address
  BcmHostKey(
      opennsl_vrf_t vrf,
      folly::IPAddress addr,
      folly::Optional<InterfaceID> intfID = folly::none);

  opennsl_vrf_t getVrf() const {
    return vrf_;
  }

  folly::IPAddress addr() const {
    return addr_;
  }

  folly::Optional<InterfaceID> intfID() const {
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

  std::string str() const;

  friend bool operator==(const BcmHostKey& a, const BcmHostKey& b);

  friend bool operator<(const BcmHostKey& a, const BcmHostKey& b);

 private:
  opennsl_vrf_t vrf_;
  folly::IPAddress addr_;
  folly::Optional<InterfaceID> intfID_;
};

class BcmLabeledHostKey {
 public:
  BcmLabeledHostKey(
      opennsl_vrf_t vrf,
      uint32_t label,
      folly::IPAddress addr,
      InterfaceID intfID)
      : vrf_(vrf), label_(label), addr_(addr), intfID_(intfID) {}

  BcmLabeledHostKey(
      opennsl_vrf_t vrf,
      LabelForwardingAction::LabelStack labels,
      folly::IPAddress addr,
      InterfaceID intfID)
      : vrf_(vrf), labels_(std::move(labels)), addr_(addr), intfID_(intfID) {
    if (labels_.size() == 0) {
      throw FbossError("invalid label stack for labeled host key");
    }
  }

  opennsl_vrf_t getVrf() const {
    return vrf_;
  }

  folly::IPAddress addr() const {
    return addr_;
  }

  folly::Optional<InterfaceID> intfID() const {
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

  std::string str() const;

  friend bool operator==(
      const BcmLabeledHostKey& rhs,
      const BcmLabeledHostKey& lhs);

  friend bool operator<(
      const BcmLabeledHostKey& rhs,
      const BcmLabeledHostKey& lhs);

 private:
  opennsl_vrf_t vrf_;
  uint32_t label_;
  LabelForwardingAction::LabelStack labels_;
  folly::IPAddress addr_;
  InterfaceID intfID_;
};

HostKey getNextHopKey(opennsl_vrf_t vrf, const NextHop& nexthop);
void toAppend(const HostKey& key, std::string* result);
std::ostream& operator<<(std::ostream& os, const HostKey& key);
}}
