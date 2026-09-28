// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/platform/reboot_cause_finder/RebootCauseFinderImpl.h"

#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <ctime>
#include <filesystem>
#include <string>

#include <fmt/format.h>
#include <folly/FileUtil.h>
#include <folly/json.h>
#include <gtest/gtest.h>
#include <thrift/lib/cpp2/protocol/Serializer.h>

using namespace facebook::fboss::platform::reboot_cause_finder;
namespace rcc = facebook::fboss::platform::reboot_cause_config;

namespace {

constexpr int64_t kWindow = 1800;

int64_t nowSec() {
  return static_cast<int64_t>(
      std::chrono::system_clock::to_time_t(std::chrono::system_clock::now()));
}

// Render an epoch instant the way syslog does: local time, no year.
std::string syslogStamp(int64_t epochSec) {
  const auto t = static_cast<std::time_t>(epochSec);
  std::tm tm{};
  localtime_r(&t, &tm);
  char buf[64];
  std::strftime(buf, sizeof(buf), "%b %e %H:%M:%S", &tm);
  return buf;
}

// Render an epoch instant the way kdump names a crash directory.
std::string crashDirName(int64_t epochSec) {
  const auto t = static_cast<std::time_t>(epochSec);
  std::tm tm{};
  localtime_r(&t, &tm);
  char buf[64];
  std::strftime(buf, sizeof(buf), "%Y-%m-%dT%H:%M:%S%Z", &tm);
  return buf;
}

// Sets TZ for the duration of a test. The parsers resolve local time, so a
// test asserting an absolute epoch has to say which zone it means rather
// than inheriting one from the build environment.
class ScopedTz {
 public:
  explicit ScopedTz(const char* tz) : had_(::getenv("TZ") != nullptr) {
    if (had_) {
      saved_ = ::getenv("TZ");
    }
    ::setenv("TZ", tz, 1);
    ::tzset();
  }
  ~ScopedTz() {
    if (had_) {
      ::setenv("TZ", saved_.c_str(), 1);
    } else {
      ::unsetenv("TZ");
    }
    ::tzset();
  }
  ScopedTz(const ScopedTz&) = delete;
  ScopedTz& operator=(const ScopedTz&) = delete;

 private:
  bool had_;
  std::string saved_;
};

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

  std::string writeSecureLog(const std::string& contents) {
    const auto path = (tmpDir_ / "secure").string();
    EXPECT_TRUE(folly::writeFile(contents, path.c_str()));
    return path;
  }

  std::string makeCrashDir(const std::vector<std::string>& entryNames) {
    const auto dir = tmpDir_ / "crash";
    std::filesystem::create_directories(dir);
    for (const auto& name : entryNames) {
      std::filesystem::create_directories(dir / name);
    }
    return dir.string();
  }

  std::string writeProcStat(const std::string& contents) {
    const auto path = (tmpDir_ / "stat").string();
    EXPECT_TRUE(folly::writeFile(contents, path.c_str()));
    return path;
  }

  std::filesystem::path tmpDir_;
};

// ---------------------------------------------------------------- boot time

TEST_F(RebootCauseFinderImplTest, BootTimeParsedFromProcStat) {
  const auto path =
      writeProcStat("cpu  1 2 3 4\nintr 0\nbtime 1781204485\nprocesses 99\n");
  EXPECT_EQ(detail::readBootTimeSec(path), 1781204485);
}

TEST_F(RebootCauseFinderImplTest, BootTimeAbsentWhenFileMissing) {
  EXPECT_FALSE(
      detail::readBootTimeSec((tmpDir_ / "does_not_exist").string())
          .has_value());
}

TEST_F(RebootCauseFinderImplTest, BootTimeAbsentWhenNoBtimeLine) {
  const auto path = writeProcStat("cpu  1 2 3 4\nintr 0\nprocesses 99\n");
  EXPECT_FALSE(detail::readBootTimeSec(path).has_value());
}

// A clock skewed into the future would make the window meaningless, so both
// log-derived readers must be disabled rather than fed a bogus anchor.
TEST_F(RebootCauseFinderImplTest, BootTimeInFutureIsRejected) {
  const auto path = writeProcStat(fmt::format("btime {}\n", nowSec() + 86400));
  EXPECT_FALSE(detail::readBootTimeSec(path).has_value());
}

// ------------------------------------------------------------- kernel panic

TEST_F(RebootCauseFinderImplTest, PanicInsideWindowIsReported) {
  const auto btime = nowSec();
  const auto dir = makeCrashDir({crashDirName(btime - 60)});

  const auto causes = *detail::readKernelPanic({dir}, btime, kWindow).causes();
  ASSERT_EQ(causes.size(), 1);
  EXPECT_EQ(*causes[0].description(), "Kernel Panic");
  // The crash dir name is per-instance data, not part of the cause name.
  ASSERT_TRUE(causes[0].rawValue().has_value());
  EXPECT_EQ(*causes[0].rawValue(), crashDirName(btime - 60));
}

TEST_F(RebootCauseFinderImplTest, PanicOlderThanWindowIsIgnored) {
  const auto btime = nowSec();
  const auto dir = makeCrashDir({crashDirName(btime - kWindow - 60)});
  EXPECT_TRUE(detail::readKernelPanic({dir}, btime, kWindow).causes()->empty());
}

// A dump timestamped at or after btime belongs to the boot we are running in,
// so it cannot be the cause of that boot.
TEST_F(RebootCauseFinderImplTest, PanicAtOrAfterBootStartIsIgnored) {
  const auto btime = nowSec();
  const auto dir =
      makeCrashDir({crashDirName(btime), crashDirName(btime + 60)});
  EXPECT_TRUE(detail::readKernelPanic({dir}, btime, kWindow).causes()->empty());
}

