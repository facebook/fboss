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

using namespace facebook::fboss::phy;

const std::optional<QsfpModule::LengthAndGauge> SffModule::getDACCableOverride()
    const {
  return std::nullopt;
}

const std::optional<DiagsCapability> SffModule::getDiagsCapabilityOverride() {
  return std::nullopt;
}

const std::optional<prbs::InterfacePrbsState>
SffModule::getPortPrbsStateOverrideLocked(Side /* side */) {
  return std::nullopt;
}

const std::optional<bool> SffModule::setPortPrbsOverrideLocked(
    phy::Side /* side */,
    const prbs::InterfacePrbsState& /* prbs */) {
  return std::nullopt;
}

const std::optional<long long> SffModule::getPrbsTotalBitCountOverrideLocked(
    Side /* side */,
    uint8_t /* lane */) {
  return std::nullopt;
}

const std::optional<long long> SffModule::getPrbsBitErrorCountOverrideLocked(
    Side /* side */,
    uint8_t /* lane */) {
  return std::nullopt;
}

const std::optional<int> SffModule::getPrbsLockStatusOverrideLocked(
    Side /* side */) {
  return std::nullopt;
}

const std::optional<phy::PrbsStats> SffModule::getPortPrbsStatsOverrideLocked(
    phy::Side /* side */) {
  return std::nullopt;
}

void SffModule::readSffFr1Field(
    SffField /* field */,
    uint8_t* /* data */,
    bool /* skipPageChange */) {};

void SffModule::writeSffFr1Field(
    SffField /* field */,
    uint8_t* /* data */,
    bool /* skipPageChange */) {};
} // namespace fboss
} // namespace facebook
