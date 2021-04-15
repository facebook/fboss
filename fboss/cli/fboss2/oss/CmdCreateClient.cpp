/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/cli/fboss2/CmdCreateClient.h"

namespace facebook::fboss {

template <typename T>
std::unique_ptr<T> CmdCreateClient::createAdditional(
    const std::string& /*ip*/,
    folly::EventBase& /*evb*/) {
  return nullptr;
}

} // namespace facebook::fboss