TEST_F(RebootCauseFinderImplTest, PanicNearestBootStartWins) {
  const auto btime = nowSec();
  const auto near = btime - 30;
  const auto dir =
      makeCrashDir({crashDirName(btime - 900), crashDirName(near)});

  const auto causes = *detail::readKernelPanic({dir}, btime, kWindow).causes();
  ASSERT_EQ(causes.size(), 1);
  EXPECT_EQ(*causes[0].occurredAtMs(), static_cast<int64_t>(near) * 1000);
}

// kdump writes the dump during the crash kernel or in post-boot processing,
// so the directory mtime can land after btime while the panic preceded it.
// Selection must use the name, never the mtime.
TEST_F(RebootCauseFinderImplTest, PanicSelectedByNameNotMtime) {
  const auto btime = nowSec();
  const auto inWindowByName = btime - 60;
  const auto outOfWindowByName = btime + 600;
  const auto dir = makeCrashDir(
      {crashDirName(inWindowByName), crashDirName(outOfWindowByName)});

  // Invert the mtimes relative to the names. Selecting on mtime would pick the
  // out-of-window entry and drop the in-window one.
  std::filesystem::last_write_time(
      std::filesystem::path(dir) / crashDirName(inWindowByName),
      std::filesystem::file_time_type::clock::now() + std::chrono::hours(1));
  std::filesystem::last_write_time(
      std::filesystem::path(dir) / crashDirName(outOfWindowByName),
      std::filesystem::file_time_type::clock::now() - std::chrono::hours(1));

  const auto causes = *detail::readKernelPanic({dir}, btime, kWindow).causes();
  ASSERT_EQ(causes.size(), 1);
  EXPECT_EQ(
      *causes[0].occurredAtMs(), static_cast<int64_t>(inWindowByName) * 1000);
}

// A dump is always a directory. A file whose name merely starts with a
// timestamp -- a tarball bundling several dumps, say -- need not denote that
// instant at all, so it must not be read as a panic.
TEST_F(RebootCauseFinderImplTest, TimestampNamedFileIsNotAPanic) {
  const auto btime = nowSec();
  const auto dir = makeCrashDir({});
  const auto stray =
      (std::filesystem::path(dir) / (crashDirName(btime - 60) + ".tar"))
          .string();
  ASSERT_TRUE(folly::writeFile(std::string("x"), stray.c_str()));
  const auto bare =
      (std::filesystem::path(dir) / crashDirName(btime - 60)).string();
  ASSERT_TRUE(folly::writeFile(std::string("x"), bare.c_str()));

  const auto attempt = detail::readKernelPanic({dir}, btime, kWindow);
  EXPECT_TRUE(attempt.causes()->empty());
  EXPECT_EQ(*attempt.status(), rcc::RebootCauseProviderStatus::OK);
}

TEST_F(RebootCauseFinderImplTest, MissingCrashDirIsNotAnError) {
  EXPECT_TRUE(
      detail::readKernelPanic(
          {(tmpDir_ / "no_such_dir").string()}, nowSec(), kWindow)
          .causes()
          ->empty());
}

TEST_F(RebootCauseFinderImplTest, UnparseableCrashDirEntryIsSkipped) {
  const auto btime = nowSec();
  const auto dir = makeCrashDir({"not-a-timestamp", "README"});
  EXPECT_TRUE(detail::readKernelPanic({dir}, btime, kWindow).causes()->empty());
}

// ------------------------------------------------------------ manual reboot

TEST_F(RebootCauseFinderImplTest, ManualRebootInsideWindowIsReported) {
  const auto btime = nowSec();
  const auto path = writeSecureLog(
      fmt::format(
          "{} sw systemd-logind[1]: System is rebooting.\n",
          syslogStamp(btime - 60)));

  const auto causes =
      *detail::readManualReboot({path}, btime, kWindow).causes();
  ASSERT_EQ(causes.size(), 1);
  EXPECT_EQ(*causes[0].description(), "Manual x86 Reboot");
}

TEST_F(RebootCauseFinderImplTest, ManualRebootOlderThanWindowIsIgnored) {
  const auto btime = nowSec();
  const auto path = writeSecureLog(
      fmt::format(
          "{} sw systemd-logind[1]: System is rebooting.\n",
          syslogStamp(btime - kWindow - 60)));
  EXPECT_TRUE(
      detail::readManualReboot({path}, btime, kWindow).causes()->empty());
}

// The line is written before the machine goes down, so a line at or after
// btime is a reboot being requested now -- the cause of the *next* boot.
TEST_F(RebootCauseFinderImplTest, ManualRebootAfterBootStartIsIgnored) {
  const auto btime = nowSec() - 300;
  const auto path = writeSecureLog(
      fmt::format(
          "{} sw systemd-logind[1]: System is rebooting.\n",
          syslogStamp(btime + 60)));
  EXPECT_TRUE(
      detail::readManualReboot({path}, btime, kWindow).causes()->empty());
}

TEST_F(RebootCauseFinderImplTest, ManualRebootExactlyAtBootStartIsIgnored) {
  const auto btime = nowSec() - 300;
  const auto path = writeSecureLog(
      fmt::format(
          "{} sw systemd-logind[1]: System is rebooting.\n",
          syslogStamp(btime)));
  EXPECT_TRUE(
      detail::readManualReboot({path}, btime, kWindow).causes()->empty());
}

