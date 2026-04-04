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

#include "fboss/agent/rib/NextHopIDManager.h"

#include <memory>

namespace facebook::fboss {

class SwitchState;

/*
 * SwitchStateNextHopIdUpdater updates the id2NextHop and id2NextHopSet maps
 * in FibInfo for all switches in the SwitchState. It also provides
 * verifyNextHopIdConsistency to validate consistency between route nexthop
 * IDs and the stored nexthop maps.
 */
class SwitchStateNextHopIdUpdater {
 public:
  explicit SwitchStateNextHopIdUpdater(
      const NextHopIDManager* nextHopIDManager);

  std::shared_ptr<SwitchState> operator()(
      const std::shared_ptr<SwitchState>& state);

  bool verifyNextHopIdConsistency(
      const std::shared_ptr<SwitchState>& state) const;

 private:
  const NextHopIDManager* nextHopIDManager_;
};

} // namespace facebook::fboss
