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

std::shared_ptr<MirrorMap> MirrorMap::fromThrift(
    const std::map<std::string, state::MirrorFields>& mirrors) {
  auto map = std::make_shared<MirrorMap>();
  for (auto mirror : mirrors) {
    auto node = std::make_shared<Mirror>();
    node->fromThrift(mirror.second);
    // TODO(pshaikh): make this private
    node->markResolved();
    map->insert(*mirror.second.name(), std::move(node));
  }
  return map;
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

std::shared_ptr<Mirror> MultiMirrorMap::getMirrorIf(
    const std::string& name) const {
  for (auto mnitr = cbegin(); mnitr != cend(); ++mnitr) {
    auto node = mnitr->second->getNodeIf(name);
    if (node) {
      return node;
    }
  }
  return nullptr;
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

void MultiMirrorMap::updateNode(
    std::shared_ptr<Mirror> mirror,
    const HwSwitchMatcher& matcher) {
  const auto& key = matcher.matcherString();
  auto mitr = find(key);
  if (mitr == end()) {
    throw FbossError("No mirrors found for switchIds: ", key);
  }
  auto& mirrorMap = mitr->second;
  mirrorMap->updateNode(std::move(mirror));
}

MultiMirrorMap* MultiMirrorMap::modify(std::shared_ptr<SwitchState>* state) {
  if (!isPublished()) {
    CHECK(!(*state)->isPublished());
    return this;
  }

  SwitchState::modify(state);
  auto newMnpuMirrors = clone();
  for (auto mnitr = cbegin(); mnitr != cend(); ++mnitr) {
    (*newMnpuMirrors)[mnitr->first] = mnitr->second->clone();
  }
  auto* ptr = newMnpuMirrors.get();
  (*state)->resetMirrors(std::move(newMnpuMirrors));
  return ptr;
}

std::shared_ptr<MultiMirrorMap> MultiMirrorMap::fromThrift(
    const std::map<std::string, std::map<std::string, state::MirrorFields>>&
        mnpuMirrors) {
  auto mnpuMap = std::make_shared<MultiMirrorMap>();
  for (const auto& matcherAndMirrors : mnpuMirrors) {
    auto map = MirrorMap::fromThrift(matcherAndMirrors.second);
    mnpuMap->insert(matcherAndMirrors.first, std::move(map));
  }
  return mnpuMap;
}

template class ThriftMapNode<MirrorMap, MirrorMapTraits>;

} // namespace facebook::fboss
