// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include <gflags/gflags.h>
#include <gtest/gtest.h>

#include "fboss/fsdb/client/FsdbPubSubManager.h"
#include "fboss/fsdb/tests/client/FsdbTestClients.h"
#include "fboss/fsdb/tests/utils/FsdbTestServer.h"
#include "fboss/lib/CommonUtils.h"

DECLARE_bool(enableHybridStateStorage);

namespace facebook::fboss::fsdb::test {

namespace {
auto constexpr kPublisherId = "fsdb_hybrid_test_publisher";
const std::vector<std::string> kPublishRoot{"agent"};
} // namespace

class FsdbHybridStateStorageTest : public ::testing::TestWithParam<bool> {
 public:
  void SetUp() override {
    savedFlag_ = FLAGS_enableHybridStateStorage;
    FLAGS_enableHybridStateStorage = GetParam();
    // ServiceHandler reads the flag in its ctor.
    fsdb_ = std::make_unique<FsdbTestServer>();
  }

  void TearDown() override {
    if (pubSubManager_) {
      pubSubManager_->removeStatePathPublisher();
      pubSubManager_.reset();
    }
    fsdb_.reset();
    FLAGS_enableHybridStateStorage = savedFlag_;
  }

 protected:
  void createStatePublisher() {
    pubSubManager_ = std::make_unique<FsdbPubSubManager>(kPublisherId);
    pubSubManager_->createStatePathPublisher(
        kPublishRoot, [](auto, auto) {}, fsdb_->getFsdbPort());
    WITH_RETRIES({
      auto metadata =
          fsdb_->getPublisherRootMetadata(*kPublishRoot.begin(), false);
      ASSERT_EVENTUALLY_TRUE(metadata);
      EXPECT_EVENTUALLY_EQ(metadata->numOpenConnections, 1);
    });
  }

  bool savedFlag_{false};
  std::unique_ptr<FsdbTestServer> fsdb_;
  std::unique_ptr<FsdbPubSubManager> pubSubManager_;
};

TEST_P(FsdbHybridStateStorageTest, selectsRequestedStorage) {
  EXPECT_EQ(fsdb_->serviceHandler().usingHybridStateStorage(), GetParam());
}

TEST_P(FsdbHybridStateStorageTest, publishedStateVisibleOnSelectedStorage) {
  createStatePublisher();
  auto config = makeAgentConfig({{"foo", "bar"}});
  pubSubManager_->publishState(makeState(config));
  WITH_RETRIES({
    auto root = fsdb_->serviceHandler().operRootExpensive();
    EXPECT_EVENTUALLY_EQ(*root.agent()->config(), config);
  });
}

INSTANTIATE_TEST_SUITE_P(
    StorageMode,
    FsdbHybridStateStorageTest,
    ::testing::Bool(),
    [](const ::testing::TestParamInfo<bool>& info) {
      return info.param ? "Hybrid" : "Cow";
    });

} // namespace facebook::fboss::fsdb::test
