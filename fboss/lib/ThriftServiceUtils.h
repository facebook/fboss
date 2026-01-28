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

#include <thrift/lib/cpp2/server/ThriftServer.h>

namespace facebook::fboss {

/**
 * Utility class for Thrift Service server and client configuration
 */
class ThriftServiceUtils {
 public:
  /**
   * Set preferred EventBase Backend for ThriftServer
   */
  static void setPreferredEventBaseBackend(
      apache::thrift::ThriftServer& server);

  static void setPreferredEventBaseBackendToEpoll(
      apache::thrift::ThriftServer& server);
};
} // namespace facebook::fboss
