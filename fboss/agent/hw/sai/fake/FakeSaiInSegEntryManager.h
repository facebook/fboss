// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once

#include "fboss/agent/hw/sai/fake/FakeManager.h"
#include "fboss/agent/hw/sai/fake/FakeSaiInSegEntry.h"

extern "C" {
#include <saiobject.h>
#include <saiswitch.h>
}

namespace facebook::fboss {

struct FakeInSegEntryAttributes {
  sai_packet_action_t action{SAI_PACKET_ACTION_FORWARD};
  sai_object_id_t nexthop{SAI_NULL_OBJECT_ID};
  sai_uint8_t popcount{0};
};

struct FakeInSegEntryManager
    : public FakeManager<FakeSaiInSegEntry, FakeInSegEntryAttributes> {
  static sai_mpls_api_t* kApi();
};

void populate_mpls_api(sai_mpls_api_t** mpls_api);

} // namespace facebook::fboss
