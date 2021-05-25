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

 private:
  // Forbidden copy constructor and assignment operator
  HwTest(HwTest const&) = delete;
  HwTest& operator=(HwTest const&) = delete;

  std::unique_ptr<HwQsfpEnsemble> ensemble_;
};
} // namespace facebook::fboss