TEST_F(RebootCauseFinderImplTest, ManualRebootNearestBootStartWins) {
  const auto btime = nowSec();
  const auto near = btime - 30;
  const auto path = writeSecureLog(
      fmt::format(
          "{} sw systemd-logind[1]: System is rebooting.\n"
          "{} sw systemd-logind[1]: System is rebooting.\n",
          syslogStamp(btime - 900),
          syslogStamp(near)));

  const auto causes =
      *detail::readManualReboot({path}, btime, kWindow).causes();
  ASSERT_EQ(causes.size(), 1);
  EXPECT_EQ(*causes[0].occurredAtMs(), static_cast<int64_t>(near) * 1000);
}

TEST_F(RebootCauseFinderImplTest, NonMatchingLinesAreIgnored) {
  const auto btime = nowSec();
  const auto path = writeSecureLog(
      fmt::format(
          "{} sw sshd: Accepted publickey for netops\n"
          "{} sw sudo: netops : TTY=pts/0 ; COMMAND=/bin/ls\n",
          syslogStamp(btime - 60),
          syslogStamp(btime - 50)));
  EXPECT_TRUE(
      detail::readManualReboot({path}, btime, kWindow).causes()->empty());
}

TEST_F(RebootCauseFinderImplTest, MissingSecureLogIsNotAnError) {
  EXPECT_TRUE(
      detail::readManualReboot(
          {(tmpDir_ / "no_such_file").string()}, nowSec(), kWindow)
          .causes()
          ->empty());
}

// Syslog carries no year. Anchoring on btime rather than on the current time
// is what keeps a December line correct when it is read in January.
TEST_F(
    RebootCauseFinderImplTest,
    DecemberLineReadInJanuaryResolvesToPriorYear) {
  // btime: 2027-01-01 00:10:00 local. Event: Dec 31 23:55:00, 15 min earlier.
  std::tm bootTm{};
  bootTm.tm_year = 127; // 2027
  bootTm.tm_mon = 0;
  bootTm.tm_mday = 1;
  bootTm.tm_hour = 0;
  bootTm.tm_min = 10;
  bootTm.tm_isdst = -1;
  const auto btime = static_cast<int64_t>(std::mktime(&bootTm));
  ASSERT_NE(btime, -1);

  const auto path = writeSecureLog(
      "Dec 31 23:55:00 sw systemd-logind[1]: System is rebooting.\n");

  const auto causes =
      *detail::readManualReboot({path}, btime, kWindow).causes();
  ASSERT_EQ(causes.size(), 1);
  // Must land 15 minutes before boot, not ~a year after it.
  EXPECT_EQ(*causes[0].occurredAtMs(), (btime - 900) * 1000);
}

// --------------------------------------------- golden inputs from the real
// producers. These are literal strings, not round-tripped through the same
// strftime the implementation parses with, so a shared wrong assumption about
// the format cannot cancel out. TZ is pinned by the BUCK target.

TEST_F(RebootCauseFinderImplTest, GoldenCrashDirNameWithDaylightZone) {
  // 2026-09-11 16:46:31 PDT == 1789170391 (cross-checked against the record
  // written on fboss332654848.ash7 in P2500823427). The epoch is absolute,
  // so the zone is part of the assertion.
  const ScopedTz tz("America/Los_Angeles");
  const auto when = detail::parseCrashDirName("2026-09-11T16:46:31PDT");
  ASSERT_TRUE(when.has_value());
  EXPECT_EQ(static_cast<int64_t>(*when), 1789170391);
}

TEST_F(RebootCauseFinderImplTest, GoldenCrashDirNameWithStandardZone) {
  // A winter, standard-time producer string. The expectation below builds the
  // same instant by hand with tm_isdst = 0, which is only true in a zone whose
  // January is standard time.
  const ScopedTz tz("America/Los_Angeles");
  const auto when = detail::parseCrashDirName("2026-01-15T01:15:29PST");
  ASSERT_TRUE(when.has_value());
  std::tm tm{};
  tm.tm_year = 126;
  tm.tm_mon = 0;
  tm.tm_mday = 15;
  tm.tm_hour = 1;
  tm.tm_min = 15;
  tm.tm_sec = 29;
  tm.tm_isdst = 0;
  EXPECT_EQ(*when, std::mktime(&tm));
}

// The name is produced by localtime() on this host and read back by
// mktime() on the same host, so the round trip is exact. The zone text is
// not interpreted: two names differing only in that text denote the same
// wall clock and resolve identically.
TEST_F(RebootCauseFinderImplTest, ZoneTextIsNotInterpreted) {
  const auto pdt = detail::parseCrashDirName("2026-11-01T01:30:00PDT");
  const auto pst = detail::parseCrashDirName("2026-11-01T01:30:00PST");
  ASSERT_TRUE(pdt.has_value());
  ASSERT_TRUE(pst.has_value());
  EXPECT_EQ(*pdt, *pst);
}

// Round trip against the producer: render an instant the way kdump does,
// parse it back, and require the original instant.
TEST_F(RebootCauseFinderImplTest, RoundTripsAgainstStrftime) {
  for (const int64_t t : {1789170391L, 1800000000L, nowSec()}) {
    const auto tt = static_cast<std::time_t>(t);
    std::tm tm{};
    localtime_r(&tt, &tm);
    char buf[64];
    std::strftime(buf, sizeof(buf), "%Y-%m-%dT%H:%M:%S%Z", &tm);
    const auto parsed = detail::parseCrashDirName(buf);
    ASSERT_TRUE(parsed.has_value()) << buf;
    EXPECT_EQ(static_cast<int64_t>(*parsed), t) << buf;
  }
}

