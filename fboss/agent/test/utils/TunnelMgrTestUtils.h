// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include <string>
#include <vector>

#include "fboss/agent/gen-cpp2/switch_config_types.h"
#include "fboss/agent/types.h"

namespace facebook::fboss {
class TunManager;
class SwitchState;
} // namespace facebook::fboss

namespace facebook::fboss::utility {

void clearKernelEntries(const std::string& intfIp, bool isIPv4 = true);

void clearKernelEntries(
    const std::string& intfIPv4,
    const std::string& intfIPv6);

void clearAllKernelEntries();

bool checkIpRuleEntriesRemoved(const std::string& intfIp, bool isIPv4 = true);

void checkIpKernelEntriesRemoved(const std::string& intfIp, bool isIPv4 = true);

void checkKernelEntriesNotExist(
    const std::string& intfIp,
    bool isIPv4 = true,
    bool checkRouteEntry = true);

void checkKernelEntriesExist(
    const std::string& intfIp,
    bool isIPv4 = true,
    bool checkRouteEntry = true);

void checkKernelEntriesRemoved(
    const std::string& intfIPv4,
    const std::string& intfIPv6);

void checkKernelIpEntriesExist(
    TunManager* tunMgr,
    const std::shared_ptr<SwitchState>& state,
    const InterfaceID& ifid,
    const std::string& intfIP,
    bool isIPv4);

void checkKernelIpEntriesRemoved(
    TunManager* tunMgr,
    const std::shared_ptr<SwitchState>& state,
    const InterfaceID& ifId,
    const std::string& intfIP,
    bool isIPv4);

void checkKernelIpEntriesRemovedStrict(
    const InterfaceID& ifId,
    const std::string& intfIP,
    bool isIPv4);

std::vector<std::string> getInterfaceIpAddress(
    cfg::SwitchConfig& config,
    bool isIPv4);

} // namespace facebook::fboss::utility
