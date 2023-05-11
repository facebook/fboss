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

int TeFlowSyncer::getDstIpPrefixLength(
    const std::shared_ptr<SwitchState>& state) {
  auto exactMatchTableConfigs =
      state->getSwitchSettings()->getExactMatchTableConfig();
  std::string teFlowTableName(cfg::switch_config_constants::TeFlowTableName());
  auto dstIpPrefixLength = 0;
  for (const auto& tableConfig : *exactMatchTableConfigs) {
    if ((tableConfig->cref<switch_config_tags::name>() == teFlowTableName) &&
        tableConfig->cref<switch_config_tags::dstPrefixLength>().has_value()) {
      dstIpPrefixLength =
          tableConfig->cref<switch_config_tags::dstPrefixLength>()->toThrift();
    }
  }
  if (!dstIpPrefixLength) {
    throw FbossError("Invalid dstIpPrefixLength configuration");
  }
  return dstIpPrefixLength;
}

void TeFlowSyncer::validateFlowEntry(
    const FlowEntry& flowEntry,
    const int& dstIpPrefixLength) {
  if (!flowEntry.flow()->dstPrefix().has_value() ||
      !flowEntry.flow()->srcPort().has_value()) {
    throw FbossError("Invalid dstPrefix or srcPort in TeFlow entry");
  }

  auto prefix = flowEntry.flow()->dstPrefix().value();
  if (*prefix.prefixLength() != dstIpPrefixLength) {
    std::string flowString{};
    folly::IPAddress ipaddr = network::toIPAddress(*prefix.ip());
    flowString.append(fmt::format(
        "dstPrefix:{}/{},srcPort:{}",
        ipaddr.str(),
        *prefix.prefixLength(),
        flowEntry.flow()->srcPort().value()));
    throw FbossError("Invalid prefix length in TeFlow entry: ", flowString);
  }
}

std::shared_ptr<SwitchState> TeFlowSyncer::addDelTeFlows(
    const HwSwitchMatcher& matcher,
    const std::shared_ptr<SwitchState>& state,
    const std::vector<FlowEntry>& entriesToAdd,
    const std::vector<TeFlow>& entriesToDel) {
  auto newState = state->clone();
  auto teFlowTable = newState->getTeFlowTable()->modify(&newState);
  auto numAddFlows = entriesToAdd.size();
  auto numDelFlows = entriesToDel.size();
  int dstIpPrefixLength = 0;
  if (numAddFlows) {
    dstIpPrefixLength = getDstIpPrefixLength(state);
  }

  // Add TeFlows
  for (const auto& flowEntry : entriesToAdd) {
    validateFlowEntry(flowEntry, dstIpPrefixLength);
    auto oldTeFlowEntry =
        teFlowTable->getNodeIf(getTeFlowStr(*flowEntry.flow()));
    auto newTeFlowEntry = TeFlowEntry::createTeFlowEntry(flowEntry);
    newTeFlowEntry->resolve(newState);
    if (!oldTeFlowEntry) {
      teFlowTable->addNode(newTeFlowEntry, matcher);
    } else {
      teFlowTable->updateNode(newTeFlowEntry, matcher);
    }
  }
  XLOG(DBG2) << "addTeFlows Added : " << numAddFlows;

  // Delete TeFlows
  for (const auto& flow : entriesToDel) {
    teFlowTable->removeNode(getTeFlowStr(flow));
  }
  XLOG(DBG2) << "deleteTeFlows Deleted : " << numDelFlows;
  return newState;
}

std::shared_ptr<SwitchState> TeFlowSyncer::syncTeFlows(
    const HwSwitchMatcher& matcher,
    const std::shared_ptr<SwitchState>& state,
    const std::vector<FlowEntry>& flowEntries) {
  auto numFlows = flowEntries.size();
  auto oldNumFlows = state->getTeFlowTable()->numNodes();
  auto newState = state->clone();
  auto newTeFlowTable = std::make_shared<MultiTeFlowTable>();
  newState->resetTeFlowTable(newTeFlowTable);
  auto teFlowTable = state->getTeFlowTable();
  auto dstIpPrefixLength = getDstIpPrefixLength(state);
  bool tableChanged = false;

  for (const auto& flowEntry : flowEntries) {
    validateFlowEntry(flowEntry, dstIpPrefixLength);
    TeFlow flow = *flowEntry.flow();
    auto oldTeFlowEntry = teFlowTable->getNodeIf(getTeFlowStr(flow));
    auto newTeFlowEntry = TeFlowEntry::createTeFlowEntry(flowEntry);
    newTeFlowEntry->resolve(newState);
    // new entry add it
    if (!oldTeFlowEntry) {
      newTeFlowTable->addNode(newTeFlowEntry, matcher);
      tableChanged = true;
    } else {
      // if entries are same add the old entry to the new table
      // else add the new entry to the new table
      if (*oldTeFlowEntry == *newTeFlowEntry) {
        newTeFlowTable->addNode(oldTeFlowEntry, matcher);
      } else {
        newTeFlowTable->addNode(newTeFlowEntry, matcher);
        tableChanged = true;
      }
    }
  }

  if (numFlows != oldNumFlows) {
    tableChanged = true;
  }
  XLOG(DBG2) << "syncTeFlows newFlowCount :" << numFlows
             << " oldFlowCount :" << oldNumFlows;
  if (!tableChanged) {
    return state;
  }
  return newState;
}

std::shared_ptr<SwitchState> TeFlowSyncer::programFlowEntries(
    const HwSwitchMatcher& matcher,
    const std::shared_ptr<SwitchState>& state,
    const std::vector<FlowEntry>& addTeFlows,
    const std::vector<TeFlow>& delTeFlows,
    bool isSync) {
  if (isSync) {
    XLOG(DBG3) << "Sync TeFlows";
    return syncTeFlows(matcher, state, addTeFlows);
  }
  XLOG(DBG3) << "Add or delete TeFlows";
  return addDelTeFlows(matcher, state, addTeFlows, delTeFlows);
}

MultiTeFlowTable* MultiTeFlowTable::modify(
    std::shared_ptr<SwitchState>* state) {
  if (!isPublished()) {
    CHECK(!(*state)->isPublished());
    return this;
  }

  SwitchState::modify(state);
  auto map = clone();
  for (auto mnitr = cbegin(); mnitr != cend(); ++mnitr) {
    (*map)[mnitr->first] = mnitr->second->clone();
  }
  auto* ptr = map.get();
  (*state)->resetTeFlowTable(std::move(map));
  return ptr;
}

template class ThriftMapNode<TeFlowTable, TeFlowTableThriftTraits>;

} // namespace facebook::fboss
