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

extern "C" {
#include <bcm/types.h>
}

#include "fboss/agent/FbossError.h"
#include "fboss/agent/hw/bcm/BcmAclStat.h"
#include "fboss/agent/hw/bcm/types.h"
#include "fboss/agent/state/AclEntry.h"
#include "fboss/agent/state/RouteNextHopEntry.h"

namespace facebook::fboss {

class BcmSwitch;
class AclEntry;
class BcmMultiPathNextHop;
enum class MirrorAction;
enum class MirrorDirection;

/**
 * BcmAclEntry is the class to abstract an acl's resources in BcmSwitch
 */
class BcmAclEntry {
 public:
  static constexpr int kLocalIp4DstClassL3Id = 1;
  static constexpr int kLocalIp6DstClassL3Id = 2;

  BcmAclEntry(BcmSwitch* hw, int gid, const std::shared_ptr<AclEntry>& acl);
  ~BcmAclEntry();
  BcmAclEntryHandle getHandle() const {
    return handle_;
  }

  /**
   * Check whether the acl details of handle in h/w matches the s/w acl and
   * ranges
   */
  static bool isStateSame(
      const BcmSwitch* hw,
      int gid,
      BcmAclEntryHandle handle,
      const std::shared_ptr<AclEntry>& acl);
  std::optional<std::string> getIngressAclMirror();
  std::optional<std::string> getEgressAclMirror();
  const std::string& getID() const;

  void applyMirrorAction(
      const std::string& mirrorName,
      MirrorAction action,
      MirrorDirection direction);

  int getGroupId() const {
    return gid_;
  }

  void createNewAclEntry();
  void deleteAclEntry();

 private:
  void createAclQualifiers();
  void createAclActions();
  void createAclStat();
  void applyRedirectToNextHopAction(
      const RouteNextHopSet& nexthops,
      bool isWarmBoot,
      bcm_vrf_t vrf = 0);
  void applyFlowletAction(
      const std::optional<facebook::fboss::MatchAction>& action);

  BcmSwitch* hw_;
  int gid_;
  std::shared_ptr<AclEntry> acl_;
  BcmAclEntryHandle handle_;
  std::shared_ptr<BcmMultiPathNextHop> redirectNexthop_;
};

} // namespace facebook::fboss
