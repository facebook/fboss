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

#include "fboss/agent/gen-cpp2/switch_config_types.h"
#include "fboss/agent/hw/sai/api/HashApi.h"
#include "fboss/agent/hw/sai/store/SaiObject.h"

#include "fboss/lib/RefMap.h"

namespace facebook::fboss {

class SaiManagerTable;
class StateDelta;

using SaiHash = SaiObject<SaiHashTraits>;

class SaiHashManager {
 public:
  explicit SaiHashManager(SaiManagerTable* managerTable)
      : managerTable_(managerTable) {}

  std::shared_ptr<SaiHash> getOrCreate(const cfg::Fields& hashFields);

 private:
  SaiHashTraits::AdapterHostKey toAdapterHostKey(
      const cfg::Fields& hashFields) const;
  FlatRefMap<SaiHashTraits::AdapterHostKey, SaiHash> handles_;
  SaiManagerTable* managerTable_;
};

} // namespace facebook::fboss
