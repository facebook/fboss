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

#include "fboss/agent/hw/switch_asics/HwAsic.h"
#include "fboss/agent/platforms/common/PlatformMapping.h"

namespace facebook::fboss {

class Meru800biaPlatformMapping : public PlatformMapping {
 public:
  Meru800biaPlatformMapping();
  explicit Meru800biaPlatformMapping(const std::string& platformMappingStr);
  explicit Meru800biaPlatformMapping(HwAsic::InterfaceNodeRole intfRole);
  // For CPU system port number as key, get the core for CPU port and
  // the port ID within the core.
  virtual std::map<uint32_t, std::pair<uint32_t, uint32_t>>
  getCpuPortsCoreAndPortIdx() const override;

 private:
  // Forbidden copy constructor and assignment operator
  Meru800biaPlatformMapping(Meru800biaPlatformMapping const&) = delete;
  Meru800biaPlatformMapping& operator=(Meru800biaPlatformMapping const&) =
      delete;
};
} // namespace facebook::fboss
