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
#include "fboss/agent/if/gen-cpp2/ctrl_types.h"
#include "fboss/agent/types.h"

namespace facebook::fboss {

std::string forwardActionStr(RouteForwardAction action);
RouteForwardAction str2ForwardAction(const std::string& action);

/**
 * Route prefix
 */
template <typename AddrT>
struct RoutePrefix {
  AddrT network;
  uint8_t mask;
  std::string str() const {
    return folly::to<std::string>(network, "/", static_cast<uint32_t>(mask));
  }

  folly::CIDRNetwork toCidrNetwork() const {
    return folly::CIDRNetwork{network.mask(mask), mask};
  }
  /*
   * Serialize to folly::dynamic
   */
  folly::dynamic toFollyDynamic() const;

  /*
   * Deserialize from folly::dynamic
   */
  static RoutePrefix fromFollyDynamic(const folly::dynamic& prefixJson);

  static RoutePrefix fromString(std::string str);

  bool operator<(const RoutePrefix&) const;
  bool operator>(const RoutePrefix&) const;
  bool operator==(const RoutePrefix& p2) const {
    return mask == p2.mask && network == p2.network;
  }
  bool operator!=(const RoutePrefix& p2) const {
    return !operator==(p2);
  }
  typedef AddrT AddressT;
};

struct Label {
  LabelID label;
  Label() : label(0) {}
  /* implicit */ Label(LabelID labelVal) : label(labelVal) {}
  std::string str() const {
    return folly::to<std::string>(label);
  }

  /*
   * Serialize to folly::dynamic
   */
  folly::dynamic toFollyDynamic() const;

  /*
   * Deserialize from folly::dynamic
   */
  static Label fromFollyDynamic(const folly::dynamic& prefixJson);

  static Label fromString(std::string str) {
    Label lbl;
    lbl.label = folly::to<int32_t>(str);
    return lbl;
  }

  bool operator<(const Label& p) const {
    return label < p.label;
  }
  bool operator>(const Label& p) const {
    return label > p.label;
  }
  bool operator==(const Label& p) const {
    return label == p.label;
  }
  bool operator!=(const Label& p) const {
    return !operator==(p);
  }
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
