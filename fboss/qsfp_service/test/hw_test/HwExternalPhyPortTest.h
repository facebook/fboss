/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#pragma once

#include "fboss/qsfp_service/test/hw_test/HwTest.h"

#include "fboss/lib/phy/ExternalPhy.h"

namespace facebook::fboss {
class HwExternalPhyPortTest : public HwTest {
 public:
  HwExternalPhyPortTest() = default;
  ~HwExternalPhyPortTest() override = default;

  virtual const std::vector<phy::ExternalPhy::Feature>& neededFeatures()
      const = 0;
  std::string neededFeatureNames() const;

  virtual std::vector<std::pair<PortID, cfg::PortProfileID>>
  findAvailableXphyPorts();

  std::vector<qsfp_production_features::QsfpProductionFeature>
  getProductionFeatures() const override;
};
} // namespace facebook::fboss
