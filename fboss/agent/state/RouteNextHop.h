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

#include <folly/dynamic.h>
#include <folly/IPAddress.h>
#include <folly/Poly.h>
#include <folly/poly/Regular.h>
#include <folly/Range.h>

#include "fboss/agent/AddressUtil.h"
#include "fboss/agent/if/gen-cpp2/ctrl_types.h"
#include "fboss/agent/types.h"
#include "fboss/agent/state/StateUtils.h"

namespace facebook { namespace fboss {

inline folly::StringPiece constexpr kInterface() {
  return "interface";
}

inline folly::StringPiece constexpr kNexthop() {
  return "nexthop";
}


// weights are represented as a percentage between 0 and 100
using NextHopWeight = uint32_t;

struct INextHop {
  // In this context "Interface" does not refer to network interfaces
  // but is rather an implementation detail of folly::Poly. This is
  // the "well-known" name you must use for your interface definition
  // for things like poly_call to work properly.
  template <class Base>
  struct Interface : Base {
    folly::Optional<InterfaceID> intfID() const {
      return folly::poly_call<0>(*this);
    }

    folly::IPAddress addr() const {
      return folly::poly_call<1>(*this);
    }

    bool isResolved() const {
      return intfID().hasValue();
    }

    InterfaceID intf() const {
      return intfID().value();
    }

    folly::dynamic toFollyDynamic() const {
      folly::dynamic nh = folly::dynamic::object;
      nh[kNexthop()] = addr().str();
      if (isResolved()) {
        nh[kInterface()] = static_cast<uint32_t>(intf());
      }
      return nh;
    }

    NextHopThrift toThrift() const {
      NextHopThrift nht;
      nht.address = network::toBinaryAddress(addr());
      if (isResolved()) {
        nht.address.set_ifName(util::createTunIntfName(intf()));
      }
      return nht;
    }

    std::string str() const {
      if (isResolved()) {
        return folly::to<std::string>(addr(), "@I", intf());
      } else {
        return folly::to<std::string>(addr());
      }
    }
  };

  template <class T>
  using Members = FOLLY_POLY_MEMBERS(&T::intfID, &T::addr);
};


using NextHop = folly::Poly<INextHop>;
void toAppend(const NextHop& nhop, std::string *result);
std::ostream& operator<<(std::ostream& os, const NextHop& nhop);
bool operator<(const NextHop& a, const NextHop& b);
bool operator>(const NextHop& a, const NextHop& b);
bool operator<=(const NextHop& a, const NextHop& b);
bool operator>=(const NextHop& a, const NextHop& b);
bool operator==(const NextHop& a, const NextHop& b);
bool operator!=(const NextHop& a, const NextHop& b);

class ResolvedNextHop {
 public:
  ResolvedNextHop(const folly::IPAddress& addr, InterfaceID intfID)
      : addr_(addr), intfID_(intfID) {}
  ResolvedNextHop(folly::IPAddress&& addr, InterfaceID intfID)
      : addr_(std::move(addr)), intfID_(intfID) {}
  folly::Optional<InterfaceID> intfID() const { return intfID_; }
  folly::IPAddress addr() const { return addr_; }
 private:
  folly::IPAddress addr_;
  InterfaceID intfID_;
};

bool operator==(const ResolvedNextHop& a, const ResolvedNextHop& b);

class UnresolvedNextHop {
 public:
  explicit UnresolvedNextHop(const folly::IPAddress& addr);
  explicit UnresolvedNextHop(folly::IPAddress&& addr);
  folly::Optional<InterfaceID> intfID() const { return folly::none; }
  folly::IPAddress addr() const { return addr_; }
 private:
  folly::IPAddress addr_;
};

bool operator==(const UnresolvedNextHop& a, const UnresolvedNextHop& b);

namespace util {
NextHop fromThrift(const NextHopThrift& nht);
NextHop nextHopFromFollyDynamic(const folly::dynamic& nhopJson);
}
}}
