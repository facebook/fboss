/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/state/FlowletSwitchingConfig.h"
#include "fboss/agent/state/SwitchState.h"

namespace facebook::fboss {

template struct ThriftStructNode<
    FlowletSwitchingConfig,
    cfg::FlowletSwitchingConfig>;

FlowletSwitchingConfig* FlowletSwitchingConfig::modify(
    std::shared_ptr<SwitchState>* state) {
  if (!isPublished()) {
    CHECK(!(*state)->isPublished());
    return this;
  }
  auto cloned = clone();
  auto mSwitchSettings = (*state)->getSwitchSettings()->clone();
  for (const auto& [_, switchSettings] : *std::as_const(mSwitchSettings)) {
    if (this == switchSettings->getFlowletSwitchingConfig().get()) {
      auto switchSettingsWritable = switchSettings->modify(state);
      switchSettingsWritable->setFlowletSwitchingConfig(cloned);
      return cloned.get();
    }
  }
  throw FbossError("Flowlet swtich settings not part of input state");
}
} // namespace facebook::fboss
