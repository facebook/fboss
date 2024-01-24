// (c) Facebook, Inc. and its affiliates. Confidential and proprietary.

#pragma once

#include "fboss/agent/hw/switch_asics/HwAsic.h"

#include "fboss/agent/packet/EthFrame.h"

namespace facebook::fboss {
std::vector<std::string> toStringVec(const std::vector<std::string_view>& in);
std::unique_ptr<std::vector<utility::EthFrame>> getFullHashedPackets(
    cfg::AsicType asic,
    bool isSai);

std::vector<std::string>& getFullHashedPacketsForTrident2();
std::vector<std::string>& getFullHashedPacketsForTomahawk();
std::vector<std::string>& getFullHashedPacketsForTomahawk3();
std::vector<std::string>& getFullHashedPacketsForTomahawk4();
std::vector<std::string>& getFullHashedPacketsForTrident2Sai();
std::vector<std::string>& getFullHashedPacketsForTomahawkSai();

} // namespace facebook::fboss
