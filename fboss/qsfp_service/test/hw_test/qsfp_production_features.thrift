#
# Copyright 2004-present Facebook. All Rights Reserved.
#

namespace py neteng.fboss.qsfp_production_features
namespace py3 neteng.fboss
namespace py.asyncio neteng.fboss.asyncio.qsfp_production_features
namespace cpp2 facebook.fboss.qsfp_production_features
namespace go neteng.fboss.qsfp_production_features
namespace php fboss_qsfp_production_features

enum QsfpProductionFeature {
  MACSEC = 0,
  EXTERNAL_PHY = 1,
  PIM = 2,
  XPHY_SYSTEM_NRZ_PROFILE = 3,
  XPHY_SYSTEM_PAM4_PROFILE = 4,
  XPHY_LINE_NRZ_PROFILE = 5,
  XPHY_LINE_PAM4_PROFILE = 6,
}

struct PlatformConfigToQsfpProductionFeatures {
  1: map<string, list<QsfpProductionFeature>> platformConfigToFeatures;
  2: map<string, list<string>> platformConfigToFeatureNames;
}
