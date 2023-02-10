// (c) Facebook, Inc. and its affiliates. Confidential and proprietary.

#include <gflags/gflags.h>
#include <chrono>

#include "fboss/agent/test/soak_tests/SoakTest.h"

DEFINE_int32(soak_loops, -1, "Number of soak test loops to run");
DEFINE_string(
    soak_time,
    "1s",
    "Total time to run soak test, <\\d+>[smhd], s-sec, m-min, h-hour, d-day");

namespace facebook::fboss {

uint64_t SoakTest::SoakTimeStrToSeconds(std::string timeStr) {
  char unit = timeStr[timeStr.length() - 1];
  uint64_t digits = stoi(timeStr.substr(0, timeStr.length() - 1));
  switch (unit) {
    // no break in the cases, so the executions will fall through from d to s.
    FMT_FALLTHROUGH;
    case 'd':
      digits *= 24;
      FMT_FALLTHROUGH;
    case 'h':
      digits *= 60;
      FMT_FALLTHROUGH;
    case 'm':
      digits *= 60;
      FMT_FALLTHROUGH;
    case 's':
      return digits;
    default:
      throw FbossError(
          "Invalid soak time string <", timeStr, ">, the format is \\d+[dhms]");
  }
}

bool SoakTest::RunLoops(SoakLoopArgs* args) {
  uint64_t SoakTimeSec;
  if (FLAGS_soak_loops > 0) {
    XLOG(DBG2) << " SoakTest::RunLoops: will run for " << FLAGS_soak_loops
               << " loops.";
  } else {
    XLOG(DBG2) << " SoakTest::RunLoops: will run for " << FLAGS_soak_time
               << ".";
    SoakTimeSec = SoakTimeStrToSeconds(FLAGS_soak_time);
  }
  bool ret = true;

  std::chrono::time_point timeStart = std::chrono::system_clock::now();
  std::chrono::time_point timeCurr = timeStart;
  std::chrono::duration secPassed =
      std::chrono::duration_cast<seconds>(timeCurr - timeStart);
  int loopIdx;

  for (loopIdx = 0; true; loopIdx++) {
    if (!ret) {
      XLOG(ERR) << "RunOneLoop() failed.  Exit the loop.";
      break;
    } else if ((FLAGS_soak_loops > 0) && (loopIdx >= FLAGS_soak_loops)) {
      // break by loop count
      break;
    } else {
      // break by time, default is 1 second
      timeCurr = std::chrono::system_clock::now();
      secPassed = std::chrono::duration_cast<seconds>(timeCurr - timeStart);
      if (secPassed.count() >= SoakTimeSec) {
        break;
      }
    }

    XLOG(DBG2) << " SoakTest::RunLoops: loopIdx = " << loopIdx;
    ret = RunOneLoop(args);
  }

  timeCurr = std::chrono::system_clock::now();
  secPassed = std::chrono::duration_cast<seconds>(timeCurr - timeStart);

  XLOG(DBG2) << " SoakTest::RunLoops: Done " << loopIdx << " loops, in "
             << secPassed.count() << " seconds.";

  return ret;
}

int soakTestMain(int argc, char** argv, PlatformInitFn initPlatformFn) {
  ::testing::InitGoogleTest(&argc, argv);

  initAgentTest(argc, argv, initPlatformFn);

  return RUN_ALL_TESTS();
}

} // namespace facebook::fboss
