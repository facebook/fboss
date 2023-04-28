// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/agent/state/MirrorMap.h"
#include "fboss/agent/state/Mirror.h"
#include "fboss/agent/state/NodeMap-defs.h"
#include "fboss/agent/state/SwitchState.h"

namespace facebook::fboss {

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

MultiSwitchMirrorMap* MultiSwitchMirrorMap::modify(
    std::shared_ptr<SwitchState>* state) {
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

std::shared_ptr<MultiSwitchMirrorMap> MultiSwitchMirrorMap::fromThrift(
    const std::map<std::string, std::map<std::string, state::MirrorFields>>&
        mnpuMirrors) {
  auto mnpuMap = std::make_shared<MultiSwitchMirrorMap>();
  for (const auto& matcherAndMirrors : mnpuMirrors) {
    auto map = MirrorMap::fromThrift(matcherAndMirrors.second);
    mnpuMap->insert(matcherAndMirrors.first, std::move(map));
  }
  return mnpuMap;
}

template class ThriftMapNode<MirrorMap, MirrorMapTraits>;

} // namespace facebook::fboss