// The round trip must hold in any zone, not just the pinned one. Europe is
// the case that matters: CEST and BST are *summer* times whose abbreviation
// ends in "ST", so any attempt to infer daylight from the suffix misdates
// every summer instant there by an hour. Pinning TZ hides that, so this test
// changes it deliberately.
TEST_F(RebootCauseFinderImplTest, RoundTripsInZonesWhereSuffixMisleads) {
  for (const char* tz : {"Europe/Berlin", "Europe/London", "Asia/Tokyo"}) {
    const ScopedTz scoped(tz);
    const auto tt = static_cast<std::time_t>(1789170391); // a summer instant
    std::tm tm{};
    localtime_r(&tt, &tm);
    char buf[64];
    std::strftime(buf, sizeof(buf), "%Y-%m-%dT%H:%M:%S%Z", &tm);
    const auto parsed = detail::parseCrashDirName(buf);
    EXPECT_TRUE(parsed.has_value()) << tz << " " << buf;
    if (parsed) {
      EXPECT_EQ(static_cast<int64_t>(*parsed), 1789170391) << tz << " " << buf;
    }
  }
}

// The hyphen-separated form, with no zone.
TEST_F(RebootCauseFinderImplTest, GoldenCrashDirNameKdumpHyphenForm) {
  EXPECT_TRUE(detail::parseCrashDirName("2026-09-11-16:46:31").has_value());
}

TEST_F(RebootCauseFinderImplTest, CrashDirNameGarbageRejected) {
  EXPECT_FALSE(detail::parseCrashDirName("README").has_value());
  EXPECT_FALSE(detail::parseCrashDirName("not-a-timestamp").has_value());
  EXPECT_FALSE(detail::parseCrashDirName("").has_value());
}

// The %Z text is accepted and ignored, in every form a producer emits. The
// remainder is deliberately not validated -- see the diff summary -- so this
// pins what parses, not what is rejected.
TEST_F(RebootCauseFinderImplTest, CrashDirNameZoneSuffixesAccepted) {
  for (const auto* name : {
           "2026-09-24T16:32:00", // no zone at all
           "2026-09-24T16:32:00PDT",
           "2026-09-24T16:32:00PST",
           "2026-09-24T16:32:00UTC", // lost to a DT/ST-only rule
           "2026-09-24T16:32:00GMT",
           "2026-09-24T16:32:00CEST", // four letters
           "2026-09-24T16:32:00AEDT",
           "2026-09-24T16:32:00+06", // zones with no abbreviation
           "2026-09-24T16:32:00+0530",
       }) {
    EXPECT_TRUE(detail::parseCrashDirName(name).has_value()) << name;
  }
}

// Real systemd-logind line, space-padded single-digit day.
TEST_F(RebootCauseFinderImplTest, GoldenSyslogLineSpacePaddedDay) {
  std::tm tm{};
  tm.tm_year = 126;
  tm.tm_mon = 8;
  tm.tm_mday = 3;
  tm.tm_hour = 1;
  tm.tm_min = 2;
  tm.tm_sec = 13;
  tm.tm_isdst = -1;
  const auto eventT = std::mktime(&tm);
  ASSERT_NE(eventT, -1);
  const auto btime = static_cast<int64_t>(eventT) + 120;

  const auto path = writeSecureLog(
      "Sep  3 01:02:03 sw systemd-logind[1234]: The system will reboot now!\n"
      "Sep  3 01:02:13 sw systemd-logind[1234]: System is rebooting.\n");

  const auto causes =
      *detail::readManualReboot({path}, btime, kWindow).causes();
  ASSERT_EQ(causes.size(), 1);
  EXPECT_EQ(*causes[0].occurredAtMs(), static_cast<int64_t>(eventT) * 1000);
}

// On NetOS the line lands in /var/log/messages, not /var/log/secure. Reading
// only secure was a real false negative on minipack3n.
TEST_F(RebootCauseFinderImplTest, ManualRebootFoundInSecondLogPath) {
  const auto btime = nowSec();
  const auto messages = (tmpDir_ / "messages").string();
  ASSERT_TRUE(
      folly::writeFile(
          fmt::format(
              "{} sw systemd-logind[1]: System is rebooting.\n",
              syslogStamp(btime - 60)),
          messages.c_str()));
  const auto secure = writeSecureLog("Sep  3 01:02:03 sw sudo: nothing here\n");

  const auto causes =
      *detail::readManualReboot({secure, messages}, btime, kWindow).causes();
  ASSERT_EQ(causes.size(), 1);
}

// A line after btime must fail outright, not resolve to the prior year. The
// prior-year candidate is always <= btime, so without a plausibility bound it
// silently returns a timestamp roughly a year old.
TEST_F(RebootCauseFinderImplTest, PostBootLineDoesNotResolveToPriorYear) {
  std::tm tm{};
  tm.tm_year = 126;
  tm.tm_mon = 5;
  tm.tm_mday = 15;
  tm.tm_hour = 12;
  tm.tm_min = 0;
  tm.tm_sec = 0;
  tm.tm_isdst = -1;
  const auto btime = static_cast<int64_t>(std::mktime(&tm));

  // Line one hour AFTER btime.
  const auto path = writeSecureLog(
      "Jun 15 13:00:00 sw systemd-logind[1]: System is rebooting.\n");
  EXPECT_TRUE(
      detail::readManualReboot({path}, btime, kWindow).causes()->empty());
}

// ------------------------------------------- absent source vs unreadable one

