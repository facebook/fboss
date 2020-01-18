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

#include <folly/io/IOBuf.h>
#include "fboss/agent/hw/bcm/BcmFacebookAPI.h"

namespace facebook::fboss {

class BcmLogBuffer : public BcmFacebookAPI::LogListener {
 public:
  explicit BcmLogBuffer(size_t initialSize = 1024);
  ~BcmLogBuffer() override;

  void vprintf(const char* fmt, va_list varg) override;

  folly::IOBuf* getBuffer() {
    return &buf_;
  }

 private:
  folly::IOBuf buf_;
};

} // namespace facebook::fboss
