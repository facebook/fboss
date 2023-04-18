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

#include "fboss/agent/hw/sai/api/UdfApi.h"
#include "fboss/agent/hw/sai/store/SaiObject.h"
#include "fboss/agent/state/UdfGroup.h"
#include "fboss/agent/state/UdfPacketMatcher.h"

#include <memory>

namespace facebook::fboss {

class SaiManagerTable;
class SaiPlatform;
class SaiStore;

using SaiUdf = SaiObject<SaiUdfTraits>;
using SaiUdfGroup = SaiObject<SaiUdfGroupTraits>;
using SaiUdfMatch = SaiObject<SaiUdfMatchTraits>;

struct SaiUdfHandle;

struct SaiUdfGroupHandle {
  std::shared_ptr<SaiUdfGroup> udfGroup;
  std::unordered_map<std::string, std::unique_ptr<SaiUdfHandle>> udfs;
};

struct SaiUdfMatchHandle {
  std::shared_ptr<SaiUdfMatch> udfMatch;
  std::vector<SaiUdfHandle*> udfs;
};

struct SaiUdfHandle {
  std::shared_ptr<SaiUdf> udf;
  SaiUdfMatchHandle* udfMatch;
  SaiUdfGroupHandle* udfGroup;
};

class SaiUdfManager {
 public:
  SaiUdfManager(
      SaiStore* saiStore,
      SaiManagerTable* managerTable,
      const SaiPlatform* platform)
      : saiStore_(saiStore), managerTable_(managerTable), platform_(platform) {}

  static auto constexpr kMaskDontCare = 0;
  static auto constexpr kL4PortMask = 0xFFFF;

  SaiUdfTraits::CreateAttributes udfAttr(
      const std::shared_ptr<UdfGroup> swUdfGroup,
      const sai_object_id_t saiUdfGroupId,
      const sai_object_id_t saiUdfMatchId) const;

  SaiUdfGroupTraits::CreateAttributes udfGroupAttr(
      const std::shared_ptr<UdfGroup> swUdfGroup) const;

  SaiUdfMatchTraits::CreateAttributes udfMatchAttr(
      const std::shared_ptr<UdfPacketMatcher> swUdfMatch) const;

  UdfGroupSaiId addUdfGroup(const std::shared_ptr<UdfGroup>& swUdfGroup);

  UdfMatchSaiId addUdfMatch(
      const std::shared_ptr<UdfPacketMatcher>& swUdfMatch);

  void removeUdfMatch(const std::shared_ptr<UdfPacketMatcher>& swUdfMatch);

 private:
  uint8_t cfgL4MatchTypeToSai(cfg::UdfMatchL4Type cfgType) const;
  uint16_t cfgL3MatchTypeToSai(cfg::UdfMatchL3Type cfgType) const;
  sai_udf_base_t cfgBaseToSai(cfg::UdfBaseHeaderType cfgType) const;

  std::unordered_map<std::string, std::unique_ptr<SaiUdfMatchHandle>>
      udfMatchHandles_;
  std::unordered_map<std::string, std::unique_ptr<SaiUdfGroupHandle>>
      udfGroupHandles_;
  SaiStore* saiStore_;
  SaiManagerTable* managerTable_;
  const SaiPlatform* platform_;
};

} // namespace facebook::fboss
