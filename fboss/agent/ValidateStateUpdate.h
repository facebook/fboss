// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

namespace facebook::fboss {
class StateDelta;

bool isStateUpdateValid(const StateDelta& delta);
} // namespace facebook::fboss
