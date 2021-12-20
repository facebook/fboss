// (c) Facebook, Inc. and its affiliates. Confidential and proprietary.

#include "fboss/agent/test/soak_tests/SoakTest.h"

namespace facebook::fboss {

class InterruptLoopArgs : public SoakLoopArgs {
 public:
  int numPktsPerLoop; // Not used at this point.  Could be used for finer
                      // control on the loop.
};

class InterruptTest : public SoakTest {
 public:
  void SetUp() override;
  void TearDown() override;
  bool RunOneLoop(SoakLoopArgs* args) override;

 private:
  void setUpPorts();
  bool setKmodParam(std::string param, std::string val);
  std::string getKmodParam(std::string param);
  int originalIntrTimeout_;
  PortID frontPanelPortToLoopTraffic_;
};

} // namespace facebook::fboss