// The whole point of RebootCauseProviderStatus is to tell "read cleanly and
// found nothing" apart from "could not read the source". An absent source is
// the former: most switches have never panicked, and no image uses every log
// path. Both readers must leave status OK for it.
TEST_F(RebootCauseFinderImplTest, AbsentSourcesAreNotAReadFailure) {
  const auto panic =
      detail::readKernelPanic({(tmpDir_ / "nope").string()}, nowSec(), kWindow);
  EXPECT_TRUE(panic.causes()->empty());
  EXPECT_EQ(*panic.status(), rcc::RebootCauseProviderStatus::OK);

  const auto manual = detail::readManualReboot(
      {(tmpDir_ / "nope").string()}, nowSec(), kWindow);
  EXPECT_TRUE(manual.causes()->empty());
  EXPECT_EQ(*manual.status(), rcc::RebootCauseProviderStatus::OK);
}

// A crash "dir" that is really a regular file exists but cannot be iterated.
// Using ENOTDIR rather than chmod keeps the test honest when it runs as root,
// where a 0000 mode is still readable.
TEST_F(RebootCauseFinderImplTest, UnreadableCrashDirIsReadFailure) {
  const auto notADir = (tmpDir_ / "crash_is_a_file").string();
  ASSERT_TRUE(folly::writeFile(std::string("x"), notADir.c_str()));

  const auto attempt = detail::readKernelPanic({notADir}, nowSec(), kWindow);
  EXPECT_TRUE(attempt.causes()->empty());
  EXPECT_EQ(*attempt.status(), rcc::RebootCauseProviderStatus::READ_FAILED);
}

// Likewise a log "file" that is really a directory: present, but EISDIR.
TEST_F(RebootCauseFinderImplTest, UnreadableLogPathIsReadFailure) {
  const auto notAFile = (tmpDir_ / "secure_is_a_dir").string();
  std::filesystem::create_directories(notAFile);

  const auto attempt = detail::readManualReboot({notAFile}, nowSec(), kWindow);
  EXPECT_TRUE(attempt.causes()->empty());
  EXPECT_EQ(*attempt.status(), rcc::RebootCauseProviderStatus::READ_FAILED);
}

// A path that cannot even be stat'ed is distinct from one that stats fine but
// will not open. A symlink loop gives ELOOP from exists() itself, and unlike
// chmod it behaves the same when the test runs as root.
TEST_F(RebootCauseFinderImplTest, UnstatableLogPathIsReadFailure) {
  const auto a = tmpDir_ / "loop_a";
  const auto b = tmpDir_ / "loop_b";
  std::error_code ec;
  std::filesystem::create_symlink(b, a, ec);
  ASSERT_FALSE(ec);
  std::filesystem::create_symlink(a, b, ec);
  ASSERT_FALSE(ec);

  const auto attempt =
      detail::readManualReboot({a.string()}, nowSec(), kWindow);
  EXPECT_TRUE(attempt.causes()->empty());
  EXPECT_EQ(*attempt.status(), rcc::RebootCauseProviderStatus::READ_FAILED);
}

// One bad source must not discard a cause found in a good one: the attempt is
// both READ_FAILED and carries the cause.
TEST_F(RebootCauseFinderImplTest, FailedSourceStillReportsCauseFromGoodOne) {
  const auto btime = nowSec();
  const auto good = makeCrashDir({crashDirName(btime - 60)});
  const auto bad = (tmpDir_ / "crash_is_a_file").string();
  ASSERT_TRUE(folly::writeFile(std::string("x"), bad.c_str()));

  const auto attempt = detail::readKernelPanic({bad, good}, btime, kWindow);
  ASSERT_EQ(attempt.causes()->size(), 1);
  EXPECT_EQ(*attempt.causes()->front().rawValue(), crashDirName(btime - 60));
  EXPECT_EQ(*attempt.status(), rcc::RebootCauseProviderStatus::READ_FAILED);
}

// ------------------------------------------------- where the dump lives

// A dump can sit in either directory, so searching only one loses it.
TEST_F(RebootCauseFinderImplTest, PanicFoundInUnprocessedCrashDir) {
  const auto btime = nowSec();
  const auto raw = (tmpDir_ / "crash").string();
  std::filesystem::create_directories(
      std::filesystem::path(raw) / crashDirName(btime - 60));
  const auto processed = (tmpDir_ / "crash" / "processed").string();
  std::filesystem::create_directories(processed);

  const auto causes =
      *detail::readKernelPanic({raw, processed}, btime, kWindow).causes();
  ASSERT_EQ(causes.size(), 1);
  EXPECT_EQ(*causes[0].rawValue(), crashDirName(btime - 60));
}

// A dump in the processed directory is still found. Scanning the parent
// also turns up the processed dir itself; kProcessedDirName keeps that from
// logging a spurious parse error, which is log noise only and so is not
// asserted here.
TEST_F(RebootCauseFinderImplTest, PanicFoundInProcessedCrashDir) {
  const auto btime = nowSec();
  const auto raw = (tmpDir_ / "crash").string();
  const auto processed = (tmpDir_ / "crash" / "processed").string();
  std::filesystem::create_directories(
      std::filesystem::path(processed) / crashDirName(btime - 60));

  const auto causes =
      *detail::readKernelPanic({raw, processed}, btime, kWindow).causes();
  ASSERT_EQ(causes.size(), 1);
  EXPECT_EQ(*causes[0].rawValue(), crashDirName(btime - 60));
}

// A dump caught mid-move exists in both places; it is one panic, not two,
// and the reported instant must not depend on scan order.
TEST_F(RebootCauseFinderImplTest, SameDumpInBothDirsReportedOnce) {
  const auto btime = nowSec();
  const auto raw = (tmpDir_ / "crash").string();
  const auto processed = (tmpDir_ / "crash" / "processed").string();
  std::filesystem::create_directories(
      std::filesystem::path(raw) / crashDirName(btime - 60));
  std::filesystem::create_directories(
      std::filesystem::path(processed) / crashDirName(btime - 60));

  const auto causes =
      *detail::readKernelPanic({raw, processed}, btime, kWindow).causes();
  ASSERT_EQ(causes.size(), 1);
  EXPECT_EQ(*causes[0].occurredAtMs(), (btime - 60) * 1000);
}

