// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include <functional>
#include <string>
#include <vector>

namespace facebook::fboss::hal_test {

struct TransceiverTestResult {
  int tcvrId{0};
  bool passed{true};
  bool skipped{false};
  std::vector<std::string> failures; // non-fatal (EXPECT equivalent)
  std::string fatalFailure; // fatal (ASSERT equivalent)
};

// Runs fn in parallel (one std::thread per transceiver). Catches exceptions.
std::vector<TransceiverTestResult> runForAllTransceivers(
    const std::vector<int>& tcvrIds,
    std::function<TransceiverTestResult(int)> fn);

// Replays results as gtest ADD_FAILURE() on main thread. Returns true if all
// passed.
bool reportTransceiverResults(
    const std::vector<TransceiverTestResult>& results);

} // namespace facebook::fboss::hal_test

// Record a non-fatal failure (EXPECT equivalent). Continues execution.
#define HAL_CHECK(result, cond, msg)    \
  do {                                  \
    if (!(cond)) {                      \
      (result).passed = false;          \
      (result).failures.push_back(msg); \
    }                                   \
  } while (0)

// Record a fatal failure (ASSERT equivalent). Returns from the enclosing
// function (which must return TransceiverTestResult).
#define HAL_CHECK_FATAL(result, cond, msg) \
  do {                                     \
    if (!(cond)) {                         \
      (result).passed = false;             \
      (result).fatalFailure = (msg);       \
      return (result);                     \
    }                                      \
  } while (0)

// Record a fatal failure (ASSERT equivalent) for void-returning functions.
// Use this in lambdas passed to forEachTransceiverParallel where the result
// is passed by reference.
#define HAL_CHECK_FATAL_VOID(result, cond, msg) \
  do {                                          \
    if (!(cond)) {                              \
      (result).passed = false;                  \
      (result).fatalFailure = (msg);            \
      return;                                   \
    }                                           \
  } while (0)
