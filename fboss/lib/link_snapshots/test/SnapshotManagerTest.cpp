/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/lib/link_snapshots/SnapshotManager.h"
#include <folly/logging/LogConfigParser.h>
#include <folly/logging/LoggerDB.h>
#include <folly/logging/test/TestLogHandler.h>
#include <gtest/gtest.h>
#include "fboss/lib/link_snapshots/RingBuffer-defs.h"

namespace facebook::fboss {

namespace {
constexpr auto kSnapshotLogMarker = "Collected snapshot for ports";
} // namespace

TEST(RingBufferTest, WriteReturnsReferenceToStoredElement) {
  RingBuffer<int> buf(3);
  auto& stored = buf.write(42);
  // Mutating through the returned reference must change the buffered element,
  // proving write() hands back the element inside the buffer, not a copy.
  stored = 100;
  EXPECT_EQ(buf.last(), 100);
}

// SnapshotWrapper::publish() emits an XLOG(DBG2) line (gated by the same
// !published_ check as the file write), so counting captured log messages
// tells us exactly how many times a snapshot was logged.
class SnapshotManagerLoggingTest : public ::testing::Test {
 public:
  SnapshotManagerLoggingTest() {
    // XLOG macros use the global LoggerDB singleton; reset it to a known state
    // per test and capture DBG2 messages via a handler on the root category.
    folly::LoggerDB::get().resetConfig(
        folly::parseLogConfig(".=WARN:default; default=stream:stream=stderr"));
    folly::LoggerDB::get().getCategory("")->addHandler(handler_);
    folly::LoggerDB::get().setLevel("", folly::LogLevel::DBG2);
  }

 protected:
  size_t snapshotLogCount() const {
    size_t count = 0;
    for (const auto& message : handler_->getMessageValues()) {
      if (message.find(kSnapshotLogMarker) != std::string::npos) {
        ++count;
      }
    }
    return count;
  }

  const std::set<std::string> portNames_{"eth1/1/1"};
  std::shared_ptr<folly::TestLogHandler> handler_{
      std::make_shared<folly::TestLogHandler>()};
};

// (1) Snapshots get logged: a scheduled future snapshot is logged exactly once.
TEST_F(SnapshotManagerLoggingTest, FuturePublishLogsSnapshotOnce) {
  SnapshotManager mgr(portNames_, SnapshotLogSource::SNAPSHOT_AGENT, 1);
  mgr.publishFutureSnapshots(1);

  mgr.addSnapshot(phy::LinkSnapshot{});

  EXPECT_EQ(snapshotLogCount(), 1);
}

// (2) Dumping the same buffer repeatedly logs each snapshot only once.
TEST_F(SnapshotManagerLoggingTest, RepeatedBufferDumpLogsSnapshotOnce) {
  SnapshotManager mgr(portNames_, SnapshotLogSource::SNAPSHOT_AGENT, 1);
  mgr.addSnapshot(phy::LinkSnapshot{});
  // Not scheduled for a future publish, so nothing is logged on add.
  EXPECT_EQ(snapshotLogCount(), 0);

  mgr.publishAllSnapshots();
  EXPECT_EQ(snapshotLogCount(), 1);

  // Same buffer dumped again -> must not re-log the already-published snapshot.
  mgr.publishAllSnapshots();
  EXPECT_EQ(snapshotLogCount(), 1);
}

// (3) Both publish paths interact correctly: a snapshot logged via the
// future-publish path is not re-logged by a subsequent full buffer dump.
// This is the regression case for the double-log bug.
TEST_F(
    SnapshotManagerLoggingTest,
    FuturePublishThenBufferDumpLogsSnapshotOnce) {
  SnapshotManager mgr(portNames_, SnapshotLogSource::SNAPSHOT_AGENT, 1);
  mgr.publishFutureSnapshots(1);
  mgr.addSnapshot(phy::LinkSnapshot{});
  EXPECT_EQ(snapshotLogCount(), 1);

  mgr.publishAllSnapshots();
  EXPECT_EQ(snapshotLogCount(), 1);
}

} // namespace facebook::fboss
