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

#include "fboss/platform/data_corral_service/DataCorralServiceImpl.h"
#include "fboss/platform/data_corral_service/if/gen-cpp2/DataCorralServiceThrift.h"

namespace facebook::services {
class ServiceFrameworkLight;
}
namespace apache::thrift {
class ThriftServer;
}

namespace facebook::fboss::platform::data_corral_service {

class DataCorralServiceThriftHandler;

class DataCorralServiceTest : public ::testing::Test {
 public:
  ~DataCorralServiceTest() override;
  void SetUp() override;
  void TearDown() override;

 protected:
  DataCorralFruidReadResponse getFruid(bool uncached);
  DataCorralServiceImpl* getService();
  std::unique_ptr<services::ServiceFrameworkLight> service_;
  std::shared_ptr<apache::thrift::ThriftServer> thriftServer_;
  std::shared_ptr<DataCorralServiceThriftHandler> thriftHandler_;
};
} // namespace facebook::fboss::platform::data_corral_service
