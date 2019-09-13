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

#include "fboss/agent/hw/sai/api/FdbApi.h"
#include "fboss/agent/hw/sai/store/SaiObject.h"
#include "fboss/agent/state/StateDelta.h"
#include "fboss/agent/types.h"

#include <memory>
#include <unordered_map>

namespace facebook {
namespace fboss {

class SaiManagerTable;
class SaiPlatform;

using SaiFdbEntry = SaiObject<SaiFdbTraits>;

class SaiFdbManager {
 public:
  SaiFdbManager(SaiManagerTable* managerTable, const SaiPlatform* platform);
  std::shared_ptr<SaiFdbEntry> addFdbEntry(
      const InterfaceID& intfId,
      const folly::MacAddress& mac,
      const PortDescriptor& portDesc);

 private:
  SaiManagerTable* managerTable_;
  const SaiPlatform* platform_;
};

} // namespace fboss
} // namespace facebook
