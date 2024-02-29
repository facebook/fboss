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

#include <folly/IPAddress.h>
#include <folly/Poly.h>
#include <folly/Range.h>
#include <folly/json/dynamic.h>
#include <folly/poly/Regular.h>
#include <optional>

#include "fboss/agent/AddressUtil.h"
#include "fboss/agent/Constants.h"
#include "fboss/agent/if/gen-cpp2/ctrl_types.h"
#include "fboss/agent/state/LabelForwardingAction.h"
#include "fboss/agent/state/StateUtils.h"
#include "fboss/agent/types.h"

namespace facebook::fboss {

inline folly::StringPiece constexpr kInterface() {
  return "interface";
}

inline folly::StringPiece constexpr kNexthop() {
  return "nexthop";
}

inline folly::StringPiece constexpr kLabelForwardingAction() {
  return "label_forwarding_action";
}

using NextHopWeight = uint64_t;
constexpr NextHopWeight ECMP_WEIGHT = 0;
constexpr NextHopWeight UCMP_DEFAULT_WEIGHT = 1;

struct INextHop {
  // In this context "Interface" does not refer to network interfaces
  // but is rather an implementation detail of folly::Poly. This is
  // the "well-known" name you must use for your interface definition
  // for things like poly_call to work properly.
  template <class Base>
  struct Interface : Base {
    std::optional<InterfaceID> intfID() const {
      return folly::poly_call<0>(*this);
    }

    folly::IPAddress addr() const {
      return folly::poly_call<1>(*this);
    }

    NextHopWeight weight() const {
      return folly::poly_call<2>(*this);
    }

    std::optional<LabelForwardingAction> labelForwardingAction() const {
      return folly::poly_call<3>(*this);
    }

    std::optional<bool> disableTTLDecrement() const {
      return folly::poly_call<4>(*this);
    }

    bool isResolved() const {
      return intfID().has_value();
    }

    InterfaceID intf() const {
      return intfID().value();
    }

    bool isPopAndLookup() const {
      return labelForwardingAction().has_value() &&
          labelForwardingAction()->type() ==
          LabelForwardingAction::LabelForwardingType::POP_AND_LOOKUP;
    }

    folly::dynamic toFollyDynamic() const {
      folly::dynamic nh = folly::dynamic::object;
      nh[kNexthop()] = addr().str();
      nh[kWeight] = folly::to<std::string>(weight());
      if (isResolved()) {
        nh[kInterface()] = static_cast<uint32_t>(intf());
      }
      if (labelForwardingAction()) {
        nh[kLabelForwardingAction()] =
            labelForwardingAction()->toFollyDynamic();
      }
      return nh;
    }

    NextHopThrift toThrift() const {
      NextHopThrift nht;
      *nht.address() = network::toBinaryAddress(addr());
      *nht.weight() = weight();
      if (isResolved()) {
        nht.address()->ifName() = utility::createTunIntfName(intf());
      }
      if (labelForwardingAction()) {
        nht.mplsAction() = labelForwardingAction()->toThrift();
      }
      if (auto value = disableTTLDecrement()) {
        nht.disableTTLDecrement() = value.value();
      }
      return nht;
    }

    std::string str() const {
      std::string intfStr =
          isResolved() ? folly::to<std::string>("@I", intf()) : "";
      std::string labelActionStr{};
      if (auto action = labelForwardingAction()) {
        labelActionStr = folly::to<std::string>(" ", action->str());
      }
      return folly::to<std::string>(
          addr(), intfStr, "x", weight(), labelActionStr);
    }
  };

  template <class T>
  using Members = FOLLY_POLY_MEMBERS(
      &T::intfID,
      &T::addr,
      &T::weight,
      &T::labelForwardingAction,
      &T::disableTTLDecrement);
};

using NextHop = folly::Poly<INextHop>;

void toAppend(const NextHop& nhop, std::string* result);
std::ostream& operator<<(std::ostream& os, const NextHop& nhop);

bool operator<(const NextHop& a, const NextHop& b);
bool operator>(const NextHop& a, const NextHop& b);
bool operator<=(const NextHop& a, const NextHop& b);
bool operator>=(const NextHop& a, const NextHop& b);
bool operator==(const NextHop& a, const NextHop& b);
bool operator!=(const NextHop& a, const NextHop& b);

class ResolvedNextHop {
 public:
  ResolvedNextHop(
      const folly::IPAddress& addr,
      InterfaceID intfID,
      const NextHopWeight& weight,
      const std::optional<LabelForwardingAction>& action = std::nullopt,
      const std::optional<bool>& disableTTLDecrement = std::nullopt)
      : addr_(addr),
        intfID_(intfID),
        weight_(weight),
        labelForwardingAction_(action),
        disableTTLDecrement_(disableTTLDecrement) {}
  ResolvedNextHop(
      folly::IPAddress&& addr,
      InterfaceID intfID,
      const NextHopWeight& weight,
      std::optional<LabelForwardingAction>&& action = std::nullopt,
      std::optional<bool>&& disableTTLDecrement = std::nullopt)
      : addr_(std::move(addr)),
        intfID_(intfID),
        weight_(weight),
        labelForwardingAction_(std::move(action)),
        disableTTLDecrement_(std::move(disableTTLDecrement)) {}
  std::optional<InterfaceID> intfID() const {
    return intfID_;
  }
  folly::IPAddress addr() const {
    return addr_;
  }
  NextHopWeight weight() const {
    return weight_;
  }
  std::optional<LabelForwardingAction> labelForwardingAction() const {
    return labelForwardingAction_;
  }
  std::optional<bool> disableTTLDecrement() const {
    return disableTTLDecrement_;
  }

  void setDisableTTLDecrement(std::optional<bool> disableTTLDecrement) {
    disableTTLDecrement_ = disableTTLDecrement;
  }

 private:
  folly::IPAddress addr_;
  InterfaceID intfID_;
  NextHopWeight weight_;
  std::optional<LabelForwardingAction> labelForwardingAction_;
  std::optional<bool> disableTTLDecrement_{};
};

bool operator==(const ResolvedNextHop& a, const ResolvedNextHop& b);

class UnresolvedNextHop {
 public:
  UnresolvedNextHop(
      const folly::IPAddress& addr,
      const NextHopWeight& weight,
      const std::optional<LabelForwardingAction>& action = std::nullopt);
  UnresolvedNextHop(
      folly::IPAddress&& addr,
      const NextHopWeight& weight,
      std::optional<LabelForwardingAction>&& action = std::nullopt);
  std::optional<InterfaceID> intfID() const {
    return std::nullopt;
  }
  folly::IPAddress addr() const {
    return addr_;
  }
  NextHopWeight weight() const {
    return weight_;
  }
  std::optional<LabelForwardingAction> labelForwardingAction() const {
    return labelForwardingAction_;
  }
  std::optional<bool> disableTTLDecrement() const {
    return std::nullopt;
  }

 private:
  folly::IPAddress addr_;
  NextHopWeight weight_;
  std::optional<LabelForwardingAction> labelForwardingAction_;
};

bool operator==(const UnresolvedNextHop& a, const UnresolvedNextHop& b);

namespace util {
NextHop fromThrift(const NextHopThrift& nht, bool allowV6NonLinkLocal = false);
NextHop nextHopFromFollyDynamic(const folly::dynamic& nhopJson);
} // namespace util

} // namespace facebook::fboss
