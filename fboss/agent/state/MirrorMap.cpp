// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/agent/state/MirrorMap.h"
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

void MultiMirrorMap::addMirrorMap(
    const HwSwitchMatcher& matcher,
    std::shared_ptr<MirrorMap> mirrorMap) {
  CHECK(mirrorMap);
  insert(matcher.matcherString(), std::move(mirrorMap));
}

void MultiMirrorMap::changeMirrorMap(
    const HwSwitchMatcher& matcher,
    std::shared_ptr<MirrorMap> mirrorMap) {
  CHECK(mirrorMap);
  ref(matcher.matcherString()) = mirrorMap;
}

void MultiMirrorMap::removeMirrorMap(const HwSwitchMatcher& matcher) {
  if (!getMirrorMapIf(matcher)) {
    return;
  }
  remove(matcher.matcherString());
}

template class ThriftMapNode<MirrorMap, MirrorMapTraits>;

} // namespace facebook::fboss
