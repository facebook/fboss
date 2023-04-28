// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include <string>

#include "fboss/agent/gen-cpp2/switch_state_types.h"
#include "fboss/agent/state/NodeMap.h"
#include "fboss/agent/state/TeFlowEntry.h"
#include "fboss/agent/state/Thrifty.h"
#include "fboss/agent/types.h"

namespace facebook::fboss {

void toAppend(const TeFlow& flow, std::string* result);
std::string getTeFlowStr(const TeFlow& flow);

class SwitchState;

using TeFlowTableTraits = NodeMapTraits<TeFlow, TeFlowEntry>;

using TeFlowTableTypeClass = apache::thrift::type_class::map<
    apache::thrift::type_class::string,
    apache::thrift::type_class::structure>;
using TeFlowTableThriftType = std::map<std::string, state::TeFlowEntryFields>;

class TeFlowTable;
using TeFlowTableThriftTraits = ThriftMapNodeTraits<
    TeFlowTable,
    TeFlowTableTypeClass,
    TeFlowTableThriftType,
    TeFlowEntry>;

/*
 * A container for TE flow entries
 */
class TeFlowTable : public ThriftMapNode<TeFlowTable, TeFlowTableThriftTraits> {
 public:
  using Base = ThriftMapNode<TeFlowTable, TeFlowTableThriftTraits>;
  using Traits = TeFlowTableThriftTraits;
  using Base::modify;

  TeFlowTable();
  ~TeFlowTable() override;

  const std::shared_ptr<TeFlowEntry>& getTeFlow(TeFlow id) const {
    return this->cref(getTeFlowStr(id));
  }

  std::shared_ptr<TeFlowEntry> getTeFlowIf(TeFlow id) const {
    return getNodeIf(getTeFlowStr(id));
  }

  void addTeFlowEntry(const std::shared_ptr<TeFlowEntry>& teFlowEntry);
  void changeTeFlowEntry(const std::shared_ptr<TeFlowEntry>& teFlowEntry);
  void removeTeFlowEntry(const TeFlow& id);
  TeFlowTable* modify(std::shared_ptr<SwitchState>* state);

 private:
  // Inherit the constructors required for clone()
  using Base::Base;
  friend class CloneAllocator;
};

class TeFlowSyncer {
 public:
  std::shared_ptr<SwitchState> programFlowEntries(
      const std::shared_ptr<SwitchState>& state,
      const std::vector<FlowEntry>& addTeFlows,
      const std::vector<TeFlow>& delTeFlows,
      bool isSync);

 private:
  int getDstIpPrefixLength(const std::shared_ptr<SwitchState>& state);
  void validateFlowEntry(
      const FlowEntry& flowEntry,
      const int& dstIpPrefixLength);
  std::shared_ptr<SwitchState> addDelTeFlows(
      const std::shared_ptr<SwitchState>& state,
      const std::vector<FlowEntry>& entriesToAdd,
      const std::vector<TeFlow>& entriesToDel);
  std::shared_ptr<SwitchState> syncTeFlows(
      const std::shared_ptr<SwitchState>& state,
      const std::vector<FlowEntry>& flowEntries);
};

using MultiTeFlowTableTypeClass = apache::thrift::type_class::
    map<apache::thrift::type_class::string, TeFlowTableTypeClass>;
using MultiTeFlowTableThriftType = std::map<std::string, TeFlowTableThriftType>;

class MultiTeFlowTable;

using MultiTeFlowTableTraits = ThriftMultiSwitchMapNodeTraits<
    MultiTeFlowTable,
    MultiTeFlowTableTypeClass,
    MultiTeFlowTableThriftType,
    TeFlowTable>;

class HwSwitchMatcher;

class MultiTeFlowTable : public ThriftMultiSwitchMapNode<
                             MultiTeFlowTable,
                             MultiTeFlowTableTraits> {
 public:
  using Traits = MultiTeFlowTableTraits;
  using BaseT =
      ThriftMultiSwitchMapNode<MultiTeFlowTable, MultiTeFlowTableTraits>;
  using BaseT::modify;

  MultiTeFlowTable() {}
  virtual ~MultiTeFlowTable() {}

 private:
  // Inherit the constructors required for clone()
  using BaseT::BaseT;
  friend class CloneAllocator;
};

} // namespace facebook::fboss
