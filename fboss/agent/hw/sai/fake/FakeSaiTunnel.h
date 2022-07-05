// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once

#include "fboss/agent/hw/sai/fake/FakeManager.h"

extern "C" {
#include <sai.h>
}

namespace facebook::fboss {

struct FakeTunnel {
  FakeTunnel() {}
};
void populate_tunnel_api(sai_tunnel_api_t** tunnel_api);

} // namespace facebook::fboss
