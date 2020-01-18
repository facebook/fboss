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

#include <folly/String.h>
#include <folly/experimental/StringKeyedUnorderedMap.h>

#include <map>
#include <string>

namespace facebook::fboss {

class BcmFacebookAPI {
 public:
  /*
   * A class for receiving log messages from the Broadcom SDK.
   */
  class LogListener {
   public:
    LogListener() {}
    virtual ~LogListener() {}

    virtual void vprintf(const char* fmt, va_list varg) = 0;
  };

  /*
   * Initialize the Broadcom Software Library to get all the default logs
   * to agent
   */
  static void initBSL();

  /*
   * Logging methods.
   *
   * These methods can be used to log messages and have it handled the same way
   * as log messages created from the Broadcom SDK code.
   *
   * If a log listener has been registered for this thread with
   * setLogListener(), data will be sent to that listener.  If there is no
   * listener registered, the data will be logged using glog.
   */
  static void printf(const char* fmt, ...);
  static void vprintf(const char* fmt, va_list varg);

  /*
   * Register a log listener for the current thread.
   *
   * All messages logged from the Broadcom SDK in this thread will be sent to
   * this LogListener object, until it is unregistered.  Only a single
   * LogListener may be registered at a time in each thread.
   *
   * This will not affect messages logged in other threads.
   *
   * This method will throw an exception if another listener has already been
   * registered for this thread.
   */
  static void registerLogListener(LogListener* listener);
  static void unregisterLogListener(LogListener* listener);
};

} // namespace facebook::fboss
