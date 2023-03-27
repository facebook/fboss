/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/hw/bcm/BcmFacebookAPI.h"

#include "fboss/agent/hw/bcm/BcmAPI.h"
#include "fboss/agent/hw/bcm/BcmError.h"

#include <folly/String.h>
#include <folly/experimental/StringKeyedUnorderedMap.h>
#include <folly/logging/xlog.h>
#include <glog/logging.h>

extern "C" {
#include <shared/bslext.h>
} // extern "C"

using folly::StringPiece;
using std::string;

namespace facebook::fboss {

void BcmFacebookAPI::printf(const char* fmt, ...) {
  va_list varg;
  va_start(varg, fmt);
  SCOPE_EXIT {
    va_end(varg);
  };
  BcmFacebookAPI::vprintf(fmt, varg);
}

thread_local static BcmFacebookAPI::LogListener* bcmLogListener{nullptr};

void BcmFacebookAPI::vprintf(const char* fmt, va_list varg) {
  try {
    if (bcmLogListener) {
      bcmLogListener->vprintf(fmt, varg);
      return;
    }

    // No-one is listening for this message, so log it ourselves.
    string msg = folly::stringVPrintf(fmt, varg);
    XLOG(DBG2) << "[BCM " << BcmAPI::getThreadName() << "] " << msg;
  } catch (const std::exception& ex) {
    // vprintf() is invoked by C functions, so ensure that we don't
    // throw C++ exceptions up to pure C code.
    XLOG(ERR) << "exception thrown while recording BCM log message: "
              << ex.what();
  }
}

void BcmFacebookAPI::registerLogListener(LogListener* listener) {
  if (bcmLogListener) {
    throw FbossError(
        "cannot register multiple log listeners in a "
        "single thread");
  }
  CHECK(listener != nullptr);
  bcmLogListener = listener;
}

void BcmFacebookAPI::unregisterLogListener(LogListener* listener) {
  CHECK_EQ(bcmLogListener, listener);
  bcmLogListener = nullptr;
}

} // namespace facebook::fboss

using facebook::fboss::BcmFacebookAPI;

/*
 * Override various SAL functions with our own custom behaviors.
 *
 * We define these as weak symbols in the SDK library.
 */
extern "C" int sal_vprintf(const char* fmt, va_list varg) {
  BcmFacebookAPI::vprintf(fmt, varg);
  return 0;
}

extern "C" void sal_reboot(void) {
  BcmFacebookAPI::printf("%s", "reboot is not permitted");
}

extern "C" void sal_shell(void) {
  BcmFacebookAPI::printf("%s", "shell access is not supported");
}
