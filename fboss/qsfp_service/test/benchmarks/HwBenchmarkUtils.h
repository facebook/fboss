// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include "fboss/lib/CommonFileUtils.h"
#include "fboss/qsfp_service/platforms/wedge/WedgeManager.h"
#include "fboss/qsfp_service/platforms/wedge/WedgeManagerInit.h"

namespace facebook::fboss {

std::unique_ptr<WedgeManager> setupForColdboot();
std::unique_ptr<WedgeManager> setupForWarmboot();

} // namespace facebook::fboss
