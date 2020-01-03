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

#include "fboss/agent/Constants.h"
#include "fboss/agent/FbossError.h"
#include "fboss/agent/state/NodeBase-defs.h"

#include <folly/dynamic.h>
#include <folly/json.h>

#include <vector>

#define FBOSS_INSTANTIATE_NODE_MAP(MapType, TraitsType) \
  template class ::facebook::fboss::                    \
      NodeBaseT<MapType, NodeMapFields<TraitsType>>;    \
  template class ::facebook::fboss::NodeMapT<MapType, TraitsType>;

namespace facebook::fboss {

template <typename MapTypeT, typename TraitsT>
const std::shared_ptr<typename TraitsT::Node>&
NodeMapT<MapTypeT, TraitsT>::getNode(KeyType key) const {
  const auto& nodes = getAllNodes();
  auto iter = nodes.find(key);
  if (iter == nodes.end()) {
    throw FbossError("No node ", key);
  }
  return iter->second;
}

template <typename MapTypeT, typename TraitsT>
std::shared_ptr<typename TraitsT::Node> NodeMapT<MapTypeT, TraitsT>::getNodeIf(
    KeyType key) const {
  const auto& nodes = getAllNodes();
  auto iter = nodes.find(key);
  if (iter == nodes.end()) {
    return nullptr;
  }
  return iter->second;
}

template <typename MapTypeT, typename TraitsT>
void NodeMapT<MapTypeT, TraitsT>::addNode(const std::shared_ptr<Node>& node) {
  auto& nodes = writableNodes();
  auto ret = nodes.insert(std::make_pair(TraitsT::getKey(node), node));
  if (!ret.second) {
    throw FbossError("duplicate node ID ", TraitsT::getKey(node));
  }
}

template <typename MapTypeT, typename TraitsT>
void NodeMapT<MapTypeT, TraitsT>::updateNode(
    const std::shared_ptr<Node>& node) {
  auto& nodes = writableNodes();
  auto it = nodes.find(TraitsT::getKey(node));
  if (it == nodes.end()) {
    throw FbossError("node ID ", TraitsT::getKey(node), " does not exist");
  }
  it->second = node;
}

template <typename MapTypeT, typename TraitsT>
void NodeMapT<MapTypeT, TraitsT>::removeNode(
    const std::shared_ptr<Node>& node) {
  auto& nodes = writableNodes();
  auto it = nodes.find(TraitsT::getKey(node));
  if (it == nodes.end()) {
    throw FbossError("node ID ", TraitsT::getKey(node), " does not exist");
  }
  nodes.erase(it);
}

template <typename MapTypeT, typename TraitsT>
std::shared_ptr<typename TraitsT::Node> NodeMapT<MapTypeT, TraitsT>::removeNode(
    const KeyType& key) {
  auto node = removeNodeIf(key);
  if (!node) {
    throw FbossError("node ID ", key, " does not exist");
  }
  return node;
}

template <typename MapTypeT, typename TraitsT>
std::shared_ptr<typename TraitsT::Node>
NodeMapT<MapTypeT, TraitsT>::removeNodeIf(const KeyType& key) {
  auto& nodes = writableNodes();
  auto it = nodes.find(key);
  if (it == nodes.end()) {
    return nullptr;
  }
  std::shared_ptr<Node> node = it->second;
  nodes.erase(it);
  return node;
}

template <typename MapTypeT, typename TraitsT>
folly::dynamic NodeMapT<MapTypeT, TraitsT>::toFollyDynamic() const {
  folly::dynamic nodesJson = folly::dynamic::array;
  for (const auto& node : *this) {
    nodesJson.push_back(node->toFollyDynamic());
  }
  folly::dynamic json = folly::dynamic::object;
  json[kEntries] = std::move(nodesJson);
  json[kExtraFields] = getExtraFields().toFollyDynamic();
  return json;
}

template <typename MapTypeT, typename TraitsT>
std::shared_ptr<MapTypeT> NodeMapT<MapTypeT, TraitsT>::fromFollyDynamic(
    const folly::dynamic& nodesJson) {
  auto nodeMap = std::make_shared<MapTypeT>();
  auto entries = nodesJson[kEntries];
  for (const auto& entry : entries) {
    nodeMap->addNode(Node::fromFollyDynamic(entry));
  }
  nodeMap->writableExtraFields() =
      ExtraFields::fromFollyDynamic(nodesJson[kExtraFields]);
  return nodeMap;
}

} // namespace facebook::fboss
