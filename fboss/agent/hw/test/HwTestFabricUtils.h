// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once
namespace facebook::fboss {

class HwSwitch;
void setForceTrafficOverFabric(const HwSwitch* hw, bool force);
void checkFabricReachability(const HwSwitch* hw);

} // namespace facebook::fboss
