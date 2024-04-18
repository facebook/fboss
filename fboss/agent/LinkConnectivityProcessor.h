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
#include "fboss/agent/HwSwitchCallback.h"
#include "fboss/agent/types.h"

namespace facebook::fboss {
class SwitchState;
class SwitchIdScopeResolver;
class HwAsicTable;

class LinkConnectivityProcessor {
 public:
  static std::shared_ptr<SwitchState> process(
      const SwitchIdScopeResolver& scopeResolver,
      const HwAsicTable& asicTable,
      const std::shared_ptr<SwitchState>& in,
      const std::map<PortID, multiswitch::FabricConnectivityDelta>&
          connectivityDelta);
};
} // namespace facebook::fboss
