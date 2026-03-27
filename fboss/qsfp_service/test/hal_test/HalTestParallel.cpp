// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/qsfp_service/test/hal_test/HalTestParallel.h"

#include <thread>

#include <fmt/core.h>
#include <folly/logging/xlog.h>
#include <gtest/gtest.h>

namespace facebook::fboss::hal_test {

std::vector<TransceiverTestResult> runForAllTransceivers(
    const std::vector<int>& tcvrIds,
    std::function<TransceiverTestResult(int)> fn) {
  std::vector<TransceiverTestResult> results(tcvrIds.size());
  std::vector<std::thread> threads;
  threads.reserve(tcvrIds.size());

  for (size_t i = 0; i < tcvrIds.size(); ++i) {
    threads.emplace_back([&results, &fn, &tcvrIds, i]() {
      auto tcvrId = tcvrIds[i];
      try {
        results[i] = fn(tcvrId);
      } catch (const std::exception& ex) {
        results[i].tcvrId = tcvrId;
        results[i].passed = false;
        results[i].fatalFailure = fmt::format(
            "Transceiver {} threw exception: {}", tcvrId, ex.what());
      }
    });
  }

  for (auto& t : threads) {
    t.join();
  }

  return results;
}

bool reportTransceiverResults(
    const std::vector<TransceiverTestResult>& results) {
  bool allPassed = true;

  for (const auto& r : results) {
    if (r.skipped) {
      continue;
    }
    if (!r.passed) {
      allPassed = false;
      for (const auto& f : r.failures) {
        ADD_FAILURE() << "Transceiver " << r.tcvrId << ": " << f;
      }
      if (!r.fatalFailure.empty()) {
        ADD_FAILURE() << "Transceiver " << r.tcvrId
                      << " (fatal): " << r.fatalFailure;
      }
    }
  }

  return allPassed;
}

} // namespace facebook::fboss::hal_test
