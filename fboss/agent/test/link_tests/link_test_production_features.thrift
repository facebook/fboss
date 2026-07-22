#
# Copyright 2004-present Facebook. All Rights Reserved.
#

include "thrift/annotation/thrift.thrift"

@thrift.AllowLegacyMissingUris
package;

namespace py neteng.fboss.link_test_production_features
namespace py3 neteng.fboss
namespace py.asyncio neteng.fboss.asyncio.link_test_production_features
namespace cpp2 facebook.fboss.link_test_production_features
namespace go neteng.fboss.link_test_production_features
namespace php fboss_link_test_production_features

enum LinkTestProductionFeature {
  L1_LINK_TEST = 0,
  L2_LINK_TEST = 1,
  // Functional features describing what a test verifies. Unlike L1/L2 above
  // (which gate test selection via --list_production_feature), these are
  // surfaced only via AgentEnsembleLinkTest::addVerifiedProductionFeatures()
  // for per-test metadata/Scuba reporting and are NOT used for test gating.
  LINK_BRINGUP = 2,
  TRAFFIC_FORWARDING = 3,
  IPHY_DIAGS = 4,
  XPHY_DIAGS = 5,
  IPHY_FEC_INJECT = 6,
  PRBS = 7,
  LACP = 8,
  MAC_LEARNING = 9,
  PTP = 10,
  WARMBOOT = 11,
  SPEED_CHANGE = 12,
  BMC_UPGRADE = 13,
  FABRIC_LINK = 14,
  FSDB_PUBLISH = 15,
  MACSEC = 16,
  TRANSCEIVER_TX_DISABLE = 17,
  REMEDIATION = 18,
  TRANSCEIVER_TX_RX_LATCHES = 19,
  VDM = 20,
  IPHY_FEC_COUNTERS = 21,
}
