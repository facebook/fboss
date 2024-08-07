// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include <folly/Benchmark.h>

#include "fboss/lib/CommonFileUtils.h"
#include "fboss/qsfp_service/platforms/wedge/WedgeManager.h"
#include "fboss/qsfp_service/platforms/wedge/WedgeManagerInit.h"

namespace facebook::fboss {

std::vector<TransceiverID> getMatchingTcvrIds(
    std::shared_ptr<WedgeManager> const& wedgeMgr,
    MediaInterfaceCode mediaType);
std::size_t refreshTcvrs(MediaInterfaceCode mediaType);
std::size_t readOneByte(MediaInterfaceCode mediaType);

std::unique_ptr<WedgeManager> setupForColdboot();
std::unique_ptr<WedgeManager> setupForWarmboot();

} // namespace facebook::fboss
