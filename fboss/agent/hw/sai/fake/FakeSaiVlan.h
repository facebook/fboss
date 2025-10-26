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

#include "fboss/agent/hw/sai/fake/FakeManager.h"

extern "C" {
#include <sai.h>
}
namespace facebook::fboss {

class FakeVlanMember {
 public:
  explicit FakeVlanMember(sai_object_id_t vlanId) : vlanId(vlanId) {}
  sai_object_id_t vlanId;
  sai_object_id_t bridgePortId;
  sai_object_id_t id;
};

class FakeVlan {
 public:
  explicit FakeVlan(uint16_t vlanId) : vlanId(vlanId) {}
  uint16_t vlanId;
  sai_object_id_t id;
  FakeManager<sai_object_id_t, FakeVlanMember>& fm() {
    return fm_;
  }
  const FakeManager<sai_object_id_t, FakeVlanMember>& fm() const {
    return fm_;
  }

 private:
  FakeManager<sai_object_id_t, FakeVlanMember> fm_;
};

using FakeVlanManager = FakeManagerWithMembers<FakeVlan, FakeVlanMember>;

void populate_vlan_api(sai_vlan_api_t** vlan_api);

} // namespace facebook::fboss
