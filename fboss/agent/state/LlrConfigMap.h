#pragma once

#include "fboss/agent/gen-cpp2/switch_state_types.h"
#include "fboss/agent/state/LlrConfig.h"
#include "fboss/agent/state/NodeMap.h"
#include "fboss/agent/state/Thrifty.h"
#include "fboss/agent/types.h"

namespace facebook::fboss {

using LlrConfigMapTypeClass = apache::thrift::type_class::map<
    apache::thrift::type_class::string,
    apache::thrift::type_class::structure>;
using LlrConfigMapThriftType = std::map<std::string, state::LlrFields>;

class LlrConfigMap;
using LlrConfigMapTraits = ThriftMapNodeTraits<
    LlrConfigMap,
    LlrConfigMapTypeClass,
    LlrConfigMapThriftType,
    LlrConfig>;

/*
 * A container for named LlrConfig profiles.
 */
class LlrConfigMap : public ThriftMapNode<LlrConfigMap, LlrConfigMapTraits> {
 public:
  using Base = ThriftMapNode<LlrConfigMap, LlrConfigMapTraits>;
  using Traits = LlrConfigMapTraits;
  LlrConfigMap() = default;
  virtual ~LlrConfigMap() = default;

 private:
  // Inherit the constructors required for clone()
  using Base::Base;
  friend class CloneAllocator;
};

using MultiSwitchLlrConfigMapTypeClass = apache::thrift::type_class::
    map<apache::thrift::type_class::string, LlrConfigMapTypeClass>;
using MultiSwitchLlrConfigMapThriftType =
    std::map<std::string, LlrConfigMapThriftType>;

class MultiSwitchLlrConfigMap;

using MultiSwitchLlrConfigMapTraits = ThriftMultiSwitchMapNodeTraits<
    MultiSwitchLlrConfigMap,
    MultiSwitchLlrConfigMapTypeClass,
    MultiSwitchLlrConfigMapThriftType,
    LlrConfigMap>;

class HwSwitchMatcher;

class MultiSwitchLlrConfigMap : public ThriftMultiSwitchMapNode<
                                    MultiSwitchLlrConfigMap,
                                    MultiSwitchLlrConfigMapTraits> {
 public:
  using Traits = MultiSwitchLlrConfigMapTraits;
  using BaseT = ThriftMultiSwitchMapNode<
      MultiSwitchLlrConfigMap,
      MultiSwitchLlrConfigMapTraits>;
  using BaseT::modify;

  MultiSwitchLlrConfigMap() = default;
  virtual ~MultiSwitchLlrConfigMap() = default;

  MultiSwitchLlrConfigMap* modify(std::shared_ptr<SwitchState>* state);

 private:
  // Inherit the constructors required for clone()
  using BaseT::BaseT;
  friend class CloneAllocator;
};

} // namespace facebook::fboss
