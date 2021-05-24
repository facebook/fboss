/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/qsfp_service/test/hw_test/phy/HwPhyEnsembleFactory.h"

#include "fboss/qsfp_service/test/hw_test/phy/HwPhyEnsemble.h"

namespace facebook::fboss {
std::unique_ptr<HwPhyEnsemble> createHwEnsemble(
    std::unique_ptr<HwPhyEnsemble::HwPhyEnsembleInitInfo> initInfo) {
  auto ensemble = std::make_unique<HwPhyEnsemble>();
  ensemble->init(std::move(initInfo));
  return ensemble;
}
} // namespace facebook::fboss
