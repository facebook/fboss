// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/agent/state/MirrorMap.h"
#include "fboss/agent/state/Mirror.h"
#include "fboss/agent/state/NodeMap-defs.h"
#include "fboss/agent/state/SwitchState.h"

#include "fboss/agent/HwSwitchMatcher.h"

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

void MultiMirrorMap::addNode(
    std::shared_ptr<Mirror> mirror,
    const HwSwitchMatcher& matcher) {
  const auto& key = matcher.matcherString();
  auto mitr = find(key);
  if (mitr == end()) {
    mitr = insert(key, std::make_shared<MirrorMap>()).first;
  }
  auto& mirrorMap = mitr->second;
  mirrorMap->addMirror(std::move(mirror));
}

template class ThriftMapNode<MirrorMap, MirrorMapTraits>;

} // namespace facebook::fboss
