/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/Utils.h"

#include <folly/system/ThreadName.h>

namespace facebook::fboss {

void initThread(folly::StringPiece name) {
  folly::setThreadName(name);
}

std::string getLocalHostnameUqdn() {
  // TODO: actually implement
  return getLocalHostname();
}

} // namespace facebook::fboss
