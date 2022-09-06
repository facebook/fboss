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
#include "fboss/agent/hw/bcm/BcmTeFlowEntry.h"
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

  void processAddedTeFlow(
      const int groupId,
      const std::shared_ptr<TeFlowEntry>& teFlow);
  void processRemovedTeFlow(const std::shared_ptr<TeFlowEntry>& teFlow);
  void releaseTeFlows();

  // Throw exception if not found
  BcmTeFlowEntry* getTeFlow(const std::shared_ptr<TeFlowEntry>& teFlow) const;
  // return nullptr if not found
  BcmTeFlowEntry* getTeFlowIf(const std::shared_ptr<TeFlowEntry>& teFlow) const;
  void processChangedTeFlow(
      const int groupId,
      const std::shared_ptr<TeFlowEntry>& oldTeFlow,
      const std::shared_ptr<TeFlowEntry>& newTeFlow);

 private:
  using BcmTeFlowEntryMap = std::map<TeFlow, std::unique_ptr<BcmTeFlowEntry>>;
  BcmSwitch* hw_;
  bcm_field_hintid_t hintId_{0};
  int teFlowGroupId_{0};
  BcmTeFlowEntryMap teFlowEntryMap_;
};

} // namespace facebook::fboss
