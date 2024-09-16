// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include "fboss/agent/state/SwitchState.h"
#include "fboss/fsdb/client/FsdbSubManager.h"
#include "fboss/fsdb/if/FsdbModel.h"
#include "fboss/thrift_cow/storage/CowStorage.h"

namespace facebook::fboss {

/*
Creating an instantiation of FsdbSubManager that returns fully adapted
SwitchState types. To do this, first we have to adapt all the way to fsdb root
*/

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

extern template class fsdb::FsdbSubManager<
    fsdb::CowStorage<fsdb::FsdbOperStateRoot, FsdbOperStateRoot>,
    true /* IsCow */>;
using FsdbAdaptedSubManager = fsdb::FsdbSubManager<
    fsdb::CowStorage<fsdb::FsdbOperStateRoot, FsdbOperStateRoot>,
    true /* IsCow */>;

} // namespace facebook::fboss
