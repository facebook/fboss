// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once

#include "fboss/agent/hw/bcm/types.h"

namespace facebook::fboss {

class BcmSwitch;
class AclEntry;
class Mirror;
class BcmPort;
class MirrorTunnel;

enum class MirrorAction { START = 1, STOP = 2 };
enum class MirrorDirection { INGRESS = 1, EGRESS = 2 };

class BcmMirrorDestination {
 public:
  BcmMirrorDestination(
      int unit,
      BcmPort* egressPort,
      uint8_t dscp,
      bool truncate);
  BcmMirrorDestination(
      int unit,
      BcmPort* egressPort,
      const MirrorTunnel& mirrorTunnel,
      uint8_t dscp,
      bool truncate);
  BcmMirrorDestination(int unit, BcmMirrorHandle handle, int flags)
      : unit_(unit), handle_(handle), flags_(flags) {}
  ~BcmMirrorDestination();
  BcmMirrorHandle getHandle();

  int getFlags() const {
    return flags_;
  }

 private:
  int unit_;
  BcmMirrorHandle handle_;
  int flags_{0};
};

class BcmMirror {
 public:
  BcmMirror(BcmSwitch* hw, const std::shared_ptr<Mirror>& mirror);
  ~BcmMirror();
  void applyPortMirrorAction(
      PortID port,
      MirrorAction action,
      MirrorDirection direction,
      cfg::SampleDestination sampleDest);
  void applyAclMirrorAction(
      BcmAclEntryHandle aclEntryHandle,
      MirrorAction action,
      MirrorDirection direction);
  bool isProgrammed() const {
    /* if this is programmed in hardware */
    return destination_ != nullptr;
  }
  std::optional<BcmMirrorHandle> getHandle() {
    return destination_
        ? std::optional<BcmMirrorHandle>(destination_->getHandle())
        : std::nullopt;
  }

 private:
  BcmSwitch* hw_;
  std::string mirrorName_;

  std::unique_ptr<BcmMirrorDestination> destination_;
  void program(const std::shared_ptr<Mirror>& mirror);
  void applyAclMirrorActions(MirrorAction action);
  void applyPortMirrorActions(MirrorAction action);
  uint8_t mirroredPorts_{0};
};

} // namespace facebook::fboss
