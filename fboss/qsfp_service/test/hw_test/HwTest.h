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

#include <gtest/gtest.h>
#include "fboss/agent/types.h"

#include <folly/logging/xlog.h>

DECLARE_bool(setup_for_warmboot);

namespace facebook::fboss {

class HwQsfpEnsemble;
class MultiPimPlatformPimContainer;

class HwTest : public ::testing::Test {
 public:
  HwTest() = default;
  ~HwTest() override = default;

  HwQsfpEnsemble* getHwQsfpEnsemble() {
    return ensemble_.get();
  }

  MultiPimPlatformPimContainer* getPimContainer(int pimID);

  void SetUp() override;
  void TearDown() override;
  template <typename SETUP_FN, typename VERIFY_FN>
  void verifyAcrossWarmBoots(SETUP_FN setup, VERIFY_FN verify) {
    if (!didWarmBoot()) {
      XLOG(INFO) << "STAGE: cold boot setup()";
      setup();
    }

    XLOG(INFO) << " STAGE: verify";
    verify();
    if (FLAGS_setup_for_warmboot) {
      XLOG(INFO) << " STAGE: setupForWarmboot";
      setupForWarmboot();
    }
  }

  std::vector<TransceiverID> refreshTransceiversWithRetry(int numRetries = 3);

 private:
  bool didWarmBoot() const;
  void setupForWarmboot() const;
  // Forbidden copy constructor and assignment operator
  HwTest(HwTest const&) = delete;
  HwTest& operator=(HwTest const&) = delete;

  std::unique_ptr<HwQsfpEnsemble> ensemble_;
};
} // namespace facebook::fboss
