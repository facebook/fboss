// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/agent/state/MirrorMap.h"
#include "fboss/agent/state/Mirror.h"
#include "fboss/agent/state/NodeMap-defs.h"
#include "fboss/agent/state/SwitchState.h"

namespace facebook::fboss {

std::shared_ptr<Mirror> MirrorMap::getMirrorIf(const std::string& name) const {
  auto iter = find(name);
  if (iter == cend()) {
    return nullptr;
  }
  return iter->second;
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

folly::dynamic MirrorMap::toFollyDynamic() const {
  // process old format for dynamic methods, for forward compat
  folly::dynamic nodesJson = folly::dynamic::array;
  for (auto iter : std::as_const(*this)) {
    auto node = iter.second;
    nodesJson.push_back(node->toFollyDynamic());
  }
  folly::dynamic json = folly::dynamic::object;
  json[kEntries] = std::move(nodesJson);
  json[kExtraFields] = folly::dynamic::object;
  return json;
}

std::shared_ptr<MirrorMap> MirrorMap::fromFollyDynamicLegacy(
    const folly::dynamic& dyn) {
  // write old format for dynamic methods, for backward compat
  auto mirrors = std::make_shared<MirrorMap>();
  auto entries = dyn[kEntries];
  for (const auto& entry : entries) {
    mirrors->addNode(Mirror::fromFollyDynamic(entry));
  }
  return mirrors;
}

template class thrift_cow::ThriftMapNode<ThriftMirrorMapNodeTraits>;

} // namespace facebook::fboss
