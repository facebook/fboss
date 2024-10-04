/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/lib/LogThriftCall.h"

namespace facebook::fboss {

std::optional<std::string> LogThriftCall::getIdentityFromConnContext(
    apache::thrift::Cpp2ConnContext* ctx) const {
  if (ctx) {
    // note that getPeerCommonName is not 100% reliable with certain
    // protocols that may cache certificates at the transport layer.
    auto identity = ctx->getPeerCommonName();
    if (!identity.empty()) {
      return identity;
    }
  }
  return std::nullopt;
}

} // namespace facebook::fboss
