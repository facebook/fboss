/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/platform/misc_service/hw_test/MiscServiceTest.h"

#include <folly/experimental/coro/BlockingWait.h>
#include <thrift/lib/cpp2/async/RocketClientChannel.h>
#include "thrift/lib/cpp2/server/ThriftServer.h"

#include "common/services/cpp/ServiceFrameworkLight.h"
#include "fboss/platform/misc_service/MiscServiceImpl.h"
#include "fboss/platform/misc_service/MiscServiceThriftHandler.h"
#include "fboss/platform/misc_service/SetupMiscServiceThrift.h"

namespace facebook::fboss::platform::misc_service {

MiscServiceTest::~MiscServiceTest() {}

void MiscServiceTest::SetUp() {
  std::tie(thriftServer_, thriftHandler_) = setupThrift();
  service_ =
      std::make_unique<services::ServiceFrameworkLight>("Misc service test");
  runServer(
      *service_, thriftServer_, thriftHandler_.get(), false /*loop forever*/);
}
void MiscServiceTest::TearDown() {
  service_->stop();
  service_->waitForStop();
  service_.reset();
  thriftServer_.reset();
  thriftHandler_.reset();
}

MiscServiceImpl* MiscServiceTest::getService() {
  return thriftHandler_->getServiceImpl();
}

TEST_F(MiscServiceTest, testThrift) {
  folly::SocketAddress addr("::1", 5971);
  auto socket = folly::AsyncSocket::newSocket(
      folly::EventBaseManager::get()->getEventBase(), addr, 5000);
  auto channel =
      apache::thrift::RocketClientChannel::newChannel(std::move(socket));
  auto client = MiscServiceThriftAsyncClient(std::move(channel));
  MiscFruidReadResponse response;
  client.sync_getFruid(response, false);
  EXPECT_GT(response.fruidData_ref()->size(), 0);
}
} // namespace facebook::fboss::platform::misc_service
