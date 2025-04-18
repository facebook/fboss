// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include "fboss/agent/Utils.h"
#include "fboss/agent/gen-cpp2/switch_config_types.h"

namespace facebook::fboss::utility {

#if defined(CHENAB_SAI_SDK)
// Use least-significant 2 bytes of Dst Queue Pair on Chenab platform.
inline const int kUdfHashDstQueuePairStartOffsetInBytes(14);
inline const int kUdfHashDstQueuePairFieldSizeInBytes(2);
#else
inline const int kUdfHashDstQueuePairStartOffsetInBytes(13);
inline const int kUdfHashDstQueuePairFieldSizeInBytes(3);
#endif

// Prefix is header name, suffix is field name within header
inline const int kUdfOffsetBthOpcode(0x1);
inline const int kUdfOffsetBthReserved(0x2);
inline const int kUdfOffsetAethSyndrome(0x4);
inline const int kUdfOffsetRethDmaLength(0x8);

cfg::UdfConfig addUdfHashConfig();
cfg::UdfConfig addUdfAclConfig(int udfType = kUdfOffsetBthOpcode);
cfg::UdfConfig addUdfHashAclConfig();

} // namespace facebook::fboss::utility
