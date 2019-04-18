// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/agent/state/MirrorMap.h"
#include "fboss/agent/state/Mirror.h"
#include "fboss/agent/state/NodeMap-defs.h"
#include "fboss/agent/state/SwitchState.h"

namespace facebook {
namespace fboss {

MirrorMap::MirrorMap() {}

MirrorMap::~MirrorMap() {}

std::shared_ptr<Mirror> MirrorMap::getMirrorIf(const std::string& name) const {
  for (const auto& mirror : *this) {
    if (mirror->getID() == name) {
      return mirror;
    }
  }
  return nullptr;
}

void MirrorMap::addMirror(const std::shared_ptr<Mirror>& mirror) {
  return addNode(mirror);
}

std::shared_ptr<MirrorMap> MirrorMap::fromFollyDynamic(
    const folly::dynamic& json) {
  /* TODO (pshaikh) can remove this function after push of agent that has
  correct "toFollyDynamic" implementation */
  auto mirrors = std::make_shared<MirrorMap>();
  if (json.isNull()) {
    return mirrors;
  }
  for (const auto& entry :
       json[NodeMapT<MirrorMap, MirrorMapTraits>::kEntries]) {
    mirrors->addNode(Mirror::fromFollyDynamic(entry));
  }
  return mirrors;
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

FBOSS_INSTANTIATE_NODE_MAP(MirrorMap, MirrorMapTraits);

} // namespace fboss
} // namespace facebook
