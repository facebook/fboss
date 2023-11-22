// (c) Facebook, Inc. and its affiliates. Confidential and proprietary.

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <chrono>

#include <folly/Range.h>
#include <folly/Singleton.h>
#include <folly/logging/xlog.h>

#include "common/init/Init.h"
#include "fboss/agent/SetupThrift.h"
#include "fboss/fsdb/if/FsdbModel.h"
#include "fboss/fsdb/if/gen-cpp2/fsdb_common_visitation.h"
#include "fboss/fsdb/if/gen-cpp2/fsdb_visitation.h"
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
const PublisherId kTestPublisherId{"testPublisher"};

class StatsSnapshotTest : public ::testing::TestWithParam<bool> {
 public:
  StatsSnapshotTest() = default;
  ~StatsSnapshotTest() override = default;

 private:
};

TEST_P(StatsSnapshotTest, serializeDeserializeControl) {
  const auto available = GetParam();

  // create snapshot
  auto snapshot = std::make_shared<StatsSnapshot>(
      fsdb::Utils::getFqMetric(kTestPublisherId, kTestMetric), available);
  EXPECT_GT(snapshot->timestampSec(), 0);

  // check key
  const auto [metric, timestampSec] =
      fsdb::Utils::decomposeKey(snapshot->key());
  EXPECT_EQ(metric.compare(kTestMetric), 0);
  EXPECT_EQ(timestampSec, snapshot->timestampSec());

  // check value
  const auto payload = StatsSnapshot::fromValue(snapshot->value());
  EXPECT_EQ(
      payload.getType(),
      available ? StatsSubUnitPayload::Type::available
                : StatsSubUnitPayload::Type::unavailable);

  // TODO: check for other fields in control pkt
}

TEST_F(StatsSnapshotTest, serializeDeserializeData) {
  // create snapshot
  auto snapshot = std::make_shared<StatsSnapshot>(
      fsdb::Utils::getFqMetric(kTestPublisherId, kTestMetric),
      Utils::makeVectors({"sum", "rate"}));
  EXPECT_GT(snapshot->timestampSec(), 0);

  // check key
  const auto [metric, timestampSec] =
      fsdb::Utils::decomposeKey(snapshot->key());
  EXPECT_EQ(metric.compare(kTestMetric), 0);
  EXPECT_EQ(timestampSec, snapshot->timestampSec());

  // check value
  const auto payload = StatsSnapshot::fromValue(snapshot->value());
  EXPECT_EQ(payload.getType(), StatsSubUnitPayload::Type::dataCompressed);
  EXPECT_EQ(
      payload.dataCompressed_ref()->vectors(),
      Utils::makeVectors({"sum", "rate"}));
}

TEST_F(StatsSnapshotTest, decompress) {
  // create snapshot
  auto snapshot = std::make_shared<StatsSnapshot>(
      fsdb::Utils::getFqMetric(kTestPublisherId, kTestMetric),
      Utils::makeVectors({"sum", "rate"}));
  EXPECT_EQ(
      snapshot->payload().getType(), StatsSubUnitPayload::Type::dataCompressed);

  // decompress
  snapshot->decompress();
  EXPECT_EQ(snapshot->payload().getType(), StatsSubUnitPayload::Type::data);

  // check decompressed keys
  for (const auto& [key, _] : *snapshot->payload().data_ref()->counters()) {
    EXPECT_THAT(key, StartsWith(kTestMetric));
    const auto suffix = key.substr(kTestMetric.size());
    EXPECT_THAT(suffix, AnyOf(StartsWith(".sum"), StartsWith(".rate")));
  }
}

INSTANTIATE_TEST_SUITE_P(
    StatsSnapshotTestInstantiation,
    StatsSnapshotTest,
    testing::Bool());

} // namespace facebook::fboss::fsdb::test
