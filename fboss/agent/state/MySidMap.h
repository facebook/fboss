// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include "fboss/agent/gen-cpp2/switch_state_types.h"
#include "fboss/agent/state/MySid.h"
#include "fboss/agent/state/NodeMap.h"
#include "fboss/agent/state/Thrifty.h"
#include "fboss/agent/types.h"

namespace facebook::fboss {

class SwitchState;

using MySidMapLegacyTraits = NodeMapTraits<std::string, MySid>;

using MySidMapClass = apache::thrift::type_class::map<
    apache::thrift::type_class::string,
    apache::thrift::type_class::structure>;
using MySidMapThriftType = std::map<std::string, state::MySidFields>;

class MySidMap;
using MySidMapTraits =
    ThriftMapNodeTraits<MySidMap, MySidMapClass, MySidMapThriftType, MySid>;

/*
 * A container for all MySid entries
 */
class MySidMap : public ThriftMapNode<MySidMap, MySidMapTraits> {
 public:
  using Base = ThriftMapNode<MySidMap, MySidMapTraits>;
  using Traits = MySidMapTraits;
  MySidMap();
  ~MySidMap() override;

  const std::shared_ptr<MySid> getMySid(const std::string& id) const {
    return getNode(id);
  }
  std::shared_ptr<MySid> getMySidIf(const std::string& id) const {
    return getNodeIf(id);
  }

  void addMySid(const std::shared_ptr<MySid>& mySid);
  void updateMySid(const std::shared_ptr<MySid>& mySid);
  void removeMySid(const std::string& id);

 private:
  // Inherit the constructors required for clone()
  using Base::Base;
  friend class CloneAllocator;
};

using MultiSwitchMySidMapTypeClass = apache::thrift::type_class::
    map<apache::thrift::type_class::string, MySidMapClass>;
using MultiSwitchMySidMapThriftType = std::map<std::string, MySidMapThriftType>;

class MultiSwitchMySidMap;

using MultiSwitchMySidMapTraits = ThriftMultiSwitchMapNodeTraits<
    MultiSwitchMySidMap,
    MultiSwitchMySidMapTypeClass,
    MultiSwitchMySidMapThriftType,
    MySidMap>;

class HwSwitchMatcher;

class MultiSwitchMySidMap : public ThriftMultiSwitchMapNode<
                                MultiSwitchMySidMap,
                                MultiSwitchMySidMapTraits> {
 public:
  using Traits = MultiSwitchMySidMapTraits;
  using BaseT =
      ThriftMultiSwitchMapNode<MultiSwitchMySidMap, MultiSwitchMySidMapTraits>;
  using BaseT::modify;

  MultiSwitchMySidMap() = default;
  virtual ~MultiSwitchMySidMap() = default;

  MultiSwitchMySidMap* modify(std::shared_ptr<SwitchState>* state);

 private:
  // Inherit the constructors required for clone()
  using BaseT::BaseT;
  friend class CloneAllocator;
};

} // namespace facebook::fboss
