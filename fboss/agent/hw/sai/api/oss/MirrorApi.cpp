// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/agent/hw/sai/api/MirrorApi.h"

namespace facebook::fboss {

std::optional<sai_attr_id_t>
SaiSflowMirrorTraits::Attributes::AttributeTcBufferLimit::operator()() {
  return std::nullopt;
}

} // namespace facebook::fboss
