// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/platform/reboot_cause_finder/RebootCauseFinderImpl.h"

#include <filesystem>
#include <string>
#include <vector>

#include <folly/FileUtil.h>
#include <folly/json.h>
#include <gtest/gtest.h>
#include <thrift/lib/cpp2/protocol/Serializer.h>

using namespace facebook::fboss::platform::reboot_cause_finder;
namespace rcc = facebook::fboss::platform::reboot_cause_config;

namespace {

class RebootCauseFinderImplTest : public ::testing::Test {
 protected:
  void SetUp() override {
    auto tmpl =
        (std::filesystem::temp_directory_path() / "rcf_test_XXXXXX").string();
    std::vector<char> buf(tmpl.begin(), tmpl.end());
    buf.push_back('\0');
    ASSERT_NE(::mkdtemp(buf.data()), nullptr);
    tmpDir_ = std::filesystem::path(buf.data());
  }

  void TearDown() override {
    std::error_code ec;
    std::filesystem::remove_all(tmpDir_, ec);
  }

  std::filesystem::path tmpDir_;
};

facebook::fboss::platform::reboot_cause_config::RebootCause causeAt(
    const std::string& description,
    int64_t epochSec) {
  facebook::fboss::platform::reboot_cause_config::RebootCause c;
  c.description() = description;
  c.occurredAtMs() = epochSec * 1000;
  c.occurredAtPacific() = "";
  return c;
}

facebook::fboss::platform::reboot_cause_config::RebootCauseProviderAttempt
attemptWith(
    const std::string& name,
    std::vector<facebook::fboss::platform::reboot_cause_config::RebootCause>
        causes) {
  facebook::fboss::platform::reboot_cause_config::RebootCauseProviderAttempt a;
  a.name() = name;
  a.status() = facebook::fboss::platform::reboot_cause_config::
      RebootCauseProviderStatus::OK;
  a.detail() = "";
  a.causes() = std::move(causes);
  return a;
}

// ------------------------------------------------- record honesty contract

// determinedCause must be unset, not a placeholder, when nothing was found.
// A placeholder has to carry a placeholder timestamp, and that timestamp
// necessarily postdates the boot it claims to explain.
TEST_F(RebootCauseFinderImplTest, DeterminedCauseUnsetWhenNothingFound) {
  facebook::fboss::platform::reboot_cause_config::RebootCauseRecord record;
  EXPECT_FALSE(record.determinedCause().has_value());

  const auto json = folly::parseJson(
      apache::thrift::SimpleJSONSerializer::serialize<std::string>(record));
  // Unset optionals are omitted by SimpleJSON, so absence is self-describing
  // rather than requiring consumers to string-match a sentinel.
  EXPECT_EQ(json.count("determinedCause"), 0);
}

TEST_F(RebootCauseFinderImplTest, DeterminedCauseSerialisedWhenFound) {
  facebook::fboss::platform::reboot_cause_config::RebootCauseRecord record;
  facebook::fboss::platform::reboot_cause_config::DeterminedCause d;
  d.providerName() = "ManualReboot";
  d.cause() = causeAt("Manual x86 Reboot", 1790195926);
  record.determinedCause() = d;

  const auto json = folly::parseJson(
      apache::thrift::SimpleJSONSerializer::serialize<std::string>(record));
  ASSERT_EQ(json.count("determinedCause"), 1);
  EXPECT_EQ(json["determinedCause"]["providerName"].asString(), "ManualReboot");
}

// A record must distinguish "read cleanly, nothing to report" from "the
// source was unreadable". Both produce an empty allCauses.
TEST_F(RebootCauseFinderImplTest, ProviderAttemptStatusesRoundTrip) {
  using facebook::fboss::platform::reboot_cause_config::
      RebootCauseProviderStatus;
  facebook::fboss::platform::reboot_cause_config::RebootCauseRecord record;

  facebook::fboss::platform::reboot_cause_config::RebootCauseProviderAttempt a;
  a.name() = "MERU_SCM_CPLD";
  a.status() = RebootCauseProviderStatus::READ_FAILED;
  a.detail() = "/run/devmap/fpgas/MERU_SCM_CPLD/reboot_causes";

  facebook::fboss::platform::reboot_cause_config::RebootCauseProviderAttempt b;
  b.name() = "KernelPanic";
  b.status() = RebootCauseProviderStatus::SKIPPED;
  b.detail() = "boot time unavailable";

  record.providersAttempted() = {a, b};

  const auto json = folly::parseJson(
      apache::thrift::SimpleJSONSerializer::serialize<std::string>(record));
  ASSERT_EQ(json["providersAttempted"].size(), 2);
  EXPECT_EQ(json["providersAttempted"][0]["name"].asString(), "MERU_SCM_CPLD");
  EXPECT_NE(
      json["providersAttempted"][0]["status"],
      json["providersAttempted"][1]["status"]);
}

TEST_F(RebootCauseFinderImplTest, BootTimeMsIsRecorded) {
  facebook::fboss::platform::reboot_cause_config::RebootCauseRecord record;
  record.bootTimeMs() = 1789535835000;
  const auto json = folly::parseJson(
      apache::thrift::SimpleJSONSerializer::serialize<std::string>(record));
  EXPECT_EQ(json["bootTimeMs"].asInt(), 1789535835000);
}

// A cause does not name its provider. Which provider found it is given by
// the attempt it sits in, and by DeterminedCause for the winner.
TEST_F(RebootCauseFinderImplTest, CausesAreNestedUnderTheirProvider) {
  facebook::fboss::platform::reboot_cause_config::RebootCauseRecord record;
  record.providersAttempted() = {
      attemptWith("KernelPanic", {causeAt("Kernel Panic", 1790195900)}),
      attemptWith("ManualReboot", {})};

  const auto json = folly::parseJson(
      apache::thrift::SimpleJSONSerializer::serialize<std::string>(record));
  ASSERT_EQ(json["providersAttempted"].size(), 2);
  EXPECT_EQ(json["providersAttempted"][0]["causes"].size(), 1);
  EXPECT_EQ(json["providersAttempted"][1]["causes"].size(), 0);
  // No per-cause providerName, and no flat allCauses list.
  EXPECT_EQ(
      json["providersAttempted"][0]["causes"][0].count("providerName"), 0);
  EXPECT_EQ(json.count("allCauses"), 0);
}

// ------------------------------------------------------- hardware providers

// readProvider is the only code path that runs today, and its status is what
// the record now reports. Absent, unparseable and good are three distinct
// outcomes and must not collapse into one another.
namespace {
rcc::RebootCauseProviderConfig providerAt(const std::string& path) {
  rcc::RebootCauseProviderConfig c;
  c.name() = "TEST_CPLD";
  c.priority() = 1;
  c.sysfsReadPath() = path;
  c.sysfsClearPath() = path + ".clear";
  return c;
}
} // namespace

TEST_F(RebootCauseFinderImplTest, ProviderGoodReadIsOkAndDecodes) {
  const auto path = (tmpDir_ / "causes").string();
  ASSERT_TRUE(
      folly::writeFile(
          std::string(
              R"({"causes":[{"description":"Power loss",)"
              R"("date":"09-24-2026 16:32:00","rawValue":"0x40"}]})"),
          path.c_str()));

  const auto attempt = detail::readProvider(providerAt(path));
  const auto& causes = *attempt.causes();

  EXPECT_EQ(*attempt.status(), rcc::RebootCauseProviderStatus::OK);
  ASSERT_EQ(causes.size(), 1);
  EXPECT_EQ(*causes[0].description(), "Power loss");
  ASSERT_TRUE(causes[0].rawValue().has_value());
  EXPECT_EQ(*causes[0].rawValue(), "0x40");
  EXPECT_NE(*causes[0].occurredAtMs(), 0);
}

// A provider that reads cleanly but reports nothing is OK with no causes --
// the case that must not look like a failure.
TEST_F(RebootCauseFinderImplTest, ProviderEmptyCauseListIsOk) {
  const auto path = (tmpDir_ / "causes").string();
  ASSERT_TRUE(folly::writeFile(std::string(R"({"causes":[]})"), path.c_str()));

  const auto attempt = detail::readProvider(providerAt(path));
  const auto& causes = *attempt.causes();

  EXPECT_EQ(*attempt.status(), rcc::RebootCauseProviderStatus::OK);
  EXPECT_TRUE(causes.empty());
}

TEST_F(RebootCauseFinderImplTest, ProviderMalformedJsonIsParseFailed) {
  const auto path = (tmpDir_ / "causes").string();
  ASSERT_TRUE(folly::writeFile(std::string("not json at all"), path.c_str()));

  const auto attempt = detail::readProvider(providerAt(path));
  const auto& causes = *attempt.causes();

  EXPECT_EQ(*attempt.status(), rcc::RebootCauseProviderStatus::PARSE_FAILED);
  EXPECT_TRUE(causes.empty());
}

TEST_F(RebootCauseFinderImplTest, ProviderMissingFileIsReadFailed) {
  const auto attempt =
      detail::readProvider(providerAt((tmpDir_ / "nope").string()));
  const auto& causes = *attempt.causes();

  EXPECT_EQ(*attempt.status(), rcc::RebootCauseProviderStatus::READ_FAILED);
  EXPECT_TRUE(causes.empty());
}

// An unparseable date must not discard the cause: the description is still
// the useful part, and the raw string is preserved for a human.
TEST_F(RebootCauseFinderImplTest, ProviderUnparseableDateKeepsTheCause) {
  const auto path = (tmpDir_ / "causes").string();
  ASSERT_TRUE(
      folly::writeFile(
          std::string(
              R"({"causes":[{"description":"Power loss","date":"soon"}]})"),
          path.c_str()));

  const auto attempt = detail::readProvider(providerAt(path));
  const auto& causes = *attempt.causes();

  EXPECT_EQ(*attempt.status(), rcc::RebootCauseProviderStatus::OK);
  ASSERT_EQ(causes.size(), 1);
  EXPECT_EQ(*causes[0].occurredAtMs(), 0);
  EXPECT_EQ(*causes[0].occurredAtPacific(), "soon");
}

} // namespace
