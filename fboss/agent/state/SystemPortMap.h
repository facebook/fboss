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

#include "fboss/agent/gen-cpp2/switch_state_types.h"
#include "fboss/agent/state/NodeMap.h"
#include "fboss/agent/state/SystemPort.h"
#include "fboss/agent/state/Thrifty.h"
#include "fboss/agent/types.h"

namespace facebook::fboss {

class SwitchState;

using SystemPortMapTraits = NodeMapTraits<SystemPortID, SystemPort>;

/*
 * A container for all the present SystemPorts
 */
class SystemPortMap
    : public ThriftyNodeMapT<
          SystemPortMap,
          SystemPortMapTraits,
          ThriftyNodeMapTraits<int64_t, state::SystemPortFields>> {
 public:
  SystemPortMap();
  ~SystemPortMap() override;

  const std::shared_ptr<SystemPort>& getSystemPort(SystemPortID id) const {
    return getNode(id);
  }
  std::shared_ptr<SystemPort> getSystemPortIf(SystemPortID id) const {
    return getNodeIf(id);
  }

  void addSystemPort(const std::shared_ptr<SystemPort>& systemPort);
  void updateSystemPort(const std::shared_ptr<SystemPort>& systemPort);
  void removeSystemPort(SystemPortID id);
  SystemPortMap* modify(std::shared_ptr<SwitchState>* state);

 private:
  // Inherit the constructors required for clone()
  using ThriftyNodeMapT::ThriftyNodeMapT;
  friend class CloneAllocator;
};
} // namespace facebook::fboss
