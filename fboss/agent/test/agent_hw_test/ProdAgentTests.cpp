#include "fboss/agent/test/agent_hw_test/ProdAgentTests.h"

namespace facebook::fboss {

void ProdAgentTests::SetUp() {}
void ProdAgentTests::setupConfigFlag() {}
void ProdAgentTests::setupFlags() const {
  // disable other processes which generate pkts
  FLAGS_enable_lldp = false;
  // tunnel interface enable especially on fboss2000 results
  FLAGS_tun_intf = false;
  AgentTest::setupFlags();
}

} // namespace facebook::fboss
