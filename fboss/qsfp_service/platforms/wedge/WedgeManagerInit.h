/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#pragma once

#include <folly/logging/xlog.h>
#include <memory>
#include <unordered_map>
#include "fboss/agent/platforms/common/PlatformMapping.h"
#include "fboss/agent/types.h"
#include "fboss/lib/if/gen-cpp2/fboss_common_types.h"
#include "fboss/lib/phy/PhyManager.h"
#include "fboss/qsfp_service/SlotThreadHelper.h"

namespace facebook {
namespace fboss {

class WedgeManager;
class PortManager;
class PlatformProductInfo;
class FbossMacsecHandler;

std::unique_ptr<PhyManager> createPhyManager(
    PlatformType mode,
    const PlatformMapping* platformMapping,
    const WedgeManager* wedgeManager);

std::unique_ptr<WedgeManager> createWedgeManager(
    std::unique_ptr<PlatformProductInfo> productInfo,
    const std::shared_ptr<const PlatformMapping> platformMapping,
    const std::shared_ptr<std::unordered_map<TransceiverID, SlotThreadHelper>>
        threads);

std::pair<std::unique_ptr<WedgeManager>, std::unique_ptr<PortManager>>
createQsfpManagers();

std::shared_ptr<FbossMacsecHandler> createFbossMacsecHandler(
    WedgeManager* wedgeMgr);

/**
 * These functions enable us to create different WedgeManagers for open-sourced
 * and closed-souce platforms.
 */
std::unique_ptr<WedgeManager> createFBWedgeManager(
    std::unique_ptr<PlatformProductInfo> productInfo,
    const std::shared_ptr<const PlatformMapping> platformMapping,
    const std::shared_ptr<std::unordered_map<TransceiverID, SlotThreadHelper>>
        threads);
std::unique_ptr<WedgeManager> createYampWedgeManager(
    const std::shared_ptr<const PlatformMapping> platformMapping,
    const std::shared_ptr<std::unordered_map<TransceiverID, SlotThreadHelper>>
        threads);
std::unique_ptr<WedgeManager> createDarwinWedgeManager(
    const std::shared_ptr<const PlatformMapping> platformMapping,
    const std::shared_ptr<std::unordered_map<TransceiverID, SlotThreadHelper>>
        threads);
std::unique_ptr<WedgeManager> createElbertWedgeManager(
    const std::shared_ptr<const PlatformMapping> platformMapping,
    const std::shared_ptr<std::unordered_map<TransceiverID, SlotThreadHelper>>
        threads);
bool isElbert8DD();
std::string getDeviceDatacenter();
std::string getDeviceHostnameScheme();

template <typename BspPlatformMapping, PlatformType platformType>
std::unique_ptr<WedgeManager> createBspWedgeManager(
    const std::shared_ptr<const PlatformMapping> platformMapping,
    const std::shared_ptr<std::unordered_map<TransceiverID, SlotThreadHelper>>
        threads);

} // namespace fboss
} // namespace facebook
