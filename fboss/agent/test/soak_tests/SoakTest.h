// (c) Facebook, Inc. and its affiliates. Confidential and proprietary.

#include "fboss/agent/test/AgentTest.h"

namespace facebook::fboss {

class SoakLoopArgs {};

class SoakTest : public AgentTest {
 public:
  bool RunLoops(SoakLoopArgs* args);
  // This is to be overridden by each test case.
  virtual bool RunOneLoop(SoakLoopArgs* loopArgs) = 0;

 private:
  uint64_t SoakTimeStrToSeconds(std::string timeStr);
};

int soakTestMain(int argc, char** argv, PlatformInitFn initPlatformFn);

} // namespace facebook::fboss
