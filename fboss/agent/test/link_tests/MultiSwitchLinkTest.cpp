// (c) Facebook, Inc. and its affiliates. Confidential and proprietary.

#include <folly/String.h>
#include <folly/Subprocess.h>
#include <gflags/gflags.h>

#include <thrift/lib/cpp2/protocol/DebugProtocol.h>
#include "fboss/agent/AgentConfig.h"
#include "fboss/agent/LldpManager.h"
#include "fboss/agent/SwSwitch.h"
#include "fboss/agent/hw/gen-cpp2/hardware_stats_types.h"
#include "fboss/agent/hw/test/LoadBalancerUtils.h"
#include "fboss/agent/hw/test/dataplane_tests/HwTestQosUtils.h"
#include "fboss/agent/state/Port.h"
#include "fboss/agent/state/PortMap.h"
#include "fboss/agent/state/StateUtils.h"
#include "fboss/agent/state/SwitchState.h"
#include "fboss/agent/test/EcmpSetupHelper.h"
#include "fboss/agent/test/link_tests/MultiSwitchLinkTest.h"
#include "fboss/agent/test/utils/CoppTestUtils.h"
#include "fboss/agent/test/utils/QosTestUtils.h"
#include "fboss/lib/CommonFileUtils.h"
#include "fboss/lib/CommonUtils.h"
#include "fboss/lib/config/PlatformConfigUtils.h"
#include "fboss/lib/phy/gen-cpp2/phy_types_custom_protocol.h"
#include "fboss/lib/thrift_service_client/ThriftServiceClient.h"
#include "fboss/qsfp_service/if/gen-cpp2/transceiver_types_custom_protocol.h"

DECLARE_bool(enable_macsec);

DECLARE_bool(skip_drain_check_for_prbs);

DEFINE_bool(
    link_stress_test,
    false,
    "enable to run stress tests (longer duration + more iterations)");

namespace {
const std::vector<std::string> kRestartQsfpService = {
    "/bin/systemctl",
    "restart",
    "qsfp_service_for_testing"};

const std::string kForceColdbootQsfpSvcFileName =
    "/dev/shm/fboss/qsfp_service/cold_boot_once_qsfp_service";

// This file stores all non-validated transceiver configurations found while
// running getAllTransceiverConfigValidationStatuses() in EmptyLinkTest. Each
// configuration will be in JSON format. This file is then downloaded by the
// Netcastle test runner to parse it and log these configurations to Scuba.
// Matching file definition is located in
// fbcode/neteng/netcastle/teams/fboss/constants.py.
const std::string kTransceiverConfigJsonsForScuba =
    "/tmp/transceiver_config_jsons_for_scuba.log";
} // namespace

namespace facebook::fboss {

void MultiSwitchLinkTest::SetUp() {
  AgentEnsembleTest::SetUp();
  XLOG(DBG2) << "Multi Switch Link Test setup ready";
}

void MultiSwitchLinkTest::restartQsfpService(bool coldboot) const {
  if (coldboot) {
    createFile(kForceColdbootQsfpSvcFileName);
    XLOG(DBG2) << "Restarting QSFP Service in coldboot mode";
  } else {
    XLOG(DBG2) << "Restarting QSFP Service in warmboot mode";
  }

  folly::Subprocess(kRestartQsfpService).waitChecked();
}

void MultiSwitchLinkTest::TearDown() {
  // Expect the qsfp service to be running at the end of the tests
  auto qsfpServiceClient = utils::createQsfpServiceClient();
  EXPECT_EQ(
      facebook::fb303::cpp2::fb_status::ALIVE,
      qsfpServiceClient.get()->sync_getStatus())
      << "QSFP Service no longer alive after the test";
  EXPECT_EQ(
      QsfpServiceRunState::ACTIVE,
      qsfpServiceClient.get()->sync_getQsfpServiceRunState())
      << "QSFP Service run state no longer active after the test";
  AgentEnsembleTest::TearDown();
}

void MultiSwitchLinkTest::setCmdLineFlagOverrides() const {
  FLAGS_enable_macsec = true;
  FLAGS_skip_drain_check_for_prbs = true;
  AgentEnsembleTest::setCmdLineFlagOverrides();
}

int multiSwitchLinkTestMain(
    int argc,
    char** argv,
    PlatformInitFn initPlatformFn,
    std::optional<cfg::StreamType> streamType) {
  ::testing::InitGoogleTest(&argc, argv);
  initAgentEnsembleTest(argc, argv, initPlatformFn, streamType);
  return RUN_ALL_TESTS();
}

} // namespace facebook::fboss
