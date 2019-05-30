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
#include "fboss/agent/hw/sai/api/SaiApiTable.h"
#include "fboss/agent/state/StateDelta.h"
#include "fboss/agent/types.h"

#include <memory>
#include <unordered_map>

namespace facebook {
namespace fboss {

class SaiManagerTable;
class SaiPlatform;

class SaiFdbEntry {
 public:
  SaiFdbEntry(
      SaiApiTable* apiTable,
      const FdbApiParameters::EntryType& entry,
      const FdbApiParameters::Attributes& attributes);
  ~SaiFdbEntry();
  SaiFdbEntry(const SaiFdbEntry& other) = delete;
  SaiFdbEntry(SaiFdbEntry&& other) = delete;
  SaiFdbEntry& operator=(const SaiFdbEntry& other) = delete;
  SaiFdbEntry& operator=(SaiFdbEntry&& other) = delete;
  bool operator==(const SaiFdbEntry& other) const;
  bool operator!=(const SaiFdbEntry& other) const;

  const FdbApiParameters::Attributes attributes() const {
    return attributes_;
  }

 private:
  SaiApiTable* apiTable_;
  FdbApiParameters::EntryType entry_;
  FdbApiParameters::Attributes attributes_;
};

class SaiFdbManager {
 public:
  SaiFdbManager(
      SaiApiTable* apiTable,
      SaiManagerTable* managerTable,
      const SaiPlatform* platform);
  std::unique_ptr<SaiFdbEntry> addFdbEntry(
      const InterfaceID& intfId,
      const folly::MacAddress& mac,
      const PortDescriptor& portDesc);

 private:
  SaiFdbEntry* getFdbEntryImpl(const FdbApiParameters::EntryType& entry) const;
  SaiApiTable* apiTable_;
  SaiManagerTable* managerTable_;
  const SaiPlatform* platform_;
};

} // namespace fboss
} // namespace facebook
