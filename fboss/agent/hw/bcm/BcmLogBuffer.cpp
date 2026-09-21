/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/hw/bcm/BcmLogBuffer.h"

#include <folly/io/Cursor.h>

using folly::IOBuf;
using folly::io::Appender;

namespace facebook::fboss {

BcmLogBuffer::BcmLogBuffer(size_t initialSize)
    : buf_(IOBuf::CREATE, initialSize) {
  BcmFacebookAPI::registerLogListener(this);
}

BcmLogBuffer::~BcmLogBuffer() {
  BcmFacebookAPI::unregisterLogListener(this);
}

void BcmLogBuffer::vprintf(const char* fmt, va_list varg) {
  Appender a(&buf_, 1024);
  a.vprintf(fmt, varg);
}

} // namespace facebook::fboss
