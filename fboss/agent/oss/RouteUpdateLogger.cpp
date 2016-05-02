/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/RouteUpdateLogger.h"

#include <folly/IPAddressV4.h>
#include <folly/IPAddressV6.h>


#include <memory>

namespace {
template<typename AddrT>
std::unique_ptr<facebook::fboss::RouteLogger<AddrT>> getRouteLogger() {
  return std::make_unique<facebook::fboss::GlogRouteLogger<AddrT>>();
}
}

namespace facebook { namespace fboss {

std::unique_ptr<RouteLogger<folly::IPAddressV4>>
RouteUpdateLogger::getDefaultV4RouteLogger() {
  return getRouteLogger<folly::IPAddressV4>();
}

std::unique_ptr<RouteLogger<folly::IPAddressV6>>
RouteUpdateLogger::getDefaultV6RouteLogger() {
  return getRouteLogger<folly::IPAddressV6>();
}

}}
