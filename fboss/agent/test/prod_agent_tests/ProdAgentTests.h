#pragma once
#include "fboss/agent/state/Port.h"
#include "fboss/agent/state/PortDescriptor.h"
#include "fboss/agent/test/AgentTest.h"

DECLARE_string(config);
DECLARE_bool(enable_lldp);
DECLARE_bool(tun_intf);
DECLARE_bool(disable_neighbor_updates);

namespace facebook::fboss {

class ProdAgentTests : public AgentTest {
 protected:
  void SetUp() override;
  void setupConfigFlag() override;
  void setCmdLineFlagOverrides() const override;
};
} // namespace facebook::fboss
