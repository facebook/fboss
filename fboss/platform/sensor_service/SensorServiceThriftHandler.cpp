/*
 *  Copyright (c) 2004-present, Meta Platforms, Inc. and affiliates.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/platform/sensor_service/SensorServiceThriftHandler.h"

#include <algorithm>
#include <filesystem>
#include <string_view>
#include <system_error>
#include <vector>

#include <fmt/core.h>
#include <folly/FileUtil.h>
#include <folly/logging/xlog.h>
#include <gflags/gflags.h>
#include <thrift/lib/cpp2/protocol/Serializer.h>

#include "fboss/lib/LogThriftCall.h"
#include "fboss/platform/reboot_cause_finder/if/gen-cpp2/reboot_cause_config_constants.h"
#include "fboss/platform/sensor_service/Utils.h"

DEFINE_string(
    reboot_history_dir,
    facebook::fboss::platform::reboot_cause_config::
        reboot_cause_config_constants::REBOOT_HISTORY_DIR(),
    "Directory holding reboot_cause_finder's per-boot records. Overridable "
    "for tests.");

namespace facebook::fboss::platform::sensor_service {

void SensorServiceThriftHandler::getSensorValuesByNames(
    SensorReadResponse& response,
    std::unique_ptr<std::vector<std::string>> request) {
  auto log = LOG_THRIFT_CALL(DBG1);

  response.timeStamp() = Utils::nowInSecs();

  // Request list is not empty
  if (!request->empty()) {
    response.sensorData() = sensorServiceImpl_->getSensorsData(*request);
    return;
  }

  // Request list is empty, we send all the sensor data
  for (const auto& sensorDataItr : sensorServiceImpl_->getAllSensorData()) {
    response.sensorData()->push_back(sensorDataItr.second);
  }
}

namespace {
// A record is a few KiB; anything far larger is not one of ours and is not
// worth reading into memory.
constexpr uintmax_t kMaxRecordBytes = 1 << 20; // 1 MiB
// reboot_cause_finder writes one record per boot and never prunes, so the
// directory grows without bound. Read only the newest few.
constexpr size_t kMaxRecords = 64;
constexpr size_t kMaxWarnings = 8;
// Matches the filenames reboot_cause_finder writes:
// reboot-cause-<stamp>-<bootId>.json
constexpr std::string_view kRecordPrefix = "reboot-cause-";
constexpr std::string_view kRecordSuffix = ".json";

void addWarning(RebootHistoryResponse& response, const std::string& warning) {
  auto& warnings = *response.warnings();
  if (warnings.size() >= kMaxWarnings) {
    return;
  }
  if (std::find(warnings.begin(), warnings.end(), warning) == warnings.end()) {
    warnings.push_back(warning);
  }
}
} // namespace

// Reads a directory written by a different process on its own schedule. Every
// failure here is someone else's partial write, a stale file, or a full disk --
// none of which is a reason for this RPC, let alone sensor_service, to fail.
// So: no exception escapes, and anything unreadable is counted rather than
// thrown.
void SensorServiceThriftHandler::getRebootHistory(
    RebootHistoryResponse& response) {
  response.records() = {};
  response.skippedCount() = 0;
  response.warnings() = {};

  try {
    namespace fs = std::filesystem;
    const fs::path historyDir(FLAGS_reboot_history_dir);

    std::error_code ec;
    if (!fs::is_directory(historyDir, ec) || ec) {
      // Expected on any switch that has not yet run reboot_cause_finder.
      addWarning(
          response,
          fmt::format(
              "No reboot history directory at '{}'", FLAGS_reboot_history_dir));
      return;
    }

    // Collect paths first so they can be ordered before any file is read, and
    // the read capped to kMaxRecords.
    //
    // `ec` has to be tested after construction and after each increment: on
    // failure the iterator is reset to end, so the loop condition would
    // otherwise exit first and a partial scan would look like a complete one.
    std::vector<fs::path> paths;
    fs::directory_iterator it(historyDir, ec), end;
    if (ec) {
      addWarning(response, "Reboot history directory could not be read");
      return;
    }
    // Stepped by hand rather than with a range-for: increment() resets the
    // iterator to end on failure, so a for-loop would exit on the condition
    // before any check in the body could run, and a partial scan would look
    // like a complete one.
    while (it != end) {
      if (it->is_regular_file(ec) && !ec) {
        // Only our own records. Anything else dropped here is deliberately
        // not counted as skipped: it was never a record to begin with.
        const auto name = it->path().filename().string();
        if (name.starts_with(kRecordPrefix) && name.ends_with(kRecordSuffix)) {
          paths.push_back(it->path());
        }
      }
      it.increment(ec);
      if (ec) {
        addWarning(
            response, "Reboot history directory could not be fully read");
        break;
      }
    }

    // Filenames embed a local-time stamp, so a lexicographic sort is only a
    // heuristic for "newest" -- reboot_cause_finder may stamp a record before
    // NTP has corrected the clock. It is good enough to decide which files to
    // read when there are more than the cap; the ordering actually returned is
    // by bootTimeMs below.
    std::sort(paths.begin(), paths.end());
    if (paths.size() > kMaxRecords) {
      *response.skippedCount() +=
          static_cast<int32_t>(paths.size() - kMaxRecords);
      addWarning(
          response,
          fmt::format(
              "More than {} records present; read the {} newest by filename",
              kMaxRecords,
              kMaxRecords));
      paths.erase(paths.begin(), paths.end() - kMaxRecords);
    }

    for (const auto& path : paths) {
      // Bound the read itself rather than stat-then-read: the file could grow
      // between the two. Reading one byte past the cap is what distinguishes
      // "exactly at the limit" from "truncated by it".
      std::string contents;
      if (!folly::readFile(path.c_str(), contents, kMaxRecordBytes + 1) ||
          contents.empty()) {
        *response.skippedCount() += 1;
        addWarning(response, "A record could not be read");
        continue;
      }
      if (contents.size() > kMaxRecordBytes) {
        *response.skippedCount() += 1;
        addWarning(response, "A record was too large to read");
        continue;
      }
      try {
        response.records()->push_back(
            apache::thrift::SimpleJSONSerializer::deserialize<
                reboot_cause_config::RebootCauseRecord>(contents));
      } catch (const std::exception& ex) {
        // A record still being written, or truncated by a full disk.
        *response.skippedCount() += 1;
        addWarning(
            response,
            fmt::format("A record could not be parsed: {}", ex.what()));
      }
    }

    // Newest boot first, like `last reboot`. Ordering by detectedAtMs would
    // move a boot whose cause was determined late (a manual or redo run) above
    // later boots. A record with no boot time sorts last.
    std::sort(
        response.records()->begin(),
        response.records()->end(),
        [](const auto& a, const auto& b) {
          if (*a.bootTimeMs() != *b.bootTimeMs()) {
            return *a.bootTimeMs() > *b.bootTimeMs();
          }
          return *a.detectedAtMs() > *b.detectedAtMs();
        });
  } catch (const std::exception& ex) {
    // Backstop. Nothing above is expected to throw, but this RPC must not be
    // the reason sensor_service returns an error.
    XLOG(ERR) << "getRebootHistory failed: " << ex.what();
    addWarning(response, "Reboot history could not be read");
  } catch (...) {
    XLOG(ERR) << "getRebootHistory failed with a non-standard exception";
    addWarning(response, "Reboot history could not be read");
  }
}

} // namespace facebook::fboss::platform::sensor_service
