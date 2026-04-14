// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/agent/state/MySidMap.h"

#include "fboss/agent/state/MySid.h"
#include "fboss/agent/state/NodeMap-defs.h"
#include "fboss/agent/state/SwitchState.h"

namespace facebook::fboss {

MySidMap::MySidMap() = default;

MySidMap::~MySidMap() = default;

void MySidMap::addMySid(const std::shared_ptr<MySid>& mySid) {
  addNode(mySid);
}

void MySidMap::updateMySid(const std::shared_ptr<MySid>& mySid) {
  updateNode(mySid);
}

void MySidMap::removeMySid(const std::string& id) {
  removeNodeIf(id);
}

MultiSwitchMySidMap* MultiSwitchMySidMap::modify(
    std::shared_ptr<SwitchState>* state) {
  return SwitchState::modify<switch_state_tags::mySidMaps>(state);
}

template struct ThriftMapNode<MySidMap, MySidMapTraits>;

} // namespace facebook::fboss
