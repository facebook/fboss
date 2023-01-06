/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/platform/data_corral_service/hw_test/DataCorralServiceTest.h"

#include <folly/experimental/coro/BlockingWait.h>
#include <thrift/lib/cpp2/async/RocketClientChannel.h>
#include <thrift/lib/cpp2/server/ThriftServer.h>

#include "common/services/cpp/ServiceFrameworkLight.h"
#include "fboss/platform/data_corral_service/DataCorralServiceImpl.h"
#include "fboss/platform/data_corral_service/DataCorralServiceThriftHandler.h"
#include "fboss/platform/data_corral_service/Flags.h"
#include "fboss/platform/data_corral_service/SetupDataCorralServiceThrift.h"

namespace facebook::fboss::platform::data_corral_service {

DataCorralServiceTest::~DataCorralServiceTest() {}

void DataCorralServiceTest::SetUp() {
  std::tie(thriftServer_, thriftHandler_) = setupThrift();
  service_ = std::make_unique<services::ServiceFrameworkLight>(
      "Data corral service test");
  runServer(
      *service_, thriftServer_, thriftHandler_.get(), false /*loop forever*/);
}
void DataCorralServiceTest::TearDown() {
  service_->stop();
  service_->waitForStop();
  service_.reset();
  thriftServer_.reset();
  thriftHandler_.reset();
}

DataCorralFruidReadResponse DataCorralServiceTest::getFruid(bool uncached) {
  return *(folly::coro::blockingWait(thriftHandler_->co_getFruid(uncached)));
}

DataCorralServiceImpl* DataCorralServiceTest::getService() {
  return thriftHandler_->getServiceImpl();
}

TEST_F(DataCorralServiceTest, getCachedFruid) {
  EXPECT_GT(getFruid(false).fruidData()->size(), 0);
}

TEST_F(DataCorralServiceTest, getUncachedFruid) {
  EXPECT_GT(getFruid(true).fruidData()->size(), 0);
}

TEST_F(DataCorralServiceTest, testThrift) {
  folly::SocketAddress addr("::1", FLAGS_thrift_port);
  auto socket = folly::AsyncSocket::newSocket(
      folly::EventBaseManager::get()->getEventBase(), addr, 5000);
  auto channel =
      apache::thrift::RocketClientChannel::newChannel(std::move(socket));
  auto client = DataCorralServiceThriftAsyncClient(std::move(channel));
  DataCorralFruidReadResponse response;
  client.sync_getFruid(response, false);
  EXPECT_GT(response.fruidData()->size(), 0);
}
} // namespace facebook::fboss::platform::data_corral_service
