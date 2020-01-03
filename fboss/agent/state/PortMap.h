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

#include "fboss/agent/state/NodeMap.h"
#include "fboss/agent/types.h"

namespace facebook::fboss {

class SwitchState;
class Port;
typedef NodeMapTraits<PortID, Port> PortMapTraits;

/*
 * A container for the set of ports.
 */
class PortMap : public NodeMapT<PortMap, PortMapTraits> {
 public:
  PortMap();
  ~PortMap() override;

  const std::shared_ptr<Port>& getPort(PortID id) const {
    return getNode(id);
  }
  std::shared_ptr<Port> getPortIf(PortID id) const {
    return getNodeIf(id);
  }

  size_t numPorts() const {
    return size();
  }

  /*
   * The following functions modify the static state.
   * These should only be called on unpublished objects which are only visible
   * to a single thread.
   */

  void registerPort(PortID id, const std::string& name);

  void addPort(const std::shared_ptr<Port>& port);

  void updatePort(const std::shared_ptr<Port>& port);

  PortMap* modify(std::shared_ptr<SwitchState>* state);

 private:
  // Inherit the constructors required for clone()
  using NodeMapT::NodeMapT;
  friend class CloneAllocator;
};

} // namespace facebook::fboss
