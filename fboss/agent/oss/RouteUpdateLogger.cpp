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

namespace facebook::fboss {

std::unique_ptr<UpdateLogHandler> UpdateLogHandler::get() {
  return std::make_unique<GlogUpdateLogHandler>();
}

template class RouteLogger<folly::IPAddressV4>;
template class RouteLogger<folly::IPAddressV6>;

} // namespace facebook::fboss
