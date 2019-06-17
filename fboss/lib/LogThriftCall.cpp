/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/lib/LogThriftCall.h"
#include <folly/logging/xlog.h>

using apache::thrift::Cpp2ConnContext;
using apache::thrift::Cpp2RequestContext;

namespace facebook {
namespace fboss {

LogThriftCall::LogThriftCall(const std::string& func, Cpp2RequestContext* ctx)
    : func_(func), start_(std::chrono::steady_clock::now()) {
  if (!ctx) {
    return;
  }
  Cpp2ConnContext* ctx2 = ctx->getConnectionContext();
  auto client = ctx2->getPeerAddress()->getHostStr();
  auto identity = ctx2->getPeerCommonName();
  if (identity.empty()) {
    identity = "unknown";
  }
  // this specific format is consumed by systemd-journald/rsyslogd
  XLOG(INFO) << func << " thrift request received from " << client << " ("
             << identity << ")";
}

LogThriftCall::~LogThriftCall() {
  auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
      std::chrono::steady_clock::now() - start_);
  XLOG(INFO) << func_ << " thrift request succeeded in " << ms.count() << "ms";
}

} // namespace fboss
} // namespace facebook
