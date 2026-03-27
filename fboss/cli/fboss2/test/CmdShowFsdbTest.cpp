// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include <gtest/gtest.h>

#include "fboss/cli/fboss2/commands/show/fsdb/CmdShowFsdbPublishers.h"
#include "fboss/cli/fboss2/commands/show/fsdb/CmdShowFsdbSubscribers.h"
#include "fboss/cli/fboss2/test/CmdHandlerTestBase.h"

namespace facebook::fboss {

class CmdShowFsdbTestFixture : public CmdHandlerTestBase {};

TEST_F(CmdShowFsdbTestFixture, testShowSubscibers) {
  setupMockedFsdbServer();
  auto subscribers = fsdb::SubscriberIdToOperSubscriberInfos();

  fsdb::OperSubscriberInfo sub1;
  subscribers["test-client1"] = {sub1};

  EXPECT_CALL(getMockFsdb(), sync_getAllOperSubscriberInfos(::testing::_))
      .Times(1)
      .WillOnce(([&](auto& ret) { ret = subscribers; }));

  EXPECT_CALL(
      getMockFsdb(), sync_getOperSubscriberInfos(::testing::_, ::testing::_))
      .Times(1)
      .WillOnce(([&](auto& ret, const auto&) {
        ret = fsdb::SubscriberIdToOperSubscriberInfos();
      }));

  std::vector<std::string> subIds = {};
  auto subInfos = CmdShowFsdbSubscribers().queryClient(localhost(), subIds);
  EXPECT_EQ(subInfos.size(), 1);

  subIds = {"test-client2"};
  subInfos = CmdShowFsdbSubscribers().queryClient(localhost(), subIds);
  EXPECT_EQ(subInfos.size(), 0);
}

TEST_F(CmdShowFsdbTestFixture, testShowPublishers) {
  setupMockedFsdbServer();
  auto publishers = fsdb::PublisherIdToOperPublisherInfo();

  fsdb::OperPublisherInfo pub1;
  publishers["test-client1"] = {pub1};

  EXPECT_CALL(getMockFsdb(), sync_getAllOperPublisherInfos(::testing::_))
      .Times(1)
      .WillOnce(([&](auto& ret) { ret = publishers; }));

  EXPECT_CALL(
      getMockFsdb(), sync_getOperPublisherInfos(::testing::_, ::testing::_))
      .Times(1)
      .WillOnce(([&](auto& ret, const auto&) {
        ret = fsdb::PublisherIdToOperPublisherInfo();
      }));

  std::vector<std::string> pubIds = {};
  auto pubInfos = CmdShowFsdbPublishers().queryClient(localhost(), pubIds);
  EXPECT_EQ(pubInfos.size(), 1);
  pubIds = {"test-client2"};
  pubInfos = CmdShowFsdbPublishers().queryClient(localhost(), pubIds);
  EXPECT_EQ(pubInfos.size(), 0);
}

} // namespace facebook::fboss
