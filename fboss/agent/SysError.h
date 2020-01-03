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

#include <folly/logging/xlog.h>
#include "fboss/agent/FbossError.h"

extern "C" {
#include <errno.h>
#include <string.h>
}

namespace facebook::fboss {

/**
 * A class for errors from the Broadcom SDK.
 */
class SysError : public FbossError {
 public:
  template <typename... Args>
  SysError(int err, Args&&... args)
      : FbossError(
            std::forward<Args>(args)...,
            ": ",
            strerror_r(err, buf_, sizeof(buf_))),
        err_(err) {}

  ~SysError() throw() override {}

  int getSysError() const {
    return err_;
  }

 private:
  int err_;
  char buf_[256];
};

template <typename... Args>
void sysCheckError(int ret, Args&&... msgArgs) {
  if (ret < 0) {
    auto err = errno;
    throw SysError(err, std::forward<Args>(msgArgs)...);
  }
}

template <typename... Args>
void sysLogError(int ret, Args&&... msgArgs) {
  if (ret < 0) {
    auto err = errno;
    char buf[256];
    XLOG(ERR) << folly::to<std::string>(std::forward<Args>(msgArgs)...) << ": "
              << strerror_r(err, buf, sizeof(buf));
  }
}

template <typename... Args>
void sysLogFatal(int ret, Args&&... msgArgs) {
  if (ret < 0) {
    auto err = errno;
    char buf[256];
    XLOG(FATAL) << folly::to<std::string>(std::forward<Args>(msgArgs)...)
                << ": " << strerror_r(err, buf, sizeof(buf));
  }
}

} // namespace facebook::fboss
