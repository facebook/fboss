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

#include "fboss/agent/state/ClassBasedPolicyNode.h"
#include "fboss/agent/state/Thrifty.h"
#include "fboss/agent/types.h"

namespace facebook::fboss {

using ClassBasedPolicyMapTypeClass = apache::thrift::type_class::map<
    apache::thrift::type_class::string,
    apache::thrift::type_class::structure>;
using ClassBasedPolicyMapThriftType =
    std::map<std::string, state::ClassBasedPolicyFields>;

class ClassBasedPolicyMap;
using ClassBasedPolicyMapTraits = ThriftMapNodeTraits<
    ClassBasedPolicyMap,
    ClassBasedPolicyMapTypeClass,
    ClassBasedPolicyMapThriftType,
    ClassBasedPolicyNode>;

/*
 * A container for the set of PBR class-based policies, keyed by policy name.
 */
class ClassBasedPolicyMap
    : public ThriftMapNode<ClassBasedPolicyMap, ClassBasedPolicyMapTraits> {
 public:
  using Base = ThriftMapNode<ClassBasedPolicyMap, ClassBasedPolicyMapTraits>;
  using Traits = ClassBasedPolicyMapTraits;

  ClassBasedPolicyMap();
  ~ClassBasedPolicyMap() override;

  const std::shared_ptr<ClassBasedPolicyNode>& getPolicy(
      const std::string& name) const {
    return getNode(name);
  }
  std::shared_ptr<ClassBasedPolicyNode> getPolicyIf(
      const std::string& name) const {
    return getNodeIf(name);
  }

  void addPolicy(const std::shared_ptr<ClassBasedPolicyNode>& policy) {
    addNode(policy);
  }
  void removePolicy(const std::string& name) {
    removeNode(getNode(name));
  }

 private:
  // Inherit the constructors required for clone()
  using Base::Base;
  friend class CloneAllocator;
};

using MultiSwitchClassBasedPolicyMapTypeClass = apache::thrift::type_class::
    map<apache::thrift::type_class::string, ClassBasedPolicyMapTypeClass>;
using MultiSwitchClassBasedPolicyMapThriftType =
    std::map<std::string, ClassBasedPolicyMapThriftType>;

class MultiSwitchClassBasedPolicyMap;

using MultiSwitchClassBasedPolicyMapTraits = ThriftMultiSwitchMapNodeTraits<
    MultiSwitchClassBasedPolicyMap,
    MultiSwitchClassBasedPolicyMapTypeClass,
    MultiSwitchClassBasedPolicyMapThriftType,
    ClassBasedPolicyMap>;

class HwSwitchMatcher;

class MultiSwitchClassBasedPolicyMap
    : public ThriftMultiSwitchMapNode<
          MultiSwitchClassBasedPolicyMap,
          MultiSwitchClassBasedPolicyMapTraits> {
 public:
  using Traits = MultiSwitchClassBasedPolicyMapTraits;
  using BaseT = ThriftMultiSwitchMapNode<
      MultiSwitchClassBasedPolicyMap,
      MultiSwitchClassBasedPolicyMapTraits>;
  using BaseT::modify;

  MultiSwitchClassBasedPolicyMap() = default;
  virtual ~MultiSwitchClassBasedPolicyMap() = default;

  MultiSwitchClassBasedPolicyMap* modify(std::shared_ptr<SwitchState>* state);

 private:
  // Inherit the constructors required for clone()
  using BaseT::BaseT;
  friend class CloneAllocator;
};

} // namespace facebook::fboss
