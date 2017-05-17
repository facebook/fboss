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

#include <memory>

extern "C" {
#include <opennsl/types.h>
}

#include "fboss/agent/types.h"

namespace facebook {
namespace fboss {

class AggregatePort;
class BcmSwitch;

class BcmTrunk {
 public:
  enum : opennsl_trunk_t {
    INVALID = -1,
  };

  explicit BcmTrunk(const BcmSwitch* hw) : hw_(hw) {}
  ~BcmTrunk();

  void init(const std::shared_ptr<AggregatePort>& aggPort);
  void program(
      const std::shared_ptr<AggregatePort>& oldAggPort,
      const std::shared_ptr<AggregatePort>& newAggPort);

 private:
  void modifyMemberPortChecked(bool added, PortID memberPort);
  // Forbidden copy constructor and assignment operator
  BcmTrunk(const BcmTrunk&) = delete;
  BcmTrunk& operator=(const BcmTrunk&) = delete;
  opennsl_trunk_t bcmTrunkID_{INVALID};
  const BcmSwitch* const hw_{nullptr};
};
}
} // namespace facebook::fboss
