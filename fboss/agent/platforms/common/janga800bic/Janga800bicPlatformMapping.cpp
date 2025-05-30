/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/platforms/common/janga800bic/Janga800bicPlatformMapping.h"
#include "fboss/agent/platforms/common/janga800bic/Janga800bicProdPlatformMapping.h"

namespace facebook::fboss {
Janga800bicPlatformMapping::Janga800bicPlatformMapping()
    : PlatformMapping(
          FLAGS_multi_npu_platform_mapping
              ? kJsonMultiNpuProdPlatformMappingStr
              : kJsonSingleNpuProdPlatformMappingStr) {}

Janga800bicPlatformMapping::Janga800bicPlatformMapping(
    const std::string& platformMappingStr)
    : PlatformMapping(platformMappingStr) {}

Janga800bicPlatformMapping::Janga800bicPlatformMapping(
    bool multiNpuPlatformMapping)
    : PlatformMapping(
          multiNpuPlatformMapping ? kJsonMultiNpuProdPlatformMappingStr
                                  : kJsonSingleNpuProdPlatformMappingStr) {}

} // namespace facebook::fboss
