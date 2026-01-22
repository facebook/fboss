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
#include <thrift/lib/cpp2/server/ThriftServer.h>

namespace facebook::fboss {

/**
 * Utility class for Thrift Service server and client configuration
 */
class ThriftServiceUtils {
 public:
  /**
   * Set ThriftServer to use EpollBackend for event loop backend
   * Current ThriftServer doesn't have `setPreferEpoll()` like existing
   * `setPreferIoUring()`. This helper function can help FBOSS services to
   * switch to use EpollBackend to achieve a higher performance.
   * @param server ThriftServer to set EpollBackend for event loop backend
   */
  static void setPreferEpoll(apache::thrift::ThriftServer& server);
};
} // namespace facebook::fboss
