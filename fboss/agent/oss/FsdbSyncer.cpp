/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/FsdbSyncer.h"

namespace facebook::fboss {

std::vector<std::string> FsdbSyncer::getAgentStatePath() const {
  return {};
}

std::vector<std::string> FsdbSyncer::getAgentStatsPath() const {
  return {};
}

std::vector<std::string> FsdbSyncer::getSwitchStatePath() const {
  return {};
}

std::vector<std::string> FsdbSyncer::getSwConfigPath() const {
  return {};
}

std::vector<std::string> FsdbSyncer::getPortMapPath() const {
  return {};
}

std::vector<std::string> FsdbSyncer::getVlanMapPath() const {
  return {};
}

std::vector<std::string> FsdbSyncer::getArpTablePath(
    int16_t /* vlanId */) const {
  return {};
}

std::vector<std::string> FsdbSyncer::getNdpTablePath(
    int16_t /* vlanId */) const {
  return {};
}

std::vector<std::string> FsdbSyncer::getMacTablePath(
    int16_t /* vlanId */) const {
  return {};
}

} // namespace facebook::fboss
