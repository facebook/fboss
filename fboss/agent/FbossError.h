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

#include <folly/Conv.h>
#include "fboss/agent/if/gen-cpp2/fboss_types.h"

namespace facebook { namespace fboss {

/**
 * A base class for FBOSS exceptions.
 *
 * This derives from the thrift exception class, so that any FbossErrors
 * thrown by C++ server code can passed directly to thrift callbacks.  However,
 * note that if you are catching exceptions from a thrift client,
 * thrift::FbossBaseErrors will be thrown rather than FbossError objects.
 *
 * This class primarily serves to provide a more convenient constructor
 * interface over the thrift exception type.
 */
class FbossError : public thrift::FbossBaseError {
 public:
  template<typename... Args>
  explicit FbossError(Args&&... args) {
    message = folly::to<std::string>(std::forward<Args>(args)...);
  }

  ~FbossError() throw() override {}
};

}} // facebook::fboss