// Nearest-to-btime must win across directories, not just within one.
TEST_F(RebootCauseFinderImplTest, NearestPanicWinsAcrossCrashDirs) {
  const auto btime = nowSec();
  const auto raw = (tmpDir_ / "crash").string();
  const auto processed = (tmpDir_ / "crash" / "processed").string();
  std::filesystem::create_directories(
      std::filesystem::path(raw) / crashDirName(btime - 30));
  std::filesystem::create_directories(
      std::filesystem::path(processed) / crashDirName(btime - 900));

  const auto causes =
      *detail::readKernelPanic({raw, processed}, btime, kWindow).causes();
  ASSERT_EQ(causes.size(), 1);
  EXPECT_EQ(*causes[0].occurredAtMs(), (btime - 30) * 1000);
}

TEST_F(RebootCauseFinderImplTest, CrashDirsCoverBothLocations) {
  const std::vector<std::string> expected{"/var/crash", "/var/crash/processed"};
  EXPECT_EQ(detail::kernelPanicCrashDirs(), expected);
}

// ------------------------------------------------- who wrote the syslog line

// sshd records every remote command verbatim into the same files this reader
// searches, so anyone grepping for the phrase plants a line containing it.
// Both fixtures below are real lines captured from /var/log/secure on
// fboss329039409.snc1 after a probe ran `grep -c "System is rebooting"` over
// ssh. They sit inside the window, so only the program check rejects them.
TEST_F(RebootCauseFinderImplTest, SshdEchoOfThePhraseIsNotAReboot) {
  const auto btime = nowSec();
  const auto stamp = syslogStamp(btime - 60);
  const auto path = writeSecureLog(
      fmt::format(
          "{} sw sshd[1627813]: Exec Request for user root with command "
          "BT=$(awk \"/^btime/{{print \\$2}}\" /proc/stat); "
          "echo \"RB=$(grep -c \"System is rebooting\" /var/log/messages)\"\n"
          "{} sw sshd[1627813]: sshd_auth_msg: {{\"user\": \"root\", "
          "\"command\": \"grep -c \\\"System is rebooting\\\" "
          "/var/log/messages\"}}\n",
          stamp,
          stamp));

  EXPECT_TRUE(
      detail::readManualReboot({path}, btime, kWindow).causes()->empty());
}

// The same file can hold both. The announcement must still be found.
TEST_F(RebootCauseFinderImplTest, AnnouncementFoundAlongsideSshdEcho) {
  const auto btime = nowSec();
  const auto path = writeSecureLog(
      fmt::format(
          "{} sw sshd[99]: Exec Request for user root with command "
          "grep -c \"System is rebooting\" /var/log/messages\n"
          "{} sw systemd-logind[1]: System is rebooting.\n",
          syslogStamp(btime - 120),
          syslogStamp(btime - 60)));

  const auto causes =
      *detail::readManualReboot({path}, btime, kWindow).causes();
  ASSERT_EQ(causes.size(), 1);
  EXPECT_EQ(*causes[0].occurredAtMs(), (btime - 60) * 1000);
}

TEST_F(RebootCauseFinderImplTest, ManualRebootLineAcceptsTrustedEmitters) {
  EXPECT_TRUE(
      detail::isManualRebootLine(
          "Sep  3 01:02:13 sw systemd-logind[1234]: System is rebooting."));
  EXPECT_TRUE(
      detail::isManualRebootLine(
          "Sep  3 01:02:13 sw systemd-logind: System is rebooting."));
  EXPECT_TRUE(
      detail::isManualRebootLine(
          "Sep 13 01:02:13 sw systemd[1]: System is rebooting."));
}

TEST_F(RebootCauseFinderImplTest, ManualRebootLineRejectsOtherEmitters) {
  EXPECT_FALSE(
      detail::isManualRebootLine(
          "Sep  3 01:02:13 sw sshd[99]: System is rebooting."));
  EXPECT_FALSE(
      detail::isManualRebootLine(
          "Sep  3 01:02:13 sw sudo[99]: System is rebooting."));
  // A program whose name merely ends in the trusted one.
  EXPECT_FALSE(
      detail::isManualRebootLine(
          "Sep  3 01:02:13 sw not-systemd[1]: System is rebooting."));
}

// The phrase has to begin the message, so a line quoting it mid-sentence
// under a trusted program name still does not count.
TEST_F(
    RebootCauseFinderImplTest,
    ManualRebootLineRequiresPhraseAtMessageStart) {
  EXPECT_FALSE(
      detail::isManualRebootLine(
          "Sep  3 01:02:13 sw systemd[1]: checking whether System is rebooting."));
}

TEST_F(RebootCauseFinderImplTest, ManualRebootLineRejectsMalformedLines) {
  EXPECT_FALSE(detail::isManualRebootLine(""));
  EXPECT_FALSE(detail::isManualRebootLine("System is rebooting."));
  EXPECT_FALSE(detail::isManualRebootLine("Sep  3 01:02:13 sw systemd[1]"));
}

// ----------------------------------------------------------- exact boundaries

TEST_F(RebootCauseFinderImplTest, PanicExactlyAtWindowLowerBoundIsIncluded) {
  const auto btime = nowSec();
  const auto dir = makeCrashDir({crashDirName(btime - kWindow)});
  EXPECT_EQ(detail::readKernelPanic({dir}, btime, kWindow).causes()->size(), 1);
}

