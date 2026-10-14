// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include <sys/stat.h>
#include <unistd.h>

#include <filesystem>
#include <ranges>

#include <folly/FileUtil.h>
#include <folly/testing/TestUtil.h>
#include <gtest/gtest.h>

#include "fboss/platform/sensor_service/SensorServiceThriftHandler.h"

DECLARE_string(reboot_history_dir);
#include "fboss/platform/sensor_service/tests/TestUtils.h"

namespace facebook::fboss::platform::sensor_service {

class SensorServiceThriftHandlerTest : public testing::Test {
 public:
  void SetUp() override {
    folly::test::TemporaryDirectory tmpDir;
    auto config = getMockSensorConfig(tmpDir.path().string());
    auto impl = std::make_shared<SensorServiceImpl>(config);
    impl->fetchSensorData();
    sensorServiceHandler_ =
        std::make_unique<SensorServiceThriftHandler>(std::move(impl));
  }
  std::unique_ptr<SensorServiceThriftHandler> sensorServiceHandler_;
  std::map<std::string, float> sensorMockData_{getMockSensorData()};
};

TEST_F(
    SensorServiceThriftHandlerTest,
    getSensorValuesByNameWithEmptySensorName) {
  SensorReadResponse response;
  sensorServiceHandler_->getSensorValuesByNames(
      response, std::make_unique<std::vector<std::string>>());
  EXPECT_EQ(response.sensorData()->size(), sensorMockData_.size());
  for (auto& sensorDatum : *response.sensorData()) {
    EXPECT_EQ(sensorMockData_.at(*sensorDatum.name()), *sensorDatum.value());
    EXPECT_EQ(*sensorDatum.slotPath(), "/");
    if (*sensorDatum.name() == "MOCK_FRU_SENSOR1") {
      EXPECT_TRUE(
          sensorDatum.sysfsPath()->ends_with("mock_fru_sensor_1_path:temp1"));
    } else if (*sensorDatum.name() == "MOCK_FRU_SENSOR2") {
      EXPECT_TRUE(
          sensorDatum.sysfsPath()->ends_with("mock_fru_sensor_2_path:fan1"));
    } else if (*sensorDatum.name() == "MOCK_FRU_SENSOR3") {
      EXPECT_TRUE(
          sensorDatum.sysfsPath()->ends_with("mock_fru_sensor_3_path:vin"));
    }
  }
}

TEST_F(
    SensorServiceThriftHandlerTest,
    getSensorValuesByNameWithNonEmptySensorName) {
  auto mockSensorData = getMockSensorData();
  auto mockSensorNamesIt = std::views::keys(mockSensorData).begin();

  auto sensorNames = std::vector<std::string>{*mockSensorNamesIt};
  SensorReadResponse response;
  sensorServiceHandler_->getSensorValuesByNames(
      response, std::make_unique<std::vector<std::string>>(sensorNames));
  EXPECT_EQ(response.sensorData()->size(), sensorNames.size());
  EXPECT_EQ(
      sensorMockData_.at(*mockSensorNamesIt),
      *response.sensorData()[0].value());

  sensorNames.push_back(*(++mockSensorNamesIt));
  sensorServiceHandler_->getSensorValuesByNames(
      response, std::make_unique<std::vector<std::string>>(sensorNames));
  EXPECT_EQ(response.sensorData()->size(), sensorNames.size());
  for (auto& sensorDatum : *response.sensorData()) {
    EXPECT_EQ(sensorMockData_.at(*sensorDatum.name()), *sensorDatum.value());
  }
}

// ---------------------------------------------------------------------------
// getRebootHistory
//
// The records are written by a separate process on its own schedule, so the
// contract under test is that nothing found on disk -- absent, truncated,
// malformed, oversized, or simply too numerous -- makes this RPC throw.
// ---------------------------------------------------------------------------

namespace {
// Built by concatenation rather than fmt: the JSON braces would have to be
// doubled to satisfy fmt's compile-time format-string check, which makes the
// literal unreadable as JSON.
std::string makeRecord(const std::string& id, int64_t detectedAtMs) {
  return std::string("{\n") + "  \"bootId\": \"boot-" + id + "\",\n" +
      "  \"bootTimeMs\": " + std::to_string(detectedAtMs - 1000) + ",\n" +
      "  \"detectedAtMs\": " + std::to_string(detectedAtMs) + ",\n" +
      "  \"detectedAtPacific\": \"2026-10-06 12:48:56 PDT\",\n" +
      "  \"providersAttempted\": []\n}";
}
} // namespace

class RebootHistoryTest : public testing::Test {
 protected:
  void SetUp() override {
    savedHistoryDir_ = FLAGS_reboot_history_dir;
    dir_ = std::make_unique<folly::test::TemporaryDirectory>();
    FLAGS_reboot_history_dir = dir_->path().string();
    sensorDir_ = std::make_unique<folly::test::TemporaryDirectory>();
    auto config = getMockSensorConfig(sensorDir_->path().string());
    auto impl = std::make_shared<SensorServiceImpl>(config);
    handler_ = std::make_unique<SensorServiceThriftHandler>(std::move(impl));
  }

