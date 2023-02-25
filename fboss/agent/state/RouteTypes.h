/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
// Copyright 2004-present Facebook.  All rights reserved.
#pragma once

#include <folly/FBString.h>
#include <folly/IPAddress.h>
#include <folly/dynamic.h>
#include "fboss/agent/AddressUtil.h"
#include "fboss/agent/gen-cpp2/switch_state_types.h"
#include "fboss/agent/if/gen-cpp2/ctrl_types.h"
#include "fboss/agent/state/Thrifty.h"
#include "fboss/agent/types.h"
#include "folly/IPAddressV4.h"
#include "folly/IPAddressV6.h"

namespace facebook::fboss {

std::string forwardActionStr(RouteForwardAction action);
RouteForwardAction str2ForwardAction(const std::string& action);

/**
 * Route prefix
 */
template <typename AddrT>
struct RoutePrefix {
  RoutePrefix() {}

  RoutePrefix(AddrT addr, uint8_t u8) {
    auto& data = this->writableData();
    auto ip = folly::IPAddress(addr);
    data.prefix() = network::toBinaryAddress(ip);
    data.mask() = u8;
    if constexpr (std::is_same_v<folly::IPAddress, AddrT>) {
      data.v6() = addr.isV6();
    } else {
      static_assert(
          std::is_same_v<folly::IPAddressV6, AddrT> ||
              std::is_same_v<folly::IPAddressV4, AddrT>,
          "Address is not V4 or V6");
      data.v6() = std::is_same_v<folly::IPAddressV6, AddrT>;
    }
  }

  explicit RoutePrefix(const state::RoutePrefix& prefix) {
    auto& data = this->writableData();
    data = prefix;
  }

  std::string str() const {
    return folly::to<std::string>(
        network(), "/", static_cast<uint32_t>(mask()));
  }

  folly::CIDRNetwork toCidrNetwork() const {
    auto maskVal = mask();
    auto networkVal = network();
    return folly::CIDRNetwork{networkVal.mask(maskVal), maskVal};
  }

  state::RoutePrefix toThrift() const;
  static RoutePrefix fromThrift(const state::RoutePrefix& prefix);

  /*
   * Serialize to folly::dynamic
   */
  folly::dynamic toFollyDynamicLegacy() const;

  /*
   * Deserialize from folly::dynamic
   */
  static RoutePrefix fromFollyDynamicLegacy(const folly::dynamic& prefixJson);

  static RoutePrefix fromString(std::string str);

  bool operator<(const RoutePrefix&) const;
  bool operator>(const RoutePrefix&) const;
  bool operator==(const RoutePrefix& p2) const {
    return mask() == p2.mask() && network() == p2.network();
  }
  bool operator!=(const RoutePrefix& p2) const {
    return !operator==(p2);
  }
  typedef AddrT AddressT;

  inline AddrT network() const {
    auto ip = network::toIPAddress(*(this->data().prefix()));
    if constexpr (std::is_same_v<folly::IPAddressV4, AddrT>) {
      return ip.asV4();
    } else if constexpr (std::is_same_v<folly::IPAddressV6, AddrT>) {
      return ip.asV6();
    } else {
      static_assert(
          std::is_same_v<folly::IPAddress, AddrT>, "AddrT is not IPAddress");
      return ip;
    }
  }
  inline uint8_t mask() const {
    return *(this->data()).mask();
  }

  const state::RoutePrefix& data() const {
    return data_;
  }

 private:
  state::RoutePrefix& writableData() {
    return data_;
  }
  state::RoutePrefix data_;
};

template <>
struct is_fboss_key_object_type<RoutePrefix<folly::IPAddressV4>> {
  static constexpr bool value = true;
};

template <>
struct is_fboss_key_object_type<RoutePrefix<folly::IPAddressV6>> {
  static constexpr bool value = true;
};

struct Label {
  Label() : Label(Label::getLabelThrift(0)) {}
  /* implicit */ Label(LabelID labelVal)
      : Label(Label::getLabelThrift(labelVal)) {}
  /* implicit */ Label(MplsLabel labelVal)
      : Label(Label::getLabelThrift(labelVal)) {}
  /* implicit */ Label(state::Label labelVal) {
    writableData() = labelVal;
  }
  std::string str() const {
    return folly::to<std::string>(value());
  }

  LabelID value() const {
    return static_cast<LabelID>(*data().value());
  }

  LabelID label() const {
    return value();
  }

  state::Label toThrift() const {
    return data();
  }

  static Label fromThrift(const state::Label& thriftLabel) {
    return *thriftLabel.value();
  }

  static folly::dynamic migrateToThrifty(folly::dynamic const& dyn);
  static void migrateFromThrifty(folly::dynamic& dyn);

  /*
   * Serialize to folly::dynamic
   */
  folly::dynamic toFollyDynamicLegacy() const;

  /*
   * Deserialize from folly::dynamic
   */

  static Label fromFollyDynamicLegacy(const folly::dynamic& prefixJson);

  static Label fromString(std::string str) {
    return Label(getLabelThrift(folly::to<uint32_t>(str)));
  }

  bool operator<(const Label& p) const {
    return value() < p.value();
  }
  bool operator>(const Label& p) const {
    return value() > p.value();
  }
  bool operator==(const Label& p) const {
    return value() == p.value();
  }
  bool operator!=(const Label& p) const {
    return !operator==(p);
  }

 private:
  static state::Label getLabelThrift(uint32_t label) {
    state::Label thriftLabel{};
    thriftLabel.value() = label;
    return thriftLabel;
  }

  state::Label& writableData() {
    return data_;
  }

  const state::Label& data() const {
    return data_;
  }

  state::Label data_;
};

template <>
struct is_fboss_key_object_type<Label> {
  static constexpr bool value = true;
};

typedef RoutePrefix<folly::IPAddressV4> RoutePrefixV4;
typedef RoutePrefix<folly::IPAddressV6> RoutePrefixV6;
using RouteKeyMpls = Label;

void toAppend(const RoutePrefixV4& prefix, std::string* result);
void toAppend(const RoutePrefixV6& prefix, std::string* result);
void toAppend(const RouteKeyMpls& route, std::string* result);
void toAppend(const RouteForwardAction& action, std::string* result);
std::ostream& operator<<(std::ostream& os, const RouteForwardAction& action);

} // namespace facebook::fboss
