// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/agent/state/TeFlowTable.h"

#include "fboss/agent/FbossError.h"
#include "fboss/agent/state/NodeMap-defs.h"
#include "fboss/agent/state/SwitchState.h"
#include "fboss/agent/state/TeFlowEntry.h"

namespace facebook::fboss {

TeFlowTable::TeFlowTable() {}

TeFlowTable::~TeFlowTable() {}

void TeFlowTable::addTeFlowEntry(
    const std::shared_ptr<TeFlowEntry>& teFlowEntry) {
  XLOG(DBG3) << "Adding TeFlow " << teFlowEntry->str();
  return addNode(teFlowEntry);
}

void TeFlowTable::changeTeFlowEntry(
    const std::shared_ptr<TeFlowEntry>& teFlowEntry) {
  XLOG(DBG3) << "Updating TeFlow " << teFlowEntry->str();
  updateNode(teFlowEntry);
}

void TeFlowTable::removeTeFlowEntry(const TeFlow& flowId) {
  auto id = getTeFlowStr(flowId);
  auto oldFlowEntry = getTeFlowIf(flowId);
  if (!oldFlowEntry) {
    XLOG(ERR) << "Request to delete a non existing flow entry :" << id;
    return;
  }
  removeNodeIf(id);
  XLOG(DBG3) << "Deleting TeFlow " << oldFlowEntry->str();
}

TeFlowTable* TeFlowTable::modify(std::shared_ptr<SwitchState>* state) {
  if (!isPublished()) {
    CHECK(!(*state)->isPublished());
    return this;
  }

  SwitchState::modify(state);
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

std::string getTeFlowStr(const TeFlow& flow) {
  std::string flowStr;
  toAppend(flow, &flowStr);
  return flowStr;
}

template class ThriftMapNode<TeFlowTable, TeFlowTableThriftTraits>;

} // namespace facebook::fboss
