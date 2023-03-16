/*
 *  Copyright (c) 2004-present, Meta Platforms, Inc. and affiliates.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#pragma once

#include <gtest/gtest.h>
#include <thread>

#include <thrift/lib/cpp2/util/ScopedServerInterfaceThread.h>
#include "fboss/platform/rackmon/Rackmon.h"
#include "fboss/platform/rackmon/RackmonPlsManager.h"
#include "fboss/platform/rackmon/RackmonThriftHandler.h"
#include "fboss/platform/rackmon/if/gen-cpp2/RackmonCtrl.h"

namespace facebook::services {
class ServiceFrameworkLight;
}
namespace apache::thrift {
class ThriftServer;
}
namespace rackmonsvc {

class RackmonThriftService {
  std::shared_ptr<ThriftHandler> thriftHandler_{};
  std::unique_ptr<apache::thrift::ScopedServerInterfaceThread> server_{};

 public:
  RackmonThriftService();
};

class RackmonHwTest : public ::testing::Test {
 public:
  ~RackmonHwTest() override {}

  void SetUp() override;

 protected:
  std::unique_ptr<apache::thrift::Client<RackmonCtrl>> client_;
};
} // namespace rackmonsvc
