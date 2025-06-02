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

#include <string>
#include "fboss/agent/if/gen-cpp2/FbossCtrlAsyncClient.h"

namespace facebook::fboss::utility {
std::unique_ptr<apache::thrift::Client<FbossCtrl>> getSwAgentThriftClient(
    const std::string& switchName);

class MultiNodeUtil {
 public:
  MultiNodeUtil();
};

} // namespace facebook::fboss::utility