TEST_F(RebootCauseFinderImplTest, PanicOneSecondBeforeWindowIsExcluded) {
  const auto btime = nowSec();
  const auto dir = makeCrashDir({crashDirName(btime - kWindow - 1)});
  EXPECT_TRUE(detail::readKernelPanic({dir}, btime, kWindow).causes()->empty());
}

TEST_F(RebootCauseFinderImplTest, BootTimeExactlyNowIsAccepted) {
  const auto now = nowSec();
  const auto path = writeProcStat(fmt::format("btime {}\n", now));
  EXPECT_TRUE(detail::readBootTimeSec(path).has_value());
}

TEST_F(RebootCauseFinderImplTest, BootTimeOneSecondInFutureIsRejected) {
  const auto path = writeProcStat(fmt::format("btime {}\n", nowSec() + 60));
  EXPECT_FALSE(detail::readBootTimeSec(path).has_value());
}

TEST_F(RebootCauseFinderImplTest, MalformedBtimeLineIsRejected) {
  EXPECT_FALSE(
      detail::readBootTimeSec(writeProcStat("btime notanumber\n")).has_value());
  EXPECT_FALSE(detail::readBootTimeSec(writeProcStat("btime\n")).has_value());
  EXPECT_FALSE(
      detail::readBootTimeSec(writeProcStat("xbtime 12345\n")).has_value());
}

// ------------------------------------------------ precedence and year bound

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

// A panic and a later operator reboot can both land inside one window. The
// reboot is then the cause; the panic belongs to the boot before it. Ordering
// must come from the timestamps, not from which reader ran first.
TEST_F(RebootCauseFinderImplTest, NearestToBootWinsRegardlessOfListOrder) {
  const int64_t btime = 1789170391;
  const auto panic = causeAt("Kernel Panic", btime - 900);
  const auto manual = causeAt("Manual x86 Reboot", btime - 30);

  auto panicFirst = detail::selectNearestToBoot(
      {attemptWith("KernelPanic", {panic}),
       attemptWith("ManualReboot", {manual})});
  ASSERT_TRUE(panicFirst.has_value());
  EXPECT_EQ(*panicFirst->providerName(), "ManualReboot");

  auto manualFirst = detail::selectNearestToBoot(
      {attemptWith("ManualReboot", {manual}),
       attemptWith("KernelPanic", {panic})});
  ASSERT_TRUE(manualFirst.has_value());
  EXPECT_EQ(*manualFirst->providerName(), "ManualReboot");
}

TEST_F(RebootCauseFinderImplTest, NearestToBootPicksPanicWhenItIsNearer) {
  const int64_t btime = 1789170391;
  auto best = detail::selectNearestToBoot(
      {attemptWith("ManualReboot", {causeAt("Manual x86 Reboot", btime - 900)}),
       attemptWith("KernelPanic", {causeAt("Kernel Panic", btime - 30)})});
  ASSERT_TRUE(best.has_value());
  EXPECT_EQ(*best->providerName(), "KernelPanic");
}

// Reading only /var/log/secure missed a real graceful reboot on minipack3n,
// where systemd-logind logs to /var/log/messages. Lock both paths in.
TEST_F(RebootCauseFinderImplTest, ManualRebootSearchesMessagesAndSecure) {
  const auto& paths = detail::manualRebootLogPaths();
  EXPECT_NE(
      std::find(paths.begin(), paths.end(), "/var/log/messages"), paths.end());
  EXPECT_NE(
      std::find(paths.begin(), paths.end(), "/var/log/secure"), paths.end());
}

TEST_F(RebootCauseFinderImplTest, NearestToBootEmptyListYieldsNothing) {
  EXPECT_FALSE(detail::selectNearestToBoot({}).has_value());
}

// The prior-year candidate is always <= btime, so without a plausibility
// bound a post-boot line resolves to a timestamp about a year old instead of
// failing. Asserted directly on the parser, since the window check downstream
// would mask it.
TEST_F(RebootCauseFinderImplTest, SyslogPostBootLineYieldsNullopt) {
  std::tm tm{};
  tm.tm_year = 126;
  tm.tm_mon = 5;
  tm.tm_mday = 15;
  tm.tm_hour = 12;
  tm.tm_isdst = -1;
  const auto btime = static_cast<int64_t>(std::mktime(&tm));

  EXPECT_FALSE(
      detail::parseSyslogTimestamp("Jun 15 13:00:00 sw x: y", btime)
          .has_value());
}

TEST_F(RebootCauseFinderImplTest, SyslogDecemberLineReadInJanuaryResolves) {
  std::tm tm{};
  tm.tm_year = 127;
  tm.tm_mon = 0;
  tm.tm_mday = 1;
  tm.tm_min = 10;
  tm.tm_isdst = -1;
  const auto btime = static_cast<int64_t>(std::mktime(&tm));

  const auto when =
      detail::parseSyslogTimestamp("Dec 31 23:55:00 sw x: y", btime);
  ASSERT_TRUE(when.has_value());
  EXPECT_EQ(static_cast<int64_t>(*when), btime - 900);
}

// ------------------------------------------------ determineRebootCause

// Drives the whole of determineRebootCause() against a temp tree, which is
// where the "never fabricate a cause" rule actually lives. Asserting on the
// persisted record rather than on hand-built thrift structs means these fail
// if the arbitration changes, not only if the serializer does.
class DetermineRebootCauseTest : public RebootCauseFinderImplTest {
 protected:
  void SetUp() override {
    RebootCauseFinderImplTest::SetUp();
    historyDir_ = (tmpDir_ / "history").string();
    procStat_ = (tmpDir_ / "stat").string();
    bootIdPath_ = (tmpDir_ / "boot_id").string();
    crashDir_ = (tmpDir_ / "crash").string();
    logPath_ = (tmpDir_ / "messages").string();
    std::filesystem::create_directories(crashDir_);
    btime_ = nowSec();
    ASSERT_TRUE(
        folly::writeFile(
            fmt::format("cpu 1 2 3\nbtime {}\n", btime_), procStat_.c_str()));
    ASSERT_TRUE(
        folly::writeFile(
            std::string("11111111-2222-3333-4444-555555555555\n"),
            bootIdPath_.c_str()));
  }

