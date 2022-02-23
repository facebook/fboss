/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/qsfp_service/module/sff/SffModule.h"

namespace facebook {
namespace fboss {

const std::optional<QsfpModule::LengthAndGauge> SffModule::getDACCableOverride()
    const {
  return std::nullopt;
}

const std::optional<DiagsCapability> SffModule::getDiagsCapabilityOverride() {
  return std::nullopt;
}

const std::optional<phy::PortPrbsState>
SffModule::getPortPrbsStateOverrideLocked(Side /* side */) {
  return std::nullopt;
}

const std::optional<bool> SffModule::setPortPrbsOverrideLocked(
    phy::Side /* side */,
    const phy::PortPrbsState& /* prbs */) {
  return std::nullopt;
}

} // namespace fboss
} // namespace facebook
