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

#include <folly/io/async/EventBase.h>
#include "fboss/agent/hw/sai/switch/gen-cpp2/SaiCtrlAsyncClient.h"

#include <thread>
#include <vector>

DECLARE_bool(help);

namespace utility {
using folly::IPAddress;

std::unique_ptr<facebook::fboss::SaiCtrlAsyncClient>
getStreamingClient(folly::EventBase* evb, const IPAddress& ip, uint32_t port);

} // namespace utility
