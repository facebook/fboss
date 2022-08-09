// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/agent/state/TeFlowTable.h"

#include "fboss/agent/state/NodeMap-defs.h"
#include "fboss/agent/state/SwitchState.h"
#include "fboss/agent/state/TeFlowEntry.h"

namespace facebook::fboss {

TeFlowTable::TeFlowTable() {}

TeFlowTable::~TeFlowTable() {}

void toAppend(const TeFlow& flow, std::string* result) {
  std::string flowJson;
  apache::thrift::SimpleJSONSerializer::serialize(flow, &flowJson);
  result->append(flowJson);
}

FBOSS_INSTANTIATE_NODE_MAP(TeFlowTable, TeFlowTableTraits);

} // namespace facebook::fboss
