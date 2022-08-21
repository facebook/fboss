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

extern "C" {
#include <bcm/field.h>
}

#include "fboss/agent/Utils.h"
#include "fboss/agent/hw/bcm/types.h"

namespace facebook::fboss {

class BcmSwitch;

/**
 * A class to keep state related to TeFlow entries in BcmSwitch
 */
class BcmTeFlowTable {
 public:
  explicit BcmTeFlowTable(BcmSwitch* hw) : hw_(hw) {}
  ~BcmTeFlowTable();
  void createTeFlowGroup();
  bcm_field_hintid_t getHintId() const {
    return hintId_;
  }

 private:
  BcmSwitch* hw_;
  bcm_field_hintid_t hintId_{0};
  int teFlowGroupId_{0};
};

} // namespace facebook::fboss
