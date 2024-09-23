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
  SINGLE_ACL_TABLE = 16,
  MULTI_ACL_TABLE = 17,
  BTH_OPCODE_ACL = 18,
  DLB = 19,
  ECN = 20,
  WRED = 21,
  QUEUE_PER_HOST = 22,
  JUMBO_FRAMES = 23, // Misc
  TRAP_DISCARDS_COUNTER = 24,
  MAC_LEARNING = 25,
  SCHEDULER_PPS = 26,
  ROUTE_COUNTERS = 27,
  PORT_TX_DISABLE = 28, // Misc
  SFLOWv4_SAMPLING = 29,
  SFLOWv6_SAMPLING = 30,
  MIRROR_PACKET_TRUNCATION = 31,
  WIDE_ECMP = 32,
  INTERFACE_NEIGHBOR_TABLE = 33,
  INGRESS_MIRRORING = 34,
  EGRESS_MIRRORING = 35,
  RSW_ROUTE_SCALE = 36, // Misc
  FSW_ROUTE_SCALE = 37, // Misc
  HGRID_DU_ROUTE_SCALE = 38, // Misc
  HGRID_UU_ROUTE_SCALE = 39, // Misc
  HUNDRED_THOUSAND_ROUTE_SCALE = 40, // Misc
  TH_ALPM_ROUTE_SCALE = 41,
  PFC = 42,
  UDF_WR_IMMEDIATE_ACL = 43,
  # production feature which is present on all platforms, keep it at the end
  HW_SWITCH = 65536,
}

struct AsicToProductionFeatures {
  1: map<string, list<ProductionFeature>> asicToFeatures;
  2: map<string, list<string>> asicToFeatureNames;
}
