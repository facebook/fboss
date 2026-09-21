/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/Main.h"

#include <folly/logging/Init.h>

FOLLY_INIT_LOGGING_CONFIG("fboss=DBG2; default:async=true");

namespace facebook::fboss {

int fbossMain(
    int /*argc*/,
    char** /*argv*/,
    std::unique_ptr<AgentInitializer> initializer) {
  return initializer->initAgent();
}

} // namespace facebook::fboss
