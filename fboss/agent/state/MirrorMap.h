// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once

#include "fboss/agent/gen-cpp2/switch_state_types.h"
#include "fboss/agent/state/Mirror.h"
#include "fboss/agent/state/NodeMap.h"
#include "fboss/agent/state/Thrifty.h"
#include "fboss/agent/types.h"

#include <memory>

namespace facebook::fboss {

using MirrorMapTraits = NodeMapTraits<std::string, Mirror>;

struct MirrorMapThriftTraits
    : public ThriftyNodeMapTraits<std::string, state::MirrorFields> {
  static inline const std::string& getThriftKeyName() {
    static const std::string _key = "name";
    return _key;
  }

  static const KeyType parseKey(const folly::dynamic& key) {
    return key.asString();
  }
};

class MirrorMap : public ThriftyNodeMapT<
                      MirrorMap,
                      MirrorMapTraits,
                      MirrorMapThriftTraits> {
 public:
  MirrorMap();
  ~MirrorMap();

  MirrorMap* modify(std::shared_ptr<SwitchState>* state);
  std::shared_ptr<Mirror> getMirrorIf(const std::string& name) const;

  void addMirror(const std::shared_ptr<Mirror>& mirror);

 private:
  // Inherit the constructors required for clone()
  using ThriftyNodeMapT::ThriftyNodeMapT;
  friend class CloneAllocator;
};

} // namespace facebook::fboss
