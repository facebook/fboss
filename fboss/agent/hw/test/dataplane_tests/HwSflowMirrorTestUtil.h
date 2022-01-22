// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once

#include <chrono>
#include <vector>

#include <folly/IPAddress.h>
#include <folly/MacAddress.h>
#include <folly/io/Cursor.h>

namespace facebook::fboss::utility {
enum class SflowShimAsic : uint8_t {
  SFLOW_SHIM_ASIC_UNKNOWN = 0,
  SFLOW_SHIM_ASIC_TH3 = 1,
  SFLOW_SHIM_ASIC_TH4 = 2,
  SFLOW_SHIM_ASIC_GB = 3,
};

struct SflowShimHeaderInfo {
  SflowShimAsic asic{SflowShimAsic::SFLOW_SHIM_ASIC_UNKNOWN};
  uint32_t srcPort;
  uint32_t dstPort;
};

SflowShimHeaderInfo parseSflowShim(folly::io::Cursor& cursor);
SflowShimHeaderInfo parseSflowShim(folly::io::Cursor& cursor);
} // namespace facebook::fboss::utility
