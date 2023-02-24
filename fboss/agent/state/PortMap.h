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
#include "fboss/agent/state/Port.h"
#include "fboss/agent/state/Thrifty.h"
#include "fboss/agent/types.h"

namespace facebook::fboss {

class SwitchState;
class Port;

using PortMapTypeClass = apache::thrift::type_class::map<
    apache::thrift::type_class::integral,
    apache::thrift::type_class::structure>;
using PortMapThriftType = std::map<int16_t, state::PortFields>;

class PortMap;
using PortMapTraits =
    ThriftMapNodeTraits<PortMap, PortMapTypeClass, PortMapThriftType, Port>;

/*
 * A container for the set of ports.
 */
class PortMap : public ThriftMapNode<PortMap, PortMapTraits> {
 public:
  using Base = ThriftMapNode<PortMap, PortMapTraits>;

  PortMap();
  ~PortMap() override;

  const std::shared_ptr<Port>& getPort(PortID id) const {
    return getNode(id);
  }
  std::shared_ptr<Port> getPortIf(PortID id) const {
    return getNodeIf(id);
  }
  std::shared_ptr<Port> getPort(const std::string& name) const;
  std::shared_ptr<Port> getPortIf(const std::string& name) const;

  size_t numPorts() const {
    return size();
  }

  /*
   * The following functions modify the static state.
   * These should only be called on unpublished objects which are only visible
   * to a single thread.
   */

  void registerPort(
      PortID id,
      const std::string& name,
      cfg::PortType portType = cfg::PortType::INTERFACE_PORT);

  void addPort(const std::shared_ptr<Port>& port);

  void updatePort(const std::shared_ptr<Port>& port);

  PortMap* modify(std::shared_ptr<SwitchState>* state);

 private:
  // Inherit the constructors required for clone()
  using Base::Base;
  friend class CloneAllocator;
};

} // namespace facebook::fboss
