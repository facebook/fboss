// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once

#include "fboss/agent/gen-cpp2/switch_state_types.h"
#include "fboss/agent/state/Mirror.h"
#include "fboss/agent/state/NodeMap.h"
#include "fboss/agent/state/Thrifty.h"
#include "fboss/agent/types.h"

#include <memory>

namespace facebook::fboss {

using MirrorMapTypeClass = apache::thrift::type_class::map<
    apache::thrift::type_class::string,
    apache::thrift::type_class::structure>;
using MirrorMapThriftType = std::map<std::string, state::MirrorFields>;

using ThriftMirrorMapNodeTraits =
    thrift_cow::ThriftMapTraits<MirrorMapTypeClass, MirrorMapThriftType>;

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

ADD_THRIFT_MAP_RESOLVER_MAPPING(ThriftMirrorMapNodeTraits, MirrorMap);

class MirrorMap : public thrift_cow::ThriftMapNode<ThriftMirrorMapNodeTraits> {
 public:
  using BaseT = thrift_cow::ThriftMapNode<ThriftMirrorMapNodeTraits>;
  MirrorMap() {}
  virtual ~MirrorMap() {}

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
      map->insert(*mirror.second.name(), std::move(node));
    }
    return map;
  }

  bool operator==(const MirrorMap& that) const {
    if (size() != that.size()) {
      return false;
    }
    auto iter = begin();

    while (iter != end()) {
      auto mirror = iter->second;
      auto other = that.getMirrorIf(mirror->getID());
      if (!other || *mirror != *other) {
        return false;
      }
      iter++;
    }
    return true;
  }

  /*
   * TODO: retire to and from dynamic methods once migrated to thrift cow.
   */
  static std::shared_ptr<MirrorMap> fromFollyDynamicLegacy(
      const folly::dynamic& dyn);

  static std::shared_ptr<MirrorMap> fromFollyDynamic(
      const folly::dynamic& dyn) {
    return fromFollyDynamicLegacy(dyn);
  }

  folly::dynamic toFollyDynamicLegacy() const {
    return this->toFollyDynamic();
  }

  folly::dynamic toFollyDynamic() const override;

  void addNode(const std::shared_ptr<Mirror>& node) {
    auto key = node->getID();
    auto value = node;
    auto ret = insert(key, std::move(value));
    if (!ret.second) {
      throw FbossError("mirror ", key, " already exists");
    }
  }
  void updateNode(const std::shared_ptr<Mirror>& node) {
    removeNode(node);
    addNode(node);
  }
  void removeNode(const std::shared_ptr<Mirror>& node) {
    removeNode(node->getID());
  }
  std::shared_ptr<Mirror> removeNode(const std::string& key) {
    if (auto node = removeNodeIf(key)) {
      return node;
    }
    throw FbossError("mirror ", key, " does not exist");
  }
  std::shared_ptr<Mirror> removeNodeIf(const std::string& key) {
    auto iter = find(key);
    if (iter == end()) {
      return nullptr;
    }
    auto mirror = iter->second;
    erase(iter);
    return mirror;
  }

  std::shared_ptr<Mirror> getNode(const std::string& key) {
    auto mirror = getMirrorIf(key);
    if (!mirror) {
      throw FbossError("mirror ", key, " does not exist");
    }
    return mirror;
  }

 private:
  // Inherit the constructors required for clone()
  using BaseT::BaseT;
  friend class CloneAllocator;
};

} // namespace facebook::fboss
