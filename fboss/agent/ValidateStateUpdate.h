// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

namespace facebook::fboss {
class StateDelta;

bool isStateUpdateValidCommon(const StateDelta& delta);
bool isStateUpdateValidMultiSwitch(const StateDelta& delta);
} // namespace facebook::fboss