  RebootCauseFinderImpl::Paths paths() const {
    return RebootCauseFinderImpl::Paths{
        historyDir_, procStat_, bootIdPath_, {crashDir_}, {logPath_}};
  }

  // The single record determineRebootCause() persisted, as parsed JSON.
  folly::dynamic readRecord() const {
    std::vector<std::string> files;
    for (const auto& e : std::filesystem::directory_iterator(historyDir_)) {
      files.push_back(e.path().string());
    }
    EXPECT_EQ(files.size(), 1);
    std::string contents;
    EXPECT_TRUE(folly::readFile(files.front().c_str(), contents));
    return folly::parseJson(contents);
  }

  rcc::RebootCauseConfig configWithProvider(const std::string& readPath) {
    rcc::RebootCauseProviderConfig pc;
    pc.name() = "TEST_CPLD";
    pc.priority() = 1;
    pc.sysfsReadPath() = readPath;
    pc.sysfsClearPath() = readPath + ".clear";
    rcc::RebootCauseConfig c;
    c.rebootCauseProviderConfigs() = {pc};
    return c;
  }

  int64_t btime_{};
  std::string historyDir_, procStat_, bootIdPath_, crashDir_, logPath_;
};

// The headline rule: nothing found means nothing claimed.
TEST_F(DetermineRebootCauseTest, NoProviderReportsAnythingSoNoCauseIsClaimed) {
  RebootCauseFinderImpl(rcc::RebootCauseConfig{}, paths())
      .determineRebootCause();

  const auto rec = readRecord();
  EXPECT_EQ(rec.count("determinedCause"), 0);
  EXPECT_EQ(rec["bootTimeMs"].asInt(), btime_ * 1000);
  ASSERT_EQ(rec["providersAttempted"].size(), 2);
  for (const auto& a : rec["providersAttempted"]) {
    EXPECT_EQ(a["status"].asInt(), 0) << "absent source is OK, not a failure";
    EXPECT_EQ(a["causes"].size(), 0);
  }
}

TEST_F(DetermineRebootCauseTest, PanicInWindowBecomesTheDeterminedCause) {
  std::filesystem::create_directories(
      std::filesystem::path(crashDir_) / crashDirName(btime_ - 60));

  RebootCauseFinderImpl(rcc::RebootCauseConfig{}, paths())
      .determineRebootCause();

  const auto rec = readRecord();
  ASSERT_EQ(rec.count("determinedCause"), 1);
  EXPECT_EQ(rec["determinedCause"]["providerName"].asString(), "KernelPanic");
  EXPECT_EQ(
      rec["determinedCause"]["cause"]["description"].asString(),
      "Kernel Panic");
}

// A provider that cannot be read is recorded as READ_FAILED and claims
// nothing, rather than being indistinguishable from one that found nothing.
TEST_F(DetermineRebootCauseTest, UnreadableProviderIsRecordedNotClaimed) {
  RebootCauseFinderImpl(
      configWithProvider((tmpDir_ / "absent").string()), paths())
      .determineRebootCause();

  const auto rec = readRecord();
  EXPECT_EQ(rec.count("determinedCause"), 0);
  ASSERT_EQ(rec["providersAttempted"].size(), 3);
  const auto& hw = rec["providersAttempted"][2];
  EXPECT_EQ(hw["name"].asString(), "TEST_CPLD");
  EXPECT_EQ(hw["status"].asInt(), 1);
}

// A provider file that parses partway must not have its half-read cause
// promoted: the attempt is PARSE_FAILED, so it reports nothing at all.
TEST_F(DetermineRebootCauseTest, PartiallyParsedProviderKeepsWholeCauses) {
  const auto path = (tmpDir_ / "causes").string();
  ASSERT_TRUE(
      folly::writeFile(
          std::string(
              R"({"causes":[{"description":"Power loss","date":)"
              R"("09-24-2026 16:32:00"},{"no_description":1}]})"),
          path.c_str()));

  RebootCauseFinderImpl(configWithProvider(path), paths())
      .determineRebootCause();

  const auto rec = readRecord();
  const auto& hw = rec["providersAttempted"][2];
  EXPECT_EQ(hw["status"].asInt(), 2) << "PARSE_FAILED";
  ASSERT_EQ(hw["causes"].size(), 1) << "the whole cause must be kept";
  EXPECT_EQ(hw["causes"][0]["description"].asString(), "Power loss");
  ASSERT_EQ(rec.count("determinedCause"), 1) << "a whole cause must promote";
  EXPECT_EQ(
      rec["determinedCause"]["cause"]["description"].asString(), "Power loss");
}

// The guard is the record itself: a second run in the same boot is a no-op.
TEST_F(DetermineRebootCauseTest, SecondRunInTheSameBootDoesNothing) {
  RebootCauseFinderImpl(rcc::RebootCauseConfig{}, paths())
      .determineRebootCause();
  const auto first = readRecord()["detectedAtMs"].asInt();

  RebootCauseFinderImpl(rcc::RebootCauseConfig{}, paths())
      .determineRebootCause();
  EXPECT_EQ(readRecord()["detectedAtMs"].asInt(), first);
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
