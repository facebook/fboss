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

#include "fboss/agent/if/gen-cpp2/FbossHwCtrl.h"
#include "fboss/agent/if/gen-cpp2/TestCtrlAsyncClient.h"

namespace facebook::fboss::utility {

std::unique_ptr<apache::thrift::Client<facebook::fboss::TestCtrl>>
getSwAgentThriftClient(const std::string& switchName);
std::unique_ptr<apache::thrift::Client<FbossHwCtrl>> getHwAgentThriftClient(
    const std::string& switchName,
    int port);

MultiSwitchRunState getMultiSwitchRunState(const std::string& switchName);
int getNumHwSwitches(const std::string& switchName);

} // namespace facebook::fboss::utility
