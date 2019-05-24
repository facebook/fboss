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

#include "fboss/agent/hw/sai/api/BridgeApi.h"
#include "fboss/agent/hw/sai/api/SaiApiTable.h"
#include "fboss/agent/types.h"

#include <memory>
#include <unordered_map>

namespace facebook {
namespace fboss {

class SaiManagerTable;

class SaiBridge {
 public:
  SaiBridge(
      SaiApiTable* apiTable,
      const BridgeApiParameters::Attributes& attributes,
      const sai_object_id_t& switchId);
  ~SaiBridge();
  SaiBridge(const SaiBridge& other) = delete;
  SaiBridge(SaiBridge&& other) = delete;
  SaiBridge& operator=(const SaiBridge& other) = delete;
  SaiBridge& operator=(SaiBridge&& other) = delete;
  bool operator==(const SaiBridge& other) const;
  bool operator!=(const SaiBridge& other) const;

  const BridgeApiParameters::Attributes attributes() const {
    return attributes_;
  }
  sai_object_id_t id() const {
    return id_;
  }

 private:
  SaiApiTable* apiTable_;
  BridgeApiParameters::Attributes attributes_;
  sai_object_id_t id_;
};

class SaiBridgePort {
 public:
  SaiBridgePort(
      SaiApiTable* apiTable,
      const BridgeApiParameters::MemberAttributes& attributes,
      const sai_object_id_t& switchId);
  ~SaiBridgePort();
  SaiBridgePort(const SaiBridgePort& other) = delete;
  SaiBridgePort(SaiBridgePort&& other) = delete;
  SaiBridgePort& operator=(const SaiBridgePort& other) = delete;
  SaiBridgePort& operator=(SaiBridgePort&& other) = delete;
  bool operator==(const SaiBridgePort& other) const;
  bool operator!=(const SaiBridgePort& other) const;

  const BridgeApiParameters::MemberAttributes attributes() const {
    return attributes_;
  }
  sai_object_id_t id() const {
    return id_;
  }

 private:
  SaiApiTable* apiTable_;
  BridgeApiParameters::MemberAttributes attributes_;
  sai_object_id_t id_;
};

class SaiBridgeManager {
 public:
  SaiBridgeManager(SaiApiTable* apiTable, SaiManagerTable* managerTable);
  std::unique_ptr<SaiBridgePort> addBridgePort(sai_object_id_t portId);

 private:
  SaiApiTable* apiTable_;
  SaiManagerTable* managerTable_;
  std::unordered_map<BridgeID, std::unique_ptr<SaiBridge>> bridges_;
};

} // namespace fboss
} // namespace facebook
