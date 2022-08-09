// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/agent/state/TeFlowTable.h"

#include "fboss/agent/state/NodeMap-defs.h"
#include "fboss/agent/state/SwitchState.h"
#include "fboss/agent/state/TeFlowEntry.h"

namespace facebook::fboss {

TeFlowTable::TeFlowTable() {}

TeFlowTable::~TeFlowTable() {}

TeFlowTable* TeFlowTable::addTeFlowEntry(
    std::shared_ptr<SwitchState>* state,
    const FlowEntry& entry) {
  auto* writableTable = modify(state);
  auto oldFlowEntry = writableTable->getTeFlowIf(*entry.flow());

  auto fillFlowInfo = [&entry](auto& tableEntry) {
    tableEntry->setNextHops(*entry.nextHops());
    std::vector<NextHopThrift> resolvedNextHops;
    for (const auto& nexthop : *entry.nextHops()) {
      // TODO - verify that nexthops are reachable/resolved
      resolvedNextHops.emplace_back(nexthop);
    }
    tableEntry->setResolvedNextHops(std::move(resolvedNextHops));
    if (entry.counterID().has_value()) {
      tableEntry->setCounterID(entry.counterID().value());
    } else {
      tableEntry->setCounterID(std::nullopt);
    }
    tableEntry->setEnabled(true);
  };

  if (!oldFlowEntry) {
    auto teFlowEntry = std::make_shared<TeFlowEntry>(*entry.flow());
    fillFlowInfo(teFlowEntry);
    writableTable->addNode(teFlowEntry);
  } else {
    auto* entryToUpdate = oldFlowEntry->modify(state);
    fillFlowInfo(entryToUpdate);
  }
  return writableTable;
}

TeFlowTable* TeFlowTable::removeTeFlowEntry(
    std::shared_ptr<SwitchState>* state,
    const TeFlow& flowId) {
  auto* writableTable = modify(state);
  auto oldFlowEntry = writableTable->getTeFlowIf(flowId);
  if (!oldFlowEntry) {
    std::string flowStr;
    toAppend(flowId, &flowStr);
    throw FbossError("Request to delete a non existing flow entry :", flowStr);
  }
  writableTable->removeNodeIf(flowId);
  return writableTable;
}

TeFlowTable* TeFlowTable::modify(std::shared_ptr<SwitchState>* state) {
  if (!isPublished()) {
    CHECK(!(*state)->isPublished());
    return this;
  }

  auto newTable = clone();
  auto* ptr = newTable.get();
  (*state)->resetTeFlowTable(std::move(newTable));
  return ptr;
}

void toAppend(const TeFlow& flow, std::string* result) {
  std::string flowJson;
  apache::thrift::SimpleJSONSerializer::serialize(flow, &flowJson);
  result->append(flowJson);
}

FBOSS_INSTANTIATE_NODE_MAP(TeFlowTable, TeFlowTableTraits);

} // namespace facebook::fboss
