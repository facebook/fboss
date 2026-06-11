// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include <gtest/gtest.h>

#include "fboss/cli/fboss2/commands/show/fsdb/CmdShowFsdbPublishers.h"
#include "fboss/cli/fboss2/commands/show/fsdb/CmdShowFsdbSubscribers.h"
#include "fboss/cli/fboss2/test/CmdHandlerTestBase.h"
#include "fboss/cli/fboss2/utils/CmdUtils.h"

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

// A subscriber path that cannot be resolved against the locally compiled
// FsdbModel (e.g. schema skew with the switch, or a deprecated path) must not
// abort the whole command. getSubscriptionPathStr() should fall back to the raw
// tokens instead of throwing "Invalid extended path".
TEST_F(CmdShowFsdbTestFixture, testGetSubscriptionPathStrPatchFallback) {
  fsdb::OperSubscriberInfo subscriber;
  subscriber.isStats() = false;
  fsdb::RawOperPath rawPath;
  // "99999" is not a valid field id/name in FsdbOperStateRoot, so conversion
  // fails and we expect the raw tokens to be displayed.
  rawPath.path() = {"99999", "12345"};
  subscriber.paths() = {{0, rawPath}};

  EXPECT_EQ(utils::getSubscriptionPathStr(subscriber), "99999/12345");
}

TEST_F(CmdShowFsdbTestFixture, testGetSubscriptionPathStrExtendedFallback) {
  fsdb::OperSubscriberInfo subscriber;
  subscriber.isStats() = false;
  fsdb::ExtendedOperPath extPath;
  fsdb::OperPathElem elem;
  elem.raw() = "99999";
  extPath.path() = {elem};
  subscriber.extendedPaths() = {extPath};

  EXPECT_EQ(utils::getSubscriptionPathStr(subscriber), "99999");
}

} // namespace facebook::fboss
