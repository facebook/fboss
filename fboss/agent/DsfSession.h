// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once

#include "fboss/agent/if/gen-cpp2/ctrl_types.h"
#include "fboss/fsdb/if/gen-cpp2/fsdb_common_types.h"

#include <cstdint>
#include <optional>
#include <string>

namespace facebook::fboss {

class DsfSession {
 public:
  explicit DsfSession(std::string nodeName);
  ~DsfSession() = default;

  void localSubStateChanged(fsdb::FsdbSubscriptionState newState);
  void remoteSubStateChanged(fsdb::FsdbSubscriptionState newState);

  DsfSessionThrift toThrift() const;

 private:
  static DsfSessionState calculateSessionState(
      fsdb::FsdbSubscriptionState localState,
      fsdb::FsdbSubscriptionState remoteState);

  void changeSessionState(DsfSessionState newState);

  std::string nodeName_;
  fsdb::FsdbSubscriptionState localSubState_{
      fsdb::FsdbSubscriptionState::DISCONNECTED};
  fsdb::FsdbSubscriptionState remoteSubState_{
      fsdb::FsdbSubscriptionState::DISCONNECTED};
  DsfSessionState state_{DsfSessionState::IDLE};
  std::optional<uint64_t> lastEstablishedAt_;
  std::optional<uint64_t> lastDisconnectedAt_;
};

} // namespace facebook::fboss
