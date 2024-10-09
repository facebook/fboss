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
#include <string>

namespace facebook::fboss {

class FbossEventBase : public folly::EventBase {
 public:
  explicit FbossEventBase(const std::string& name) : eventBaseName_(name) {}

  void runInFbossEventBaseThread(Func fn) noexcept {
    if (!isRunning()) {
      XLOG(ERR) << "runInFbossEventBaseThread to non-running " << eventBaseName_
                << " FbossEventBase.";
    }
    runInEventBaseThread(std::move(fn));
  }

  template <typename T>
  void runInFbossEventBaseThread(void (*fn)(T*), T* arg) noexcept {
    return runInFbossEventBaseThread([=] { fn(arg); });
  }

  void runInFbossEventBaseThreadAndWait(Func fn) noexcept {
    if (!isRunning()) {
      XLOG(ERR) << "runInFbossEventBaseThreadAndWait for non-running "
                << eventBaseName_ << " FbossEventBase.";
    }
    runInEventBaseThreadAndWait(std::move(fn));
  }

  void runImmediatelyOrRunInFbossEventBaseThreadAndWait(Func fn) noexcept {
    if (!isRunning()) {
      XLOG(ERR)
          << "runImmediatelyOrRunInFbossEventBaseThreadAndWait for non-running "
          << eventBaseName_ << " FbossEventBase.";
    }
    runImmediatelyOrRunInEventBaseThreadAndWait(std::move(fn));
  }

 private:
  std::string eventBaseName_;
};

} // namespace facebook::fboss
