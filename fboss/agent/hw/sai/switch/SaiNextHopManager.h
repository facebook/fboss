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

#include "fboss/agent/hw/sai/api/NextHopApi.h"
#include "fboss/agent/hw/sai/api/SaiApiTable.h"
#include "fboss/agent/types.h"

#include "folly/IPAddress.h"

#include <memory>
#include <unordered_map>

namespace facebook {
namespace fboss {

class SaiManagerTable;
class SaiPlatform;

class SaiNextHop {
 public:
  SaiNextHop(
      SaiApiTable* apiTable,
      const NextHopApiParameters::Attributes& attributes,
      const sai_object_id_t& switchID);
  ~SaiNextHop();
  SaiNextHop(const SaiNextHop& other) = delete;
  SaiNextHop(SaiNextHop&& other) = delete;
  SaiNextHop& operator=(const SaiNextHop& other) = delete;
  SaiNextHop& operator=(SaiNextHop&& other) = delete;
  bool operator==(const SaiNextHop& other) const;
  bool operator!=(const SaiNextHop& other) const;

  void setAttributes(
      const NextHopApiParameters::Attributes& desiredAttributes) {
    throw FbossError("No support for changing next hop attributes");
  }
  const NextHopApiParameters::Attributes attributes() const {
    return attributes_;
  }
  sai_object_id_t id() const {
    return id_;
  }

 private:
  SaiApiTable* apiTable_;
  NextHopApiParameters::Attributes attributes_;
  sai_object_id_t id_;
};

class SaiNextHopManager {
 public:
  SaiNextHopManager(
      SaiApiTable* apiTable,
      SaiManagerTable* managerTable,
      const SaiPlatform* platform);
  std::unique_ptr<SaiNextHop> addNextHop(
      sai_object_id_t routerInterfaceId,
      const folly::IPAddress& ip);

 private:
  SaiApiTable* apiTable_;
  SaiManagerTable* managerTable_;
  const SaiPlatform* platform_;
};

} // namespace fboss
} // namespace facebook
