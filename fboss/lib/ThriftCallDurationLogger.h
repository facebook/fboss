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

#include <chrono>

namespace facebook::fboss {

/**
 * Interface for recording thrift call completion time.
 *
 * This decouples LogThriftCall (in fboss/lib) from SwitchStats (in
 * fboss/agent), allowing services that don't need agent stats (FSDB,
 * QSFP service, policydb, etc.) to use LogThriftCall without pulling
 * in the agent dependency graph.
 */
class ThriftCallDurationLogger {
 public:
  virtual ~ThriftCallDurationLogger() = default;
  virtual void thriftRequestCompletionTimeMs(std::chrono::milliseconds ms) = 0;
};

} // namespace facebook::fboss
