// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include "fboss/agent/Utils.h"
#include "fboss/agent/gen-cpp2/switch_config_types.h"

namespace facebook::fboss::utility {

inline const int kUdfHashDstQueuePairStartOffsetInBytes(13);
inline const int kUdfHashDstQueuePairFieldSizeInBytes(3);

// chenab has different numbers
inline const int kChenabUdfHashDstQueuePairStartOffsetInBytes(14);
inline const int kChenabUdfHashDstQueuePairFieldSizeInBytes(2);

// Prefix is header name, suffix is field name within header
inline const int kUdfOffsetBthOpcode(0x1);
inline const int kUdfOffsetBthReserved(0x2);
inline const int kUdfOffsetAethSyndrome(0x4);
inline const int kUdfOffsetRethDmaLength(0x8);

cfg::UdfConfig addUdfHashConfig(cfg::AsicType asicType);
cfg::UdfConfig addUdfAclConfig(int udfType = kUdfOffsetBthOpcode);
cfg::UdfConfig addUdfHashAclConfig(cfg::AsicType asicType);

} // namespace facebook::fboss::utility
