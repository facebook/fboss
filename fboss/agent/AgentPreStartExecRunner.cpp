// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/agent/AgentPreStartExec.h"

#include "fboss/agent/FbossError.h"

#include <folly/init/Init.h>
#include <folly/logging/Init.h>
#include <folly/logging/xlog.h>
#include <gflags/gflags.h>
#include <thread>

#ifdef IS_OSS
FOLLY_INIT_LOGGING_CONFIG("DBG2; default:async=true");
#else
FOLLY_INIT_LOGGING_CONFIG("fboss=DBG2; default:async=true");
#endif

int main(int argc, char* argv[]) {
  gflags::ParseCommandLineFlags(&argc, &argv, false);
  facebook::fboss::AgentPreStartExec preStartExec;
  folly::Init init{&argc, &argv, true};
  try {
    preStartExec.run();
  } catch (const facebook::fboss::FbossError& error) {
    // Exit gracefully without throwing an exception to prevent tight crash
    // loop.
    XLOG(ERR) << "Failed to run AgentPreStartExec: " << error.what();
    // @lint-ignore CLANGTIDY
    std::this_thread::sleep_for(std::chrono::seconds(1));
    return 2;
  }
  return 0;
}
