// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include "fboss/agent/state/PortPgConfig.h"

namespace facebook::fboss::utility {

bool isLosslessPg(const PortPgConfig& pgConfig);
bool isLosslessPg(const state::PortPgFields& pgConfig);

} // namespace facebook::fboss::utility
