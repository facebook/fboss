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
}

const list<ProductionFeature> tomahawkFeature = [];
const list<ProductionFeature> tomahawk3Feature = [];
const list<ProductionFeature> tomahawk4Feature = [];
const list<ProductionFeature> tomahawk5Feature = [];
const list<ProductionFeature> jericho2Feature = [ProductionFeature.VOQ];
const list<ProductionFeature> jericho3Feature = [ProductionFeature.VOQ];
const list<ProductionFeature> ramonFeature = [ProductionFeature.FABRIC];
const list<ProductionFeature> ramon3Feature = [ProductionFeature.FABRIC];
const list<ProductionFeature> ebroFeature = [];
const list<ProductionFeature> yubaFeature = [];
