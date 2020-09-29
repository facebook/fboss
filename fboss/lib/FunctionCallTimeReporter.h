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

#include <memory>

#include <folly/ScopeGuard.h>
#include <folly/Singleton.h>

#include <chrono>

namespace facebook::fboss {

class FunctionCallTimeReporter {
 public:
  static std::shared_ptr<FunctionCallTimeReporter> getInstance();
  FunctionCallTimeReporter(const FunctionCallTimeReporter&) = delete;
  FunctionCallTimeReporter& operator=(const FunctionCallTimeReporter&) = delete;
  FunctionCallTimeReporter() = default;
  ~FunctionCallTimeReporter() = default;
  void start();
  void end();
  void callStart() {
    if (UNLIKELY(isOn_)) {
      tracker_.callStart();
    }
  }
  void callEnd() {
    if (UNLIKELY(isOn_)) {
      tracker_.callEnd();
    }
  }

 private:
  struct CallTimeTracker {
    ~CallTimeTracker();
    void callStart() {
      startTime_ = std::chrono::steady_clock::now();
    }
    void callEnd() {
      std::chrono::duration<double, std::micro> callUsecs =
          std::chrono::steady_clock::now() - startTime_;
      cumalativeUsecs_ += callUsecs;
    }

    std::chrono::time_point<std::chrono::steady_clock> startTime_;
    std::chrono::duration<double, std::micro> cumalativeUsecs_{0};
  };

  std::chrono::time_point<std::chrono::steady_clock> startTime_;
  /*
   * Use a bool to track on/off. Another option was to wrap startTime in
   * a std::optional and use that to infer whether time reporting was
   * turned on or not. However that would preclude use of std::atomic
   * and would need heavier means of synchronization
   */
  std::atomic<bool> isOn_{false};
  static thread_local CallTimeTracker tracker_;
};

#define TIME_CALL                                       \
  FunctionCallTimeReporter::getInstance()->callStart(); \
  SCOPE_EXIT {                                          \
    FunctionCallTimeReporter::getInstance()->callEnd(); \
  };

class ScopedCallTimer {
 public:
  ScopedCallTimer();
  ~ScopedCallTimer();
  ScopedCallTimer(const ScopedCallTimer&) = delete;
  ScopedCallTimer& operator=(const ScopedCallTimer&) = delete;
};
} // namespace facebook::fboss
