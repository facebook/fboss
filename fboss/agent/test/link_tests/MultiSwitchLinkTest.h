// (c) Facebook, Inc. and its affiliates. Confidential and proprietary.

#include "fboss/agent/LldpManager.h"
#include "fboss/agent/Main.h"
#include "fboss/agent/state/PortDescriptor.h"
#include "fboss/agent/test/AgentEnsembleTest.h"
#include "fboss/agent/test/EcmpSetupHelper.h"
#include "fboss/agent/types.h"
#include "fboss/lib/phy/gen-cpp2/phy_types.h"

#include <boost/container/flat_set.hpp>

DECLARE_string(oob_asset);
DECLARE_string(oob_flash_device_name);
DECLARE_string(openbmc_password);
DECLARE_bool(enable_lldp);
DECLARE_bool(tun_intf);
DECLARE_string(volatile_state_dir);
DECLARE_bool(setup_for_warmboot);
DECLARE_string(config);
DECLARE_bool(disable_neighbor_updates);
DECLARE_bool(link_stress_test);

namespace facebook::fboss {

using namespace std::chrono_literals;

// When we switch to use qsfp_service to collect stats(PhyInfo), default stats
// collection frequency is 60s. Give the maximum check time 5x24=120s here.
constexpr auto kSecondsBetweenXphyInfoCollectionCheck = 5s;
constexpr auto kMaxNumXphyInfoCollectionCheck = 24;

class MultiSwitchLinkTest : public AgentEnsembleTest {
 protected:
  void SetUp() override;

  void TearDown() override;

 private:
};
int multiSwitchLinkTestMain(
    int argc,
    char** argv,
    PlatformInitFn initPlatformFn,
    std::optional<cfg::StreamType> streamType = std::nullopt);
} // namespace facebook::fboss
