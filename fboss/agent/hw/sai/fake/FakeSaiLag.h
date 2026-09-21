// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once

#include "fboss/agent/hw/sai/fake/FakeManager.h"

#include <folly/IPAddressV4.h>
#include <folly/IPAddressV6.h>
#include <folly/MacAddress.h>

extern "C" {
#include <sai.h>
}

namespace facebook::fboss {

struct FakeLagMember {
  FakeLagMember(
      sai_object_id_t lagId,
      sai_object_id_t portId,
      bool egressDisable)
      : lagId_(lagId), portId_(portId), egressDisable_(egressDisable) {}
  sai_object_id_t lagId_;
  sai_object_id_t portId_;
  sai_object_id_t id;
  bool egressDisable_;
};

struct FakeLag {
  sai_object_id_t id;
  std::array<char, 32> label;
  uint16_t vlan;
  FakeManager<sai_object_id_t, FakeLagMember>& fm() {
    return fm_;
  }
  const FakeManager<sai_object_id_t, FakeLagMember>& fm() const {
    return fm_;
  }
  FakeManager<sai_object_id_t, FakeLagMember> fm_;
};

using FakeLagManager = FakeManagerWithMembers<FakeLag, FakeLagMember>;

void populate_lag_api(sai_lag_api_t** lag_api);

} // namespace facebook::fboss
