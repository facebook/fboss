// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/agent/state/MirrorMap.h"
#include <utility>
#include "fboss/agent/state/Mirror.h"
#include "fboss/agent/state/NodeMap-defs.h"
#include "fboss/agent/state/SwitchState.h"

#include "fboss/agent/NpuMatcher.h"

namespace facebook::fboss {

std::shared_ptr<Mirror> MirrorMap::getMirrorIf(const std::string& name) const {
  return getNodeIf(name);
}

void MirrorMap::addMirror(const std::shared_ptr<Mirror>& mirror) {
  return addNode(mirror);
}

MirrorMap* MirrorMap::modify(std::shared_ptr<SwitchState>* state) {
  if (!isPublished()) {
    CHECK(!(*state)->isPublished());
    return this;
  }

  SwitchState::modify(state);
  auto newMirrors = clone();
  auto* ptr = newMirrors.get();
  (*state)->resetMirrors(std::move(newMirrors));
  return ptr;
}

std::shared_ptr<const MirrorMap> MultiMirrorMap::getMirrorMapIf(
    const HwSwitchMatcher& matcher) const {
  auto iter = std::as_const(*this).find(matcher.matcherString());
  if (iter == cend()) {
    return nullptr;
  }
  return iter->second;
}

template class ThriftMapNode<MirrorMap, MirrorMapTraits>;

} // namespace facebook::fboss
