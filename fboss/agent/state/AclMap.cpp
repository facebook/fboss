/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/state/AclMap.h"

#include "fboss/agent/state/NodeMap-defs.h"

#include "fboss/agent/HwSwitchMatcher.h"
#include "fboss/agent/state/SwitchState.h"

namespace facebook::fboss {

AclMap::AclMap() {}

AclMap::~AclMap() {}

std::set<cfg::AclTableQualifier> PrioAclMap::requiredQualifiers() const {
  std::set<cfg::AclTableQualifier> qualifiers{};
  for (const auto& entry : *this) {
    for (auto qualifier : entry->getRequiredAclTableQualifiers()) {
      qualifiers.insert(qualifier);
    }
  }
  return qualifiers;
}

MultiSwitchAclMap* MultiSwitchAclMap::modify(
    std::shared_ptr<SwitchState>* state) {
  if (!isPublished()) {
    CHECK(!(*state)->isPublished());
    return this;
  }

  SwitchState::modify(state);
  auto newMultiSwitchMap = clone();
  for (auto mnitr = cbegin(); mnitr != cend(); ++mnitr) {
    (*newMultiSwitchMap)[mnitr->first] = mnitr->second->clone();
  }
  auto* ptr = newMultiSwitchMap.get();
  (*state)->resetAcls(std::move(newMultiSwitchMap));
  return ptr;
}

template class ThriftMapNode<AclMap, AclMapTraits>;
FBOSS_INSTANTIATE_NODE_MAP(PrioAclMap, PrioAclMapTraits);

template class NodeMapDelta<
    PrioAclMap,
    DeltaValue<PrioAclMap::Node>,
    MapUniquePointerTraits<PrioAclMap>>;

} // namespace facebook::fboss
