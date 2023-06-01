/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/qsfp_service/test/hw_test/HwExternalPhyPortTest.h"

#include "fboss/lib/phy/PhyManager.h"
#include "fboss/qsfp_service/test/hw_test/HwPortUtils.h"
#include "fboss/qsfp_service/test/hw_test/HwQsfpEnsemble.h"

#include <fmt/format.h>

namespace facebook::fboss {
std::string HwExternalPhyPortTest::neededFeatureNames() const {
  std::stringstream ss;
  ss << "[";
  const auto& features = neededFeatures();
  for (auto i = 0; i < features.size(); i++) {
    ss << phy::ExternalPhy::featureName(features[i]);
    if (i != features.size() - 1) {
      ss << ", ";
    }
  }
  ss << "]";
  return ss.str();
}

std::vector<std::pair<PortID, cfg::PortProfileID>>
HwExternalPhyPortTest::findAvailableXphyPorts() {
  auto* phyManager = getHwQsfpEnsemble()->getPhyManager();
  const auto& ports = utility::findAvailableCabledPorts(getHwQsfpEnsemble());
  std::vector<std::pair<PortID, cfg::PortProfileID>> xphyPortAndProfiles;
  // Check whether the ExternalPhy of such xphy port support all needed features
  const auto& features = neededFeatures();
  for (auto& [port, profile] : ports.xphyPorts) {
    auto* xphy = phyManager->getExternalPhy(port);
    bool isSupported = true;
    for (auto feature : features) {
      if (!xphy->isSupported(feature)) {
        isSupported = false;
        continue;
      }
    }
    if (isSupported) {
      xphyPortAndProfiles.push_back(std::make_pair(port, profile));
    }
  }
  CHECK(!xphyPortAndProfiles.empty())
      << "Can't find xphy ports to support features:" << neededFeatureNames();
  return xphyPortAndProfiles;
}
} // namespace facebook::fboss
