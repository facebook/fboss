// (c) Facebook, Inc. and its affiliates. Confidential and proprietary.

#pragma once

#include <fboss/mka_service/if/gen-cpp2/mka_types.h>
#include "fboss/agent/hw/sai/api/Types.h"
#include "fboss/agent/types.h"
#include "folly/MacAddress.h"

extern "C" {
#include <sai.h>
}

namespace facebook::fboss::utility {

mka::MKASci makeSci(const std::string& mac, PortID portId);
mka::MKASak makeSak(
    const mka::MKASci& sci,
    std::string portName,
    const std::string& keyHex,
    const std::string& keyIdHex,
    int assocNum);
std::string getAclName(
    facebook::fboss::PortID port,
    sai_macsec_direction_t direction);
MacsecSecureChannelId packSecureChannelId(const mka::MKASci& sci);
} // namespace facebook::fboss::utility
