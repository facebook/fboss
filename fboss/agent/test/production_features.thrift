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
}

const list<ProductionFeature> tomahawkFeature = [
  ProductionFeature.CPU_RX_TX,
  ProductionFeature.L3_FORWARDING,
  ProductionFeature.NULL_ROUTE_IN_DISCARDS_COUNTER,
];
const list<ProductionFeature> tomahawk3Feature = [
  ProductionFeature.CPU_RX_TX,
  ProductionFeature.L3_FORWARDING,
  ProductionFeature.NULL_ROUTE_IN_DISCARDS_COUNTER,
];
const list<ProductionFeature> tomahawk4Feature = [
  ProductionFeature.CPU_RX_TX,
  ProductionFeature.L3_FORWARDING,
  ProductionFeature.NULL_ROUTE_IN_DISCARDS_COUNTER,
];
const list<ProductionFeature> tomahawk5Feature = [
  ProductionFeature.CPU_RX_TX,
  ProductionFeature.L3_FORWARDING,
  ProductionFeature.NULL_ROUTE_IN_DISCARDS_COUNTER,
];
const list<ProductionFeature> jericho2Feature = [
  ProductionFeature.CPU_RX_TX,
  ProductionFeature.L3_FORWARDING,
  ProductionFeature.NULL_ROUTE_IN_DISCARDS_COUNTER,
  ProductionFeature.VOQ,
  ProductionFeature.ACL_PORT_IN_DISCARDS_COUNTER,
  ProductionFeature.EGRESS_FORWARDING_DISCARDS_COUNTER,
];
const list<ProductionFeature> jericho3Feature = [
  ProductionFeature.CPU_RX_TX,
  ProductionFeature.L3_FORWARDING,
  ProductionFeature.NULL_ROUTE_IN_DISCARDS_COUNTER,
  ProductionFeature.VOQ,
  ProductionFeature.ACL_PORT_IN_DISCARDS_COUNTER,
  ProductionFeature.EGRESS_FORWARDING_DISCARDS_COUNTER,
];
const list<ProductionFeature> ramonFeature = [ProductionFeature.FABRIC];
const list<ProductionFeature> ramon3Feature = [ProductionFeature.FABRIC];
const list<ProductionFeature> ebroFeature = [
  ProductionFeature.CPU_RX_TX,
  ProductionFeature.L3_FORWARDING,
  ProductionFeature.NULL_ROUTE_IN_DISCARDS_COUNTER,
];
const list<ProductionFeature> yubaFeature = [
  ProductionFeature.CPU_RX_TX,
  ProductionFeature.L3_FORWARDING,
  ProductionFeature.NULL_ROUTE_IN_DISCARDS_COUNTER,
];
