// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/agent/hw/bcm/BcmMirrorTable.h"
#include "fboss/agent/hw/bcm/BcmSwitch.h"

namespace facebook::fboss {

void BcmMirrorTable::processAddedMirror(const std::shared_ptr<Mirror>& mirror) {
  if (mirrorEntryMap_.find(mirror->getID()) != mirrorEntryMap_.end()) {
    throw FbossError("Mirror ", mirror->getID(), " already exists");
  }
  auto mirrorEntry = std::make_unique<BcmMirror>(hw_, mirror);
  const auto& entry =
      mirrorEntryMap_.emplace(mirror->getID(), std::move(mirrorEntry));
  if (!entry.second) {
    throw FbossError("Failed to add an mirror entry ", mirror->getID());
  }
}

void BcmMirrorTable::processRemovedMirror(
    const std::shared_ptr<Mirror>& mirror) {
  if (mirrorEntryMap_.find(mirror->getID()) == mirrorEntryMap_.end()) {
    throw FbossError("Mirror ", mirror->getID(), " does not exist");
  }
  const auto& numErasedMirrors = mirrorEntryMap_.erase(mirror->getID());
  if (!numErasedMirrors) {
    throw FbossError("Failed to remove an mirror ", mirror->getID());
  }
}

void BcmMirrorTable::processChangedMirror(
    const std::shared_ptr<Mirror>& oldMirror,
    const std::shared_ptr<Mirror>& newMirror) {
  processRemovedMirror(oldMirror);
  processAddedMirror(newMirror);
}

BcmMirror* BcmMirrorTable::getMirror(const std::string& mirrorName) const {
  auto mirror = getNodeIf(mirrorName);
  if (!mirror) {
    throw FbossError("Mirror ", mirrorName, " does not exist");
  }
  return mirror;
}

BcmMirror* FOLLY_NULLABLE
BcmMirrorTable::getNodeIf(const std::string& mirrorName) const {
  auto iter = mirrorEntryMap_.find(mirrorName);
  if (iter == mirrorEntryMap_.end()) {
    return nullptr;
  }
  return iter->second.get();
}

} // namespace facebook::fboss
