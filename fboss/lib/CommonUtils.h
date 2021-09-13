// (c) Facebook, Inc. and its affiliates. Confidential and proprietary.

#pragma once

#include "fboss/agent/FbossError.h"

namespace facebook::fboss {

template <typename CONDITION_FN>
void checkWithRetry(
    CONDITION_FN condition,
    int retries = 10,
    std::chrono::duration<uint32_t, std::milli> msBetweenRetry =
        std::chrono::milliseconds(1000)) {
  while (retries--) {
    if (condition()) {
      return;
    }
    std::this_thread::sleep_for(msBetweenRetry);
  }
  throw FbossError("Verify with retry failed, condition was never satisfied");
}

} // namespace facebook::fboss
