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

class MirrorMap : public NodeMapT<MirrorMap, MirrorMapTraits> {
 public:
  using BaseT = NodeMapT<MirrorMap, MirrorMapTraits>;
  MirrorMap();
  ~MirrorMap();

  MirrorMap* modify(std::shared_ptr<SwitchState>* state);
  std::shared_ptr<Mirror> getMirrorIf(const std::string& name) const;

  void addMirror(const std::shared_ptr<Mirror>& mirror);

  static std::shared_ptr<MirrorMap> fromThrift(
      const std::map<std::string, state::MirrorFields>& mirrors) {
    auto map = std::make_shared<MirrorMap>();
    for (auto mirror : mirrors) {
      auto node = std::make_shared<Mirror>();
      node->fromThrift(mirror.second);
      // TODO(pshaikh): make this private
      node->markResolved();
      map->addNode(node);
    }
    return map;
  }

  std::map<std::string, state::MirrorFields> toThrift() const {
    std::map<std::string, state::MirrorFields> map{};
    for (auto entry : getAllNodes()) {
      map.emplace(entry.first, entry.second->toThrift());
    }
    return map;
  }

  bool operator==(const MirrorMap& that) const {
    if (size() != that.size()) {
      return false;
    }
    auto iter = begin();

    while (iter != end()) {
      auto mirror = *iter;
      auto other = that.getMirrorIf(mirror->getID());
      if (!other || *mirror != *other) {
        return false;
      }
      iter++;
    }
    return true;
  }

  static std::shared_ptr<MirrorMap> fromFollyDynamicLegacy(
      const folly::dynamic& dyn) {
    return BaseT::fromFollyDynamic(dyn);
  }

  folly::dynamic toFollyDynamicLegacy() const {
    return this->toFollyDynamic();
  }

 private:
  // Inherit the constructors required for clone()
  using BaseT::BaseT;
  friend class CloneAllocator;
};

} // namespace facebook::fboss
