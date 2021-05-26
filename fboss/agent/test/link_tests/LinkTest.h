// (c) Facebook, Inc. and its affiliates. Confidential and proprietary.

#include "fboss/agent/Main.h"
#include "fboss/agent/state/Port.h"
#include "fboss/agent/test/AgentTest.h"

namespace facebook::fboss {

class LinkTest : public AgentTest {
 protected:
  void SetUp() override;
  void waitForAllCabledPorts(
      bool up,
      uint32_t retries = 60,
      std::chrono::duration<uint32_t, std::milli> msBetweenRetry =
          std::chrono::milliseconds(1000)) const;
  void waitForLinkStatus(
      const std::vector<PortID>& portsToCheck,
      bool up,
      uint32_t retries = 60,
      std::chrono::duration<uint32_t, std::milli> msBetweenRetry =
          std::chrono::milliseconds(1000)) const;
  const std::vector<PortID>& getCabledPorts() const;
  std::vector<std::string> getPortNames(const std::vector<PortID>& ports) const;

 private:
  void initializeCabledPorts();
  std::vector<PortID> cabledPorts_;
};
int linkTestMain(int argc, char** argv, PlatformInitFn initPlatformFn);
} // namespace facebook::fboss
