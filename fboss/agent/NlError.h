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

#include "fboss/agent/FbossError.h"

extern "C" {
#include <netlink/errno.h>
}

namespace facebook::fboss {

/**
 * A class for errors from libnl
 */

class NlError : public FbossError {
 public:
  template <typename... Args>
  NlError(int nlerr, Args&&... args)
      : FbossError(std::forward<Args>(args)..., ": ", nl_geterror(nlerr)),
        nlerr_(nlerr) {}

  ~NlError() throw() override {}

  int getNlError() const {
    return nlerr_;
  }

 private:
  int nlerr_;
};

template <typename... Args>
void nlCheckError(int ret, Args&&... msgArgs) {
  if (ret < 0) {
    throw NlError(ret, std::forward<Args>(msgArgs)...);
  }
}

} // namespace facebook::fboss
