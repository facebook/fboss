// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once

#include "fboss/agent/state/NodeMap.h"
#include "fboss/agent/types.h"

#include <memory>

namespace facebook {
namespace fboss {

class Mirror;

using MirrorMapTraits = NodeMapTraits<std::string, Mirror>;

class MirrorMap : public NodeMapT<MirrorMap, MirrorMapTraits> {
 public:
  MirrorMap();
  ~MirrorMap();

  std::shared_ptr<Mirror> getMirrorIf(const std::string& name) const;

  void addMirror(const std::shared_ptr<Mirror>& mirror);

  static std::shared_ptr<MirrorMap> fromFollyDynamic(
      const folly::dynamic& json);

 private:
  // Inherit the constructors required for clone()
  using NodeMapT::NodeMapT;
  friend class CloneAllocator;
};

} // namespace fboss
} // namespace facebook
