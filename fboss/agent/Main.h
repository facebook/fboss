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
#include <memory>

#include <folly/experimental/FunctionScheduler.h>
#include <thrift/lib/cpp2/server/ThriftServer.h>
#include "fboss/agent/AgentInitializer.h"
#include "fboss/agent/CommonInit.h"
#include "fboss/agent/HwAgent.h"
#include "fboss/agent/SwSwitch.h"
#include "fboss/agent/TunManager.h"

#include <gflags/gflags.h>
#include <chrono>
#include <condition_variable>
#include <cstdio>
#include <functional>
#include <future>
#include <mutex>
#include <string>

namespace facebook::fboss {

void setVersionInfo();

int fbossMain(
    int argc,
    char** argv,
    std::unique_ptr<AgentInitializer> initializer);

} // namespace facebook::fboss
