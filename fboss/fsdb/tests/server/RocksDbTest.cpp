// (c) Facebook, Inc. and its affiliates. Confidential and proprietary.

#include <gtest/gtest.h>
#include <chrono>

#include <folly/Range.h>
#include <folly/Singleton.h>
#include <folly/experimental/AutoTimer.h>
#include <folly/experimental/TestUtil.h>
#include <folly/logging/xlog.h>

#include "common/init/Init.h"
#include "fboss/agent/SetupThrift.h"
#include "fboss/fsdb/server/RocksDb.h"
#include "fboss/fsdb/server/StatsSnapshot.h"
#include "fboss/fsdb/tests/utils/Utils.h"

using namespace ::testing;

int main(int argc, char* argv[]) {
  // Parse command line flags
  testing::InitGoogleTest(&argc, argv);

  facebook::initFacebook(&argc, &argv);

  return RUN_ALL_TESTS();
}

namespace facebook::fboss::fsdb::test {

const Metric kTestMetric{"testMetric"};
const std::string kCategory{"stats"};

class RocksDbTest : public ::testing::Test {
 public:
  RocksDbTest() = default;
  ~RocksDbTest() override = default;

  void SetUp() override {
    // Pick different dir for tests
    tmpDir_ =
        std::make_unique<folly::test::TemporaryDirectory>("fsdb_rocksdb_tests");
    gflags::SetCommandLineOptionWithMode(
        "rocksdbDir",
        tmpDir_->path().string().c_str(),
        gflags::SET_FLAG_IF_DEFAULT);
  }

  void TearDown() override {
    tmpDir_.reset();
  }

  // NOTE: tests may be run concurrently by gtest. But rocksdb with same dir
  // name may not be used concurrently. Disambiguate rocksdb instances for each
  // test using complete test name.
  std::string getPublisherId() {
    return folly::to<std::string>(
        "testPublisher-",
        testing::UnitTest::GetInstance()->current_test_info()->test_case_name(),
        "-",
        testing::UnitTest::GetInstance()->current_test_info()->name());
  }

 private:
  std::unique_ptr<folly::test::TemporaryDirectory> tmpDir_;
};

TEST_F(RocksDbTest, writePointReadRangeQuery) {
  auto snapshot = std::make_shared<StatsSnapshot>(
      fsdb::Utils::getFqMetric(getPublisherId(), kTestMetric),
      Utils::makeVectors({"sum", "rate"}));
  EXPECT_FALSE(snapshot->isControl());

  // open, write, close
  {
    using namespace std::chrono_literals;
    RocksDb rocksDb{
        kCategory, getPublisherId(), 0s /* ttl */, true /* eraseExisting */};
    EXPECT_TRUE(rocksDb.open());
    EXPECT_TRUE(rocksDb.put(snapshot->key(), snapshot->value()));
    EXPECT_TRUE(rocksDb.close());
  }

  // open, point read, close
  {
    RocksDb rocksDb{kCategory, getPublisherId()};
    EXPECT_TRUE(rocksDb.open());
    std::string value;
    EXPECT_TRUE(rocksDb.get(snapshot->key(), &value));
    EXPECT_TRUE(rocksDb.close());

    const auto [readMetric, readTimestampSec] =
        fsdb::Utils::decomposeKey(snapshot->key());
    auto readSnapshot = std::make_shared<StatsSnapshot>(
        fsdb::Utils::getFqMetric(getPublisherId(), readMetric),
        readTimestampSec,
        value);
    EXPECT_TRUE(
        ThriftVisitationEquals(snapshot->payload(), readSnapshot->payload()));
  }

  // open, range query, close
  {
    RocksDb rocksDb{kCategory, getPublisherId()};
    EXPECT_TRUE(rocksDb.open());

    // check for snapshots at most an hour old
    auto kSinceTimeIntervalAgoSec = 3600;
    auto readSnapshots = rocksDb.getSnapshots(
        kTestMetric, fsdb::Utils::getNowSec() - kSinceTimeIntervalAgoSec);
    EXPECT_EQ(readSnapshots.size(), 1);
    const auto& readSnapshot = readSnapshots.front();
    EXPECT_EQ(*readSnapshot, *snapshot);
    EXPECT_TRUE(rocksDb.close(true /* erase */));
  }
}

TEST_F(RocksDbTest, bulkWritesToFake) {
  auto snapshot = std::make_shared<StatsSnapshot>(
      fsdb::Utils::getFqMetric(getPublisherId(), kTestMetric),
      Utils::makeVectors({"sum", "rate"}));
  EXPECT_FALSE(snapshot->isControl());

  // open, bulk write, close
  {
    using namespace std::chrono_literals;
    RocksDbFake rocksDb{
        kCategory, getPublisherId(), 0s /* ttl */, true /* eraseExisting */};
    EXPECT_TRUE(rocksDb.open());

    constexpr int kLogAt = 1000;
    constexpr int kLimit = kLogAt * 50;
    {
      folly::AutoTimer t(fmt::format("{} writes done", kLimit));
      for (auto i = 1; i <= kLimit; ++i) {
        XLOG_IF(INFO, (i % kLogAt == 0)) << "Written " << i << " times";
        EXPECT_TRUE(rocksDb.put(snapshot->key(), snapshot->value()));
      }
    }

    EXPECT_TRUE(rocksDb.close());
  }
}

} // namespace facebook::fboss::fsdb::test
