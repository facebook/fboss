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

#include "fboss/agent/hw/switch_asics/ChenabAsic.h"

namespace facebook::fboss {

class Chenab2Asic : public ChenabAsic {
 public:
  using ChenabAsic::ChenabAsic;
  cfg::AsicType getAsicType() const override;
  uint64_t getMMUSizeBytes() const override;
  int getMaxNumLogicalPorts() const override;
  uint32_t getPacketBufferUnitSize() const override;
  uint32_t getMaxEcmpSize() const override;
  std::optional<uint32_t> getMaxEcmpMembers() const override;
  std::optional<uint32_t> getMaxArsGroups() const override;
};

} // namespace facebook::fboss
