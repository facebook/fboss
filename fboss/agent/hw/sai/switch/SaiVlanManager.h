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
#include "fboss/agent/hw/sai/api/VlanApi.h"
#include "fboss/agent/state/Vlan.h"
#include "fboss/agent/types.h"

#include <memory>
#include <unordered_map>

namespace facebook {
namespace fboss {

class SaiManagerTable;

class SaiVlanMember {
 public:
  SaiVlanMember(
      SaiApiTable* apiTable,
      const VlanApiParameters::MemberAttributes& attributes);
  ~SaiVlanMember();
  SaiVlanMember(const SaiVlanMember& other) = delete;
  SaiVlanMember(SaiVlanMember&& other) = delete;
  SaiVlanMember& operator=(const SaiVlanMember& other) = delete;
  SaiVlanMember& operator=(SaiVlanMember&& other) = delete;
  bool operator==(const SaiVlanMember& other) const;
  bool operator!=(const SaiVlanMember& other) const;

  void setAttributes(
      const VlanApiParameters::MemberAttributes& desiredAttributes);
  const VlanApiParameters::MemberAttributes attributes() const {
    return attributes_;
  }
  sai_object_id_t id() const {
    return id_;
  }

 private:
  SaiApiTable* apiTable_;
  VlanApiParameters::MemberAttributes attributes_;
  sai_object_id_t id_;
};

class SaiVlan {
 public:
  SaiVlan(
      SaiApiTable* apiTable,
      SaiManagerTable* managerTable,
      const VlanApiParameters::Attributes& attributes);
  ~SaiVlan();
  SaiVlan(const SaiVlan& other) = delete;
  SaiVlan(SaiVlan&& other) = delete;
  SaiVlan& operator=(const SaiVlan& other) = delete;
  SaiVlan& operator=(SaiVlan&& other) = delete;
  bool operator==(const SaiVlan& other) const;
  bool operator!=(const SaiVlan& other) const;

  void addMember(PortID swPortId);
  void removeMember(PortID swPortId);
  std::vector<sai_object_id_t> getMemberBridgePortIds() const;
  const VlanApiParameters::Attributes attributes() const {
    return attributes_;
  }
  sai_object_id_t id() const {
    return id_;
  }

 private:
  SaiApiTable* apiTable_;
  SaiManagerTable* managerTable_;
  VlanApiParameters::Attributes attributes_;
  std::unordered_map<sai_object_id_t, std::unique_ptr<SaiVlanMember>> members_;
  std::unordered_map<sai_object_id_t, sai_object_id_t> memberIdMap_;
  sai_object_id_t id_;
};

class SaiVlanManager {
 public:
  SaiVlanManager(SaiApiTable* apiTable, SaiManagerTable* managerTable);
  sai_object_id_t addVlan(const std::shared_ptr<Vlan>& swVlan);
  void removeVlan(const VlanID& id);
  void changeVlan(
      const std::shared_ptr<Vlan>& swVlanOld,
      const std::shared_ptr<Vlan>& swVlanNew);

  SaiVlan* getVlan(VlanID swId);
  const SaiVlan* getVlan(VlanID swId) const;

 private:
  SaiVlan* getVlanImpl(VlanID swId) const;
  SaiApiTable* apiTable_;
  SaiManagerTable* managerTable_;
  std::unordered_map<VlanID, std::unique_ptr<SaiVlan>> vlans_;
};

} // namespace fboss
} // namespace facebook
