// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include <string>
#pragma once

#include "fboss/agent/gen-cpp2/switch_state_types.h"
#include "fboss/agent/state/NodeMap.h"
#include "fboss/agent/state/TeFlowEntry.h"
#include "fboss/agent/state/Thrifty.h"
#include "fboss/agent/types.h"

namespace facebook::fboss {

class SwitchState;

using TeFlowTableTraits = NodeMapTraits<TeFlow, TeFlowEntry>;

struct TeFlowTableThriftTraits
    : public ThriftyNodeMapTraits<std::string, state::TeFlowEntryFields> {
  static inline const std::string& getThriftKeyName() {
    static const std::string _key = "flow";
    return _key;
  }
  static const KeyType parseKey(const folly::dynamic& key) {
    return key.asString();
  }
  static const KeyType convertKey(const TeFlow& key) {
    std::string flowJson;
    apache::thrift::SimpleJSONSerializer::serialize(key, &flowJson);
    return flowJson;
  }
};

/*
 * A container for TE flow entries
 */
class TeFlowTable : public ThriftyNodeMapT<
                        TeFlowTable,
                        TeFlowTableTraits,
                        TeFlowTableThriftTraits> {
 public:
  TeFlowTable();
  ~TeFlowTable() override;

  const std::shared_ptr<TeFlowEntry>& getTeFlow(TeFlow id) const {
    return getNode(id);
  }
  std::shared_ptr<TeFlowEntry> getTeFlowIf(TeFlow id) const {
    return getNodeIf(id);
  }
  TeFlowTable* addTeFlowEntry(
      std::shared_ptr<SwitchState>* state,
      const FlowEntry& entry);
  TeFlowTable* removeTeFlowEntry(
      std::shared_ptr<SwitchState>* state,
      const TeFlow& id);
  TeFlowTable* modify(std::shared_ptr<SwitchState>* state);

 private:
  // Inherit the constructors required for clone()
  using ThriftyNodeMapT::ThriftyNodeMapT;
  friend class CloneAllocator;
};
void toAppend(const TeFlow& flow, std::string* result);
} // namespace facebook::fboss
