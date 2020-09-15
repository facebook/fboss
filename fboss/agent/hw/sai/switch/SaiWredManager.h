/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#pragma once

#include "fboss/agent/hw/sai/api/WredApi.h"
#include "fboss/agent/hw/sai/store/SaiObject.h"
#include "fboss/lib/RefMap.h"

#include <memory>

namespace facebook::fboss {

class SaiManagerTable;
class PortQueue;
class SaiPlatform;

using SaiWred = SaiObject<SaiWredTraits>;

class SaiWredManager {
 public:
  SaiWredManager(SaiManagerTable* managerTable, const SaiPlatform* platform)
      : managerTable_(managerTable), platform_(platform) {}

  std::shared_ptr<SaiWred> getOrCreateProfile(const PortQueue& queue);

 private:
  // TODO
  SaiWredTraits::CreateAttributes profileCreateAttrs(
      const PortQueue& queue) const;

  SaiManagerTable* managerTable_;
  const SaiPlatform* platform_;
  UnorderedRefMap<SaiWredTraits::AdapterHostKey, SaiWred> wredProfiles_;
};

} // namespace facebook::fboss
