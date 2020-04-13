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

#include <cstdint>
#include <memory>
#include <vector>

#include "fboss/agent/hw/bcm/BcmQosPolicy.h"
#include "fboss/agent/hw/bcm/RxUtils.h"

extern "C" {
#include <bcm/cosq.h>
#include <bcm/port.h>
}

namespace facebook::fboss {

class PortID;
class BcmSwitch;

class BcmCosManager {
 public:
  explicit BcmCosManager(BcmSwitch* hw);

  bcm_port_t getPhysicalCpuPort() const;

  void enableBst();

  void disableBst();

  void profileSet(PortID port, int32_t cosq, int32_t bid, int32_t threshold);

  int32_t profileGet(PortID port, int32_t cosq, int32_t bid);

  void statClear(PortID port, int32_t cosq, int32_t bid);

  uint64_t
  statGet(PortID port, int32_t cosq, int32_t bid, bool clearAfter = true);

  void deviceStatClear(int32_t bid);
  uint64_t deviceStatGet(int32_t bid, bool clearAfter = true);

 private:
  const BcmSwitch* hw_;
};

} // namespace facebook::fboss
