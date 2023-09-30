// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/agent/AgentPreStartExec.h"

#include <folly/logging/Init.h>
#include <gflags/gflags.h>

#ifdef IS_OSS
FOLLY_INIT_LOGGING_CONFIG("DBG2; default:async=true");
#else
FOLLY_INIT_LOGGING_CONFIG("fboss=DBG2; default:async=true");
#endif

int main(int argc, char* argv[]) {
  gflags::ParseCommandLineFlags(&argc, &argv, false);
  facebook::fboss::AgentPreStartExec preStartExec;
  preStartExec.run();
  return 0;
}
