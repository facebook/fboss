// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/agent/hw/sai/api/DebugCounterApi.h"

#include <optional>

namespace facebook::fboss::detail {

std::optional<sai_int32_t> trapDrops() {
  return std::nullopt;
}
} // namespace facebook::fboss::detail
