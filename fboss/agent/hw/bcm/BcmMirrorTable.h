// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once

#include "fboss/agent/hw/bcm/BcmMirror.h"
#include "fboss/agent/state/Mirror.h"

namespace facebook::fboss {

class BcmSwitch;

class BcmMirrorTable {
 public:
  explicit BcmMirrorTable(BcmSwitch* hw) : hw_(hw) {}
  ~BcmMirrorTable() {}
  void processAddedMirror(const std::shared_ptr<Mirror>& mirror);
  void processRemovedMirror(const std::shared_ptr<Mirror>& mirror);
  void processChangedMirror(
      const std::shared_ptr<Mirror>& oldMirror,
      const std::shared_ptr<Mirror>& newMirror);

  BcmMirror* getMirror(const std::string& mirrorName) const;
  BcmMirror* getNodeIf(const std::string& mirrorName) const;

 private:
  using BcmMirrorMap =
      boost::container::flat_map<std::string, std::unique_ptr<BcmMirror>>;

  BcmSwitch* hw_;
  BcmMirrorMap mirrorEntryMap_;
};

} // namespace facebook::fboss
