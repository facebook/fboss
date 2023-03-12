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
#include "fboss/agent/state/NodeMapDelta.h"
#include "fboss/agent/state/QosPolicy.h"
#include "fboss/agent/state/Thrifty.h"
#include "fboss/agent/types.h"

#include <memory>

namespace facebook::fboss {

using QosPolicyMapLegacyTraits = NodeMapTraits<std::string, QosPolicy>;

using QosPolicyMapTypeClass = apache::thrift::type_class::map<
    apache::thrift::type_class::string,
    apache::thrift::type_class::structure>;
using QosPolicyMapThriftType = std::map<std::string, state::QosPolicyFields>;
class QosPolicyMap;
using QosPolicyMapTraits = ThriftMapNodeTraits<
    QosPolicyMap,
    QosPolicyMapTypeClass,
    QosPolicyMapThriftType,
    QosPolicy>;

class QosPolicyMap : public ThriftMapNode<QosPolicyMap, QosPolicyMapTraits> {
 public:
  using BaseT = ThriftMapNode<QosPolicyMap, QosPolicyMapTraits>;
  using BaseT::modify;
  QosPolicyMap();
  ~QosPolicyMap() override;

  std::shared_ptr<QosPolicy> getQosPolicyIf(const std::string& name) const {
    return getNodeIf(name);
  }

  QosPolicyMap* modify(std::shared_ptr<SwitchState>* state);

 private:
  // Inherit the constructors required for clone()
  using BaseT::BaseT;
  friend class CloneAllocator;
};

using QosPolicyMapDelta = thrift_cow::ThriftMapDelta<QosPolicyMap>;

} // namespace facebook::fboss
