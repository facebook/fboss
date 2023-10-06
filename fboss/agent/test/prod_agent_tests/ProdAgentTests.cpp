#include "fboss/agent/test/prod_agent_tests/ProdAgentTests.h"

namespace facebook::fboss {

void ProdAgentTests::SetUp() {}
void ProdAgentTests::setupConfigFlag() {}
void ProdAgentTests::setCmdLineFlagOverrides() const {
  // disable other processes which generate pkts
  FLAGS_enable_lldp = false;
  // tunnel interface enable especially on fboss2000 results
  FLAGS_tun_intf = false;
  AgentTest::setCmdLineFlagOverrides();
}

} // namespace facebook::fboss
