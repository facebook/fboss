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

#include <memory>

namespace facebook {
namespace fboss {

class TransceiverManager;
class PlatformProductInfo;
class FbossMacsecHandler;

std::unique_ptr<TransceiverManager> createTransceiverManager();

std::shared_ptr<FbossMacsecHandler> createFbossMacsecHandler(
    TransceiverManager* wedgeMgr);

/**
 * This function should return derived TransceiverManager which is still in dev.
 */
std::unique_ptr<TransceiverManager> createFBTransceiverManager(
    std::unique_ptr<PlatformProductInfo> productInfo);

std::unique_ptr<TransceiverManager> createYampTransceiverManager();

std::unique_ptr<TransceiverManager> createElbertTransceiverManager();
} // namespace fboss
} // namespace facebook
