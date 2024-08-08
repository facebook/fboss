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
#include <folly/logging/xlog.h>

namespace facebook::fboss {

class FbossEventBase : public folly::EventBase {
 public:
  void runInFbossEventBaseThread(Func fn) noexcept {
    runInEventBaseThread(std::move(fn));
    if (!isRunning()) {
      XLOG(ERR) << "Cannot enqueue callback to non-running FbossEventBase.";
    }
  }

  template <typename T>
  void runInFbossEventBaseThread(void (*fn)(T*), T* arg) noexcept {
    return runInFbossEventBaseThread([=] { fn(arg); });
  }

  void runInFbossEventBaseThreadAndWait(Func fn) noexcept {
    runInEventBaseThreadAndWait(std::move(fn));
    if (!isRunning()) {
      XLOG(ERR) << "Cannot enqueue callback to non-running FbossEventBase.";
    }
  }

  void runImmediatelyOrRunInFbossEventBaseThreadAndWait(Func fn) noexcept {
    runImmediatelyOrRunInEventBaseThreadAndWait(std::move(fn));
    if (!isRunning()) {
      XLOG(ERR) << "Cannot enqueue callback to non-running FbossEventBase.";
    }
  }
};

} // namespace facebook::fboss
