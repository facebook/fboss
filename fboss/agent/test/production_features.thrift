#
# Copyright 2004-present Facebook. All Rights Reserved.
#

namespace py neteng.fboss.production_features
namespace py3 neteng.fboss
namespace py.asyncio neteng.fboss.asyncio.production_features
namespace cpp2 facebook.fboss.production_features
namespace go neteng.fboss.production_features
namespace php fboss_production_features

enum ProductionFeature {
  VOQ = 0,
  FABRIC = 1,
  CPU_RX_TX = 2,
  L3_FORWARDING = 3,
  ACL_PORT_IN_DISCARDS_COUNTER = 4,
  NULL_ROUTE_IN_DISCARDS_COUNTER = 5,
  EGRESS_FORWARDING_DISCARDS_COUNTER = 6,
  PRBS = 7,
  COPP = 8,
  LAG = 9,
  DSCP_REMARKING = 10,
  L3_QOS = 11,
  ACL_COUNTER = 12,
  ECMP_LOAD_BALANCER = 13,
  LAG_LOAD_BALANCER = 14,
  VOQ_DNX_INTERRUPTS = 15,
}

struct AsicToProductionFeatures {
  1: map<string, list<ProductionFeature>> asicToFeatures;
  2: map<string, list<string>> asicToFeatureNames;
}
