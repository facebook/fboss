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

#include <thrift/lib/cpp2/server/Cpp2ConnContext.h>
#include <string>

namespace facebook {
namespace fboss {

class LogThriftCall {
 public:
  explicit LogThriftCall(
      const std::string& func,
      apache::thrift::Cpp2RequestContext* ctx);
  ~LogThriftCall();

 private:
  const std::string func_;
  const std::chrono::time_point<std::chrono::steady_clock> start_;
};

} // namespace fboss
} // namespace facebook
