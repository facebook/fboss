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
#include <folly/Format.h>
#include <folly/logging/xlog.h>
#include "fboss/agent/FbossError.h"

extern "C" {
  #include <sai.h>
}

namespace facebook {
namespace fboss {

class SaiApiError : public FbossError {
 public:
  template <typename... Args>
  SaiApiError(sai_status_t status, Args&&... args)
      : FbossError(std::forward<Args>(args)..., ": ", status),
        status_(status) {}
  sai_status_t getSaiStatus() const {
    return status_;
  }
  ~SaiApiError() throw() override {}

 private:
  sai_status_t status_;
};

template<typename... Args>
void saiCheckError(sai_status_t status, Args&&... args) {
  if (status != SAI_STATUS_SUCCESS) {
    throw SaiApiError(status, std::forward<Args>(args)...);
  }
}

template<typename... Args>
void saiLogError(sai_status_t status, Args&&... args) {
  if (status != SAI_STATUS_SUCCESS) {
    XLOG(ERR) << folly::to<std::string>(std::forward<Args>(args)...) << ": "
              << status;
  }
}

// TODO: saiLogFatal

} // namespace fboss
} // namespace facebook
