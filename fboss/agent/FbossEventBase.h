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

#include "fboss/agent/AgentFeatures.h"

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
    if (getNotificationQueueSize() >= FLAGS_fboss_event_base_queue_limit) {
      XLOG(ERR) << "Stop enqueuing to " << eventBaseName_
                << ". Queue size greater than limit "
                << FLAGS_fboss_event_base_queue_limit;
      return;
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
    if (getNotificationQueueSize() >= FLAGS_fboss_event_base_queue_limit) {
      XLOG(ERR) << "Stop enqueuing to " << eventBaseName_
                << ". Queue size greater than limit "
                << FLAGS_fboss_event_base_queue_limit;
      return;
    }
    runInEventBaseThreadAndWait(std::move(fn));
  }

  void runImmediatelyOrRunInFbossEventBaseThreadAndWait(Func fn) noexcept {
    if (!isRunning()) {
      XLOG(ERR)
          << "runImmediatelyOrRunInFbossEventBaseThreadAndWait for non-running "
          << eventBaseName_ << " FbossEventBase.";
    }
    if (getNotificationQueueSize() >= FLAGS_fboss_event_base_queue_limit) {
      XLOG(ERR) << "Stop enqueuing to " << eventBaseName_
                << ". Queue size greater than limit "
                << FLAGS_fboss_event_base_queue_limit;
      return;
    }
    runImmediatelyOrRunInEventBaseThreadAndWait(std::move(fn));
  }

 private:
  std::string eventBaseName_;
};

} // namespace facebook::fboss
