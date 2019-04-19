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

#include "fboss/agent/hw/sai/api/SaiApiTable.h"
#include "fboss/agent/hw/sai/api/VirtualRouterApi.h"
#include "fboss/agent/types.h"

#include "folly/MacAddress.h"

#include <memory>
#include <unordered_map>

namespace facebook {
namespace fboss {

class SaiManagerTable;

class SaiSwitchInstance {
 public:
  explicit SaiSwitchInstance(SaiApiTable* apiTable);
  SaiSwitchInstance(
      SaiApiTable* apiTable,
      const SwitchApiParameters::Attributes& attributes);
  ~SaiSwitchInstance();
  SaiSwitchInstance(const SaiSwitchInstance& other) = delete;
  SaiSwitchInstance(SaiSwitchInstance&& other) = delete;
  SaiSwitchInstance& operator=(const SaiSwitchInstance& other) = delete;
  SaiSwitchInstance& operator=(SaiSwitchInstance&& other) = delete;
  bool operator==(const SaiSwitchInstance& other) const;
  bool operator!=(const SaiSwitchInstance& other) const;

  const SwitchApiParameters::Attributes attributes() const {
    return attributes_;
  }
  sai_object_id_t id() const {
    return id_;
  }

 private:
  SaiApiTable* apiTable_;
  SwitchApiParameters::Attributes attributes_;
  sai_object_id_t id_;
};

class SaiSwitchManager {
 public:
  SaiSwitchManager(SaiApiTable* apiTable, SaiManagerTable* managerTable);
  const SaiSwitchInstance* getSwitch(const SwitchID& switchId) const;
  SaiSwitchInstance* getSwitch(const SwitchID& switchId);
  sai_object_id_t getSwitchSaiId(const SwitchID& switchId) const;

 private:
  SaiSwitchInstance* getSwitchImpl(const SwitchID& switchId) const;
  SaiApiTable* apiTable_;
  SaiManagerTable* managerTable_;
  std::unordered_map<SwitchID, std::unique_ptr<SaiSwitchInstance>> switches_;
};

} // namespace fboss
} // namespace facebook
