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

#include "fboss/platform/misc_service/MiscServiceImpl.h"
#include "fboss/platform/misc_service/if/gen-cpp2/MiscServiceThrift.h"

namespace facebook::services {
class ServiceFrameworkLight;
}
namespace apache::thrift {
class ThriftServer;
}

namespace facebook::fboss::platform::misc_service {

class MiscServiceThriftHandler;

class MiscServiceTest : public ::testing::Test {
 public:
  ~MiscServiceTest() override;
  void SetUp() override;
  void TearDown() override;

 protected:
  MiscServiceImpl* getService();
  std::unique_ptr<services::ServiceFrameworkLight> service_;
  std::shared_ptr<apache::thrift::ThriftServer> thriftServer_;
  std::shared_ptr<MiscServiceThriftHandler> thriftHandler_;
};
} // namespace facebook::fboss::platform::misc_service
