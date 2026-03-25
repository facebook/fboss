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

std::pair<std::unique_ptr<WedgeManager>, std::unique_ptr<PortManager>>
setupForColdboot();
std::pair<std::unique_ptr<WedgeManager>, std::unique_ptr<PortManager>>
setupForWarmboot();

} // namespace facebook::fboss