  void TearDown() override {
    // The flag is global; leaving it pointed at a deleted temp dir would
    // leak into whatever test runs next.
    FLAGS_reboot_history_dir = savedHistoryDir_;
  }
  std::filesystem::path path(const std::string& name) {
    return std::filesystem::path(dir_->path().string()) / name;
  }
  void write(const std::filesystem::path& p, const std::string& contents) {
    ASSERT_TRUE(folly::writeFile(contents, p.c_str()));
  }
  RebootHistoryResponse call() {
    RebootHistoryResponse response;
    handler_->getRebootHistory(response);
    return response;
  }
  std::string savedHistoryDir_;
  std::unique_ptr<folly::test::TemporaryDirectory> dir_;
  std::unique_ptr<folly::test::TemporaryDirectory> sensorDir_;
  std::unique_ptr<SensorServiceThriftHandler> handler_;
};

TEST_F(RebootHistoryTest, MissingDirectoryIsNotAnError) {
  FLAGS_reboot_history_dir = "/nonexistent/reboot_history";
  RebootHistoryResponse response;
  EXPECT_NO_THROW({ response = call(); });
  EXPECT_TRUE(response.records()->empty());
  EXPECT_EQ(*response.skippedCount(), 0);
  EXPECT_FALSE(response.warnings()->empty());
}

TEST_F(RebootHistoryTest, EmptyDirectoryReturnsNothing) {
  auto response = call();
  EXPECT_TRUE(response.records()->empty());
  EXPECT_EQ(*response.skippedCount(), 0);
}

TEST_F(RebootHistoryTest, ReturnsValidRecord) {
  write(path("reboot-cause-1.json"), makeRecord("aaa", 1000));
  auto response = call();
  ASSERT_EQ(response.records()->size(), 1);
  EXPECT_EQ(*response.records()->at(0).bootId(), "boot-aaa");
  EXPECT_EQ(*response.skippedCount(), 0);
}

TEST_F(RebootHistoryTest, OrdersNewestFirstByDetectedAtMs) {
  // Written so filename order disagrees with detectedAtMs order.
  write(path("reboot-cause-a.json"), makeRecord("oldest", 1000));
  write(path("reboot-cause-b.json"), makeRecord("newest", 3000));
  write(path("reboot-cause-c.json"), makeRecord("middle", 2000));
  auto response = call();
  ASSERT_EQ(response.records()->size(), 3);
  EXPECT_EQ(*response.records()->at(0).bootId(), "boot-newest");
  EXPECT_EQ(*response.records()->at(1).bootId(), "boot-middle");
  EXPECT_EQ(*response.records()->at(2).bootId(), "boot-oldest");
}

TEST_F(RebootHistoryTest, MalformedRecordIsSkippedAndOthersSurvive) {
  write(path("reboot-cause-good.json"), makeRecord("good", 2000));
  write(path("reboot-cause-bad.json"), "{ this is not json");
  RebootHistoryResponse response;
  EXPECT_NO_THROW({ response = call(); });
  ASSERT_EQ(response.records()->size(), 1);
  EXPECT_EQ(*response.records()->at(0).bootId(), "boot-good");
  EXPECT_EQ(*response.skippedCount(), 1);
  EXPECT_FALSE(response.warnings()->empty());
}

TEST_F(RebootHistoryTest, TruncatedRecordIsSkipped) {
  auto full = makeRecord("truncated", 1000);
  write(path("reboot-cause-trunc.json"), full.substr(0, full.size() / 2));
  RebootHistoryResponse response;
  EXPECT_NO_THROW({ response = call(); });
  EXPECT_TRUE(response.records()->empty());
  EXPECT_EQ(*response.skippedCount(), 1);
}

TEST_F(RebootHistoryTest, EmptyFileIsSkipped) {
  write(path("reboot-cause-empty.json"), "");
  RebootHistoryResponse response;
  EXPECT_NO_THROW({ response = call(); });
  EXPECT_TRUE(response.records()->empty());
  EXPECT_EQ(*response.skippedCount(), 1);
}

TEST_F(RebootHistoryTest, BinaryGarbageIsSkipped) {
  write(path("reboot-cause-bin.json"), std::string("\x00\x01\x02\xff", 4));
  RebootHistoryResponse response;
  EXPECT_NO_THROW({ response = call(); });
  EXPECT_TRUE(response.records()->empty());
  EXPECT_EQ(*response.skippedCount(), 1);
}

TEST_F(RebootHistoryTest, OversizedRecordIsSkipped) {
  write(path("reboot-cause-big.json"), std::string((1 << 20) + 1, 'x'));
  RebootHistoryResponse response;
  EXPECT_NO_THROW({ response = call(); });
  EXPECT_TRUE(response.records()->empty());
  EXPECT_EQ(*response.skippedCount(), 1);
}

TEST_F(RebootHistoryTest, SubdirectoriesAreIgnored) {
  std::filesystem::create_directory(path("reboot-cause-dir.json"));
  write(path("reboot-cause-ok.json"), makeRecord("ok", 1000));
  auto response = call();
  ASSERT_EQ(response.records()->size(), 1);
  EXPECT_EQ(*response.skippedCount(), 0);
}

TEST_F(RebootHistoryTest, UnrelatedFilesAreIgnoredNotCounted) {
  // A stray file is not a record that failed; it was never a record.
  write(path("README"), "notes");
  write(path("reboot-cause-1.json.tmp"), "partial write");
  write(path("reboot-cause-ok.json"), makeRecord("ok", 1000));
  auto response = call();
  ASSERT_EQ(response.records()->size(), 1);
  EXPECT_EQ(*response.skippedCount(), 0);
}

TEST_F(RebootHistoryTest, UnreadableDirectoryReportsAWarning) {
  // A directory that cannot be opened must not look like an empty one: the
  // iterator is reset to end on error, so this only holds if the error code
  // is checked after construction rather than inside the loop.
  if (::geteuid() == 0) {
    GTEST_SKIP() << "root ignores the directory permissions this relies on";
  }
  ASSERT_EQ(::chmod(dir_->path().string().c_str(), 0), 0);
  RebootHistoryResponse response;
  EXPECT_NO_THROW({ response = call(); });
  ASSERT_EQ(::chmod(dir_->path().string().c_str(), 0755), 0);
  EXPECT_TRUE(response.records()->empty());
  EXPECT_FALSE(response.warnings()->empty());
}

TEST_F(RebootHistoryTest, CapsRecordCountAndReportsTheRest) {
  for (int i = 0; i < 70; ++i) {
    char name[64];
    snprintf(name, sizeof(name), "reboot-cause-%03d.json", i);
    char id[16];
    snprintf(id, sizeof(id), "%03d", i);
    write(path(name), makeRecord(id, 1000 + i));
  }
  auto response = call();
  EXPECT_EQ(response.records()->size(), 64);
  EXPECT_EQ(*response.skippedCount(), 6);
  EXPECT_FALSE(response.warnings()->empty());
  EXPECT_EQ(*response.records()->at(0).bootId(), "boot-069");
}

}; // namespace facebook::fboss::platform::sensor_service
