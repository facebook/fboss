/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/qsfp_service/StatsPublisher.h"
namespace facebook {
namespace fboss {
void StatsPublisher::init() {}

void StatsPublisher::publishStats(folly::EventBase*, int32_t) {}
// static
void StatsPublisher::bumpPciLockHeld() {}
// static
void StatsPublisher::bumpReadFailure() {}
// static
void StatsPublisher::bumpWriteFailure() {}
// static
void StatsPublisher::missingPorts(TransceiverID /* unused */) {}
// static
void StatsPublisher::bumpModuleErrors() {}
// static
void StatsPublisher::bumpAOIOverride() {}
// static
void StatsPublisher::bumpHighTemp() {}
// static
void StatsPublisher::bumpHighVcc() {}
// static
void StatsPublisher::bumpHighTempPort(std::string& /* portName */) {}
// static
void StatsPublisher::bumpHighVccPort(std::string& /* portName */) {}
// static
void StatsPublisher::initPerPortFb303Stats(
    std::set<std::string>& /* portNames */) {}
} // namespace fboss
} // namespace facebook
