// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "fboss/platform/platform_manager/gen-cpp2/PlatformManagerService.h"
#include "fboss/platform/platform_manager/gen-cpp2/platform_manager_service_types.h"
#include "fboss/platform/sensor_service/PmClientFactory.h"
#include "fboss/platform/sensor_service/PmUnitInfoFetcher.h"

namespace pm = facebook::fboss::platform::platform_manager;
using namespace ::testing;

namespace facebook::fboss::platform::sensor_service {

class FakePmClient : public apache::thrift::Client<pm::PlatformManagerService> {
 public:
  explicit FakePmClient(
      std::function<void(pm::PmUnitInfoResponse&, const pm::PmUnitInfoRequest&)>
          handler)
      : apache::thrift::Client<pm::PlatformManagerService>(nullptr),
        handler_(std::move(handler)) {}

  void sync_getPmUnitInfo(
      pm::PmUnitInfoResponse& response,
      const pm::PmUnitInfoRequest& request) override {
    handler_(response, request);
  }

 private:
  std::function<void(pm::PmUnitInfoResponse&, const pm::PmUnitInfoRequest&)>
      handler_;
};

class MockPmClientFactory : public PmClientFactory {
 public:
  MOCK_METHOD(
      (std::unique_ptr<apache::thrift::Client<pm::PlatformManagerService>>),
      create,
      (),
      (const, override));
};

class PmUnitInfoFetcherTest : public Test {
 protected:
  void SetUp() override {
    mockFactory_ = std::make_shared<MockPmClientFactory>();
  }

  std::shared_ptr<MockPmClientFactory> mockFactory_;
};

TEST_F(PmUnitInfoFetcherTest, FetchWithValidVersion) {
  EXPECT_CALL(*mockFactory_, create()).WillOnce([]() {
    return std::make_unique<FakePmClient>(
        [](pm::PmUnitInfoResponse& response, const pm::PmUnitInfoRequest&) {
          pm::PmUnitVersion version;
          version.productProductionState() = 1;
          version.productVersion() = 2;
          version.productSubVersion() = 3;

          pm::PmUnitInfo info;
          info.name() = "TestPmUnit";
          info.version() = version;

          response.pmUnitInfo() = info;
        });
  });

  PmUnitInfoFetcher fetcher(
      std::static_pointer_cast<PmClientFactory>(mockFactory_));
  auto result = fetcher.fetch("/test/slot");

  ASSERT_TRUE(result.has_value());
  const std::array<int16_t, 3> expected{1, 2, 3};
  EXPECT_EQ(*result, expected);
}

TEST_F(PmUnitInfoFetcherTest, FetchWithoutVersion) {
  EXPECT_CALL(*mockFactory_, create()).WillOnce([]() {
    return std::make_unique<FakePmClient>(
        [](pm::PmUnitInfoResponse& response, const pm::PmUnitInfoRequest&) {
          pm::PmUnitInfo info;
          info.name() = "TestPmUnitNoVersion";
          response.pmUnitInfo() = info;
        });
  });

  PmUnitInfoFetcher fetcher(
      std::static_pointer_cast<PmClientFactory>(mockFactory_));
  auto result = fetcher.fetch("/test/slot");

  EXPECT_FALSE(result.has_value());
}

TEST_F(PmUnitInfoFetcherTest, FetchWithPlatformManagerError) {
  EXPECT_CALL(*mockFactory_, create()).WillOnce([]() {
    return std::make_unique<FakePmClient>(
        [](pm::PmUnitInfoResponse&, const pm::PmUnitInfoRequest&) {
          pm::PlatformManagerError error;
          error.errorCode() = pm::PlatformManagerErrorCode::PM_UNIT_NOT_FOUND;
          error.message() = "PmUnit not found";
          throw error;
        });
  });

  PmUnitInfoFetcher fetcher(
      std::static_pointer_cast<PmClientFactory>(mockFactory_));
  auto result = fetcher.fetch("/test/slot");

  EXPECT_FALSE(result.has_value());
}

TEST_F(PmUnitInfoFetcherTest, FetchWithStdException) {
  EXPECT_CALL(*mockFactory_, create()).WillOnce([]() {
    return std::make_unique<FakePmClient>(
        [](pm::PmUnitInfoResponse&, const pm::PmUnitInfoRequest&) {
          throw std::runtime_error("Connection failed");
        });
  });

  PmUnitInfoFetcher fetcher(
      std::static_pointer_cast<PmClientFactory>(mockFactory_));
  auto result = fetcher.fetch("/test/slot");

  EXPECT_FALSE(result.has_value());
}

} // namespace facebook::fboss::platform::sensor_service
