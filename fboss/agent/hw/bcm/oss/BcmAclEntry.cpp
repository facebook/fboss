/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/hw/bcm/BcmAclEntry.h"

namespace facebook {
namespace fboss {
constexpr int BcmAclEntry::kLocalIp4DstClassL3Id;
constexpr int BcmAclEntry::kLocalIp6DstClassL3Id;

BcmAclEntry::BcmAclEntry(
    BcmSwitch* /*hw*/,
    int /*gid*/,
    const std::shared_ptr<AclEntry>& /*acl*/) {}
BcmAclEntry::~BcmAclEntry() {}

std::optional<std::string> BcmAclEntry::getIngressAclMirror() {
  return std::optional<std::string>();
}

std::optional<std::string> BcmAclEntry::getEgressAclMirror() {
  return std::optional<std::string>();
}

void BcmAclEntry::applyMirrorAction(
    const std::string& /*mirrorName*/,
    MirrorAction /*action*/,
    MirrorDirection /*direction*/) {}
} // namespace fboss
} // namespace facebook
