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

#include "fboss/agent/state/SwitchState.h"
#include "fboss/fsdb/if/FsdbModel.h"

// Forward declarations and definitions
namespace facebook::fboss {

using k_fsdb_model = fsdb::fsdb_model_tags::strings;

class AgentData;
RESOLVE_STRUCT_MEMBER(AgentData, k_fsdb_model::switchState, SwitchState);
class AgentData : public ThriftStructNode<AgentData, fsdb::AgentData> {
 public:
  using BaseT = ThriftStructNode<AgentData, fsdb::AgentData>;
  using BaseT::modify;
  // Inherit the constructors required for clone()
  using BaseT::BaseT;
  friend class CloneAllocator;
};

class FsdbOperStateRoot;
RESOLVE_STRUCT_MEMBER(FsdbOperStateRoot, k_fsdb_model::agent, AgentData);
class FsdbOperStateRoot
    : public ThriftStructNode<FsdbOperStateRoot, fsdb::FsdbOperStateRoot> {
 public:
  using BaseT = ThriftStructNode<FsdbOperStateRoot, fsdb::FsdbOperStateRoot>;
  using BaseT::modify;
  // Inherit the constructors required for clone()
  using BaseT::BaseT;
  friend class CloneAllocator;
};

} // namespace facebook::fboss
