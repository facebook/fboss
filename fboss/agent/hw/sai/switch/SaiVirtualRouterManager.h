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

class SaiVirtualRouter {
 public:
  // Default virtual router
  explicit SaiVirtualRouter(SaiApiTable* apiTable);
  // SaiSwitch managed virtual router
  SaiVirtualRouter(
      SaiApiTable* apiTable,
      const VirtualRouterApiParameters::Attributes& attributes);
  ~SaiVirtualRouter();
  SaiVirtualRouter(const SaiVirtualRouter& other) = delete;
  SaiVirtualRouter(SaiVirtualRouter&& other) = delete;
  SaiVirtualRouter& operator=(const SaiVirtualRouter& other) = delete;
  SaiVirtualRouter& operator=(SaiVirtualRouter&& other) = delete;
  bool operator==(const SaiVirtualRouter& other) const;
  bool operator!=(const SaiVirtualRouter& other) const;

  const VirtualRouterApiParameters::Attributes attributes() const {
    return attributes_;
  }
  sai_object_id_t id() const {
    return id_;
  }

 private:
  SaiApiTable* apiTable_;
  VirtualRouterApiParameters::Attributes attributes_;
  sai_object_id_t id_;
  // In SAI, there is a default VirtualRouter. We should be able to use it,
  // but should not RAII manage its lifetime.
  bool default_{false};
};

class SaiVirtualRouterManager {
 public:
  SaiVirtualRouterManager(SaiApiTable* apiTable, SaiManagerTable* managerTable);
  sai_object_id_t addVirtualRouter(const RouterID& routerId);
  SaiVirtualRouter* getVirtualRouter(const RouterID& routerId);

 private:
  SaiApiTable* apiTable_;
  SaiManagerTable* managerTable_;
  std::unordered_map<RouterID, std::unique_ptr<SaiVirtualRouter>>
      virtualRouters_;
};

} // namespace fboss
} // namespace facebook
