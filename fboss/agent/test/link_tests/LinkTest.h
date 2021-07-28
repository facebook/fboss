// (c) Facebook, Inc. and its affiliates. Confidential and proprietary.

#include "fboss/agent/Main.h"
#include "fboss/agent/state/Port.h"
#include "fboss/agent/state/PortDescriptor.h"
#include "fboss/agent/test/AgentTest.h"

#include <boost/container/flat_set.hpp>

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
  /*
   * Ports where we expect optics to be plugged in.
   * In link tests this information is conveyed in config via non
   * null LLDP neighbors. We pick that up here to extract cabled ports
   */
  const std::vector<PortID>& getCabledPorts() const;
  std::vector<std::string> getPortNames(const std::vector<PortID>& ports) const;
  boost::container::flat_set<PortDescriptor> getVlanOwningCabledPorts() const;
  /*
   * Assert no in discards occured on any of the switch ports.
   * When used in conjunction with createL3DataplaneFlood, can be
   * used to verify that none of the traffic bearing ports flapped
   */
  void assertNoInDiscards();
  /*
   * Create a L3 data plane loop and seed it with traffic
   */
  void createL3DataplaneFlood();

 private:
  void initializeCabledPorts();
  std::vector<PortID> cabledPorts_;
};
int linkTestMain(int argc, char** argv, PlatformInitFn initPlatformFn);
} // namespace facebook::fboss
