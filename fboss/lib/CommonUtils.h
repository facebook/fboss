// (c) Facebook, Inc. and its affiliates. Confidential and proprietary.

#pragma once

#include <fb303/ThreadCachedServiceData.h>
#include <fb303/ThreadLocalStats.h>
#include <optional>
#include "fboss/agent/FbossError.h"

namespace facebook::fboss {
/*
 * For tests prefer using WITH_RETRIES and ASSERT_EVENTUALLY_* over this
 * utility to get better logging
 */
template <typename CONDITION_FN>
void checkWithRetry(
    CONDITION_FN condition,
    int retries = 10,
    std::chrono::duration<uint32_t, std::milli> msBetweenRetry =
        std::chrono::milliseconds(1000),
    std::optional<std::string> conditionFailedLog = std::nullopt) {
  while (retries--) {
    if (condition()) {
      return;
    }
    std::this_thread::sleep_for(msBetweenRetry);
  }

  constexpr auto kFailedConditionLog =
      "Verify with retry failed, condition was never satisfied";
  if (conditionFailedLog) {
    throw FbossError(kFailedConditionLog, " : ", *conditionFailedLog);
  } else {
    throw FbossError(kFailedConditionLog);
  }
}

inline int64_t getCumulativeValue(
    const fb303::ThreadCachedServiceData::TLTimeseries& stat) {
  auto counterVal = fb303::fbData->getCounterIfExists(stat.name() + ".sum");
  return counterVal ? *counterVal : 0;
}

/*
 * Utility to be used when we need to do a check on some async work.
 * This needs to be used in conjunction with ASSERT_EVENTUALLY to repeat
 * a series of checks and fail only if any of the checks fail after retries.
 * This solves the same problem as checkWithRetry, except we get much better
 * logs in case of failures. As such this should always be preferred over
 * checkWithRetry in tests.
 * USAGES:
 *
 * WITH_RETRIES_N_TIMED({
 *   auto someData = foo();
 *   ASSERT_EVENTUALLY_TRUE(someData.bar());
 *   ASSERT_EVENTUALLY_EQ(someData.bar().baz(), 0) << "baz is not zero!";
 * }, 30, std::chrono::milliseconds(1000));
 *
 * WITH_RETRIES(ASSERT_EVENTUALLY_TRUE(someBoolExpr()));
 *
 * The first example will run the code block up to 30 times and with 1s sleeps
 * in between.
 * ASSERT_EVENTUALLY: If a single ASSERT_EVENTUALLY fails, we'll loop back,
 * sleep and retry. Important to note that we don't evaluate future checks if
 * earlier ones fail, which means we can null check an object in an assert and
 * then deference safely. After all retries, we will hard ASSERT all checks.
 * EXPECT_EVENTUALLY: Unlike ASSERT_EVENTUALLY, during retries EXPECT_EVENTUALLY
 * will continue execution in case of failure. After all retires we will hard
 * EXPECT all checks
 */
// Exception not subclassing std::exception to avoid being caught by user code
struct _SoftAssertFail {};
#define WITH_RETRIES_N_TIMED(tests, maxRetries, sleepTime)              \
  {                                                                     \
    int WITH_RETRIES_tries = 0;                                         \
    while (WITH_RETRIES_tries++ < maxRetries) {                         \
      /* Do not sleep on first iteration */                             \
      if (WITH_RETRIES_tries != 1) {                                    \
        std::this_thread::sleep_for(sleepTime);                         \
      }                                                                 \
      /* Only switch to hard test on last retry */                      \
      bool WITH_RETRIES_softTest = WITH_RETRIES_tries != maxRetries;    \
      bool WITH_RETRIES_pass = true;                                    \
      /* _ASSERT_EVENTUALLY and _EXPECT_EVENTUALLY will read            \
       * WITH_RETRIES_softTest to decide how to assert.                 \
       * - soft expects will set WITH_RETRIES_pass in case of failures  \
       * - soft asserts will throw _SoftAssertFail so we can loop again \
       */                                                               \
      try {                                                             \
        tests;                                                          \
      } catch (const _SoftAssertFail&) {                                \
        continue;                                                       \
      }                                                                 \
      /* If all tests pass, we can break out early */                   \
      if (WITH_RETRIES_pass) {                                          \
        break;                                                          \
      }                                                                 \
    }                                                                   \
  }

// Helper with default sleep time
#define WITH_RETRIES_N(tests, maxRetries) \
  WITH_RETRIES_N_TIMED(tests, maxRetries, std::chrono::milliseconds(1000));

// Helper with default retries and sleep time
#define WITH_RETRIES(tests) WITH_RETRIES_N(tests, 30);

// Should ONLY be used inside WITH_RETIRES*. See helpers below
#define _ASSERT_EVENTUALLY(softTest, hardTest)                         \
  if (WITH_RETRIES_softTest) {                                         \
    /* If single test fails, continue */                               \
    if (!(softTest)) {                                                 \
      throw _SoftAssertFail();                                         \
    }                                                                  \
  } else                                                               \
    /* Skip braces and semi to allow logging with ASSERT(b) << msg; */ \
    hardTest

// Should ONLY be used inside WITH_RETIRES*. See helpers below
#define _EXPECT_EVENTUALLY(softTest, hardTest)                         \
  if (WITH_RETRIES_softTest) {                                         \
    /* evaluate test but continue execution  */                        \
    WITH_RETRIES_pass &= softTest;                                     \
  } else                                                               \
    /* Skip braces and semi to allow logging with ASSERT(b) << msg; */ \
    hardTest

/*
 * Helpers to ONLY be used inside WITH_RETIRES*. See usage described in
 * WITH_RETRIES_N_TIMED
 */
#define ASSERT_EVENTUALLY_TRUE(expr) _ASSERT_EVENTUALLY(expr, ASSERT_TRUE(expr))
#define ASSERT_EVENTUALLY_FALSE(expr) \
  _ASSERT_EVENTUALLY(!expr, ASSERT_FALSE(expr))
#define ASSERT_EVENTUALLY_EQ(expr1, expr2) \
  _ASSERT_EVENTUALLY(expr1 == expr2, ASSERT_EQ(expr1, expr2))

#define EXPECT_EVENTUALLY_TRUE(expr) _EXPECT_EVENTUALLY(expr, EXPECT_TRUE(expr))
#define EXPECT_EVENTUALLY_FALSE(expr) \
  _EXPECT_EVENTUALLY(!expr, EXPECT_FALSE(expr))
#define EXPECT_EVENTUALLY_EQ(expr1, expr2) \
  _EXPECT_EVENTUALLY(expr1 == expr2, EXPECT_EQ(expr1, expr2))

} // namespace facebook::fboss
