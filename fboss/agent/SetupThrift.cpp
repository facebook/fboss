/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/SetupThrift.h"

#include <folly/io/async/EventBase.h>
#include <gflags/gflags.h>
#include <thrift/lib/cpp2/server/ThriftServer.h>

DEFINE_int32(thrift_idle_timeout, 60, "Thrift idle timeout in seconds.");
// Programming 16K routes can take 20+ seconds
DEFINE_int32(
    thrift_task_expire_timeout,
    30,
    "Thrift task expire timeout in seconds.");
