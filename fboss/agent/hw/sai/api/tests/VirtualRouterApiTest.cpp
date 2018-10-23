/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/hw/sai/api/VirtualRouterApi.h"
#include "fboss/agent/hw/sai/fake/FakeSai.h"

#include <folly/logging/xlog.h>

#include <gtest/gtest.h>

using namespace facebook::fboss;

class VirtualRouterApiTest : public ::testing::Test {
 public:
  void SetUp() override {
    fs = FakeSai::getInstance();
    sai_api_initialize(0, nullptr);
    virtualRouterApi = std::make_unique<VirtualRouterApi>();
  }
  std::shared_ptr<FakeSai> fs;
  std::unique_ptr<VirtualRouterApi> virtualRouterApi;
};

TEST_F(VirtualRouterApiTest, createVirtualRouter) {
  virtualRouterApi->create({}, 0);
}
