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

#include <memory>
#include <unordered_map>

#include <boost/container/flat_map.hpp>

namespace facebook::fboss {
/*
 * RefMap is a helper class template for managing SAI objects with shared
 * ownership _within_ SAI. If the creation or deletion of an object is
 * triggered directly by a SwitchState update (port, route, vlan, etc...)
 * then it is not meant to be stored in a RefMap. However, if the object is
 * internal to SAI but is not exclusively owned by another object, RefMap
 * is appropriate.
 *
 * A good example is a next hop. We create a next hop while creating a route
 * which uses the next hop, but many routes would want to reuse the same next
 * hop. When no more routes refer to a next hop, it should be deleted.
 */

namespace {
template <typename K, typename V>
using RefMapUMap = std::unordered_map<K, V>;

template <typename K, typename V>
using RefMapFlatMap = boost::container::flat_map<K, V>;
}; // namespace

template <template <class, class> class M, typename K, typename V>
class RefMap {
 public:
  // using MapType = std::unordered_map<K, std::weak_ptr<V>>;
  // using MapType = boost::container::flat_map<K, std::weak_ptr<V>>;
  using MapType = M<K, std::weak_ptr<V>>;
  using KeyType = K;
  using ValueType = std::weak_ptr<V>;
  RefMap() {}
  RefMap(const RefMap& other) = delete;
  RefMap& operator=(const RefMap& other) = delete;

  template <typename... Args>
  std::pair<std::shared_ptr<V>, bool> refOrEmplace(const K& k, Args&&... args) {
    std::shared_ptr<V> vsp;
    bool ins;
    auto itr = map_.find(k);
    if (itr != map_.end()) {
      vsp = itr->second.lock();
      ins = false;
    } else {
      auto del = [& m = map_, k](V* v) {
        m.erase(k);
        std::default_delete<V>()(v);
      };
      vsp = std::shared_ptr<V>(new V(std::forward<Args>(args)...), del);
      map_[k] = vsp;
      ins = true;
    }
    return {vsp, ins};
  }

  std::size_t size() const {
    return map_.size();
  }

  std::shared_ptr<V> ref(const K& k) const {
    auto itr = map_.find(k);
    if (itr == map_.end()) {
      return std::shared_ptr<V>{};
    }
    return itr->second.lock();
  }

  V* get(const K& k) {
    return getImpl(k);
  }

  const V* get(const K& k) const {
    return getImpl(k);
  }

  V* getMutable(const K& k) const {
    return getImpl(k);
  }

  typename MapType::iterator begin() {
    return map_.begin();
  }

  typename MapType::iterator end() {
    return map_.end();
  }

  typename MapType::const_iterator begin() const {
    return map_.cbegin();
  }

  typename MapType::const_iterator end() const {
    return map_.cend();
  }

  typename MapType::const_iterator cbegin() const {
    return map_.cbegin();
  }

  typename MapType::const_iterator cend() const {
    return map_.cend();
  }

  long referenceCount(const K& k) const {
    auto iter = map_.find(k);
    if (iter == map_.cend() || iter->second.expired()) {
      return 0;
    }
    return iter->second.use_count();
  }

  void clear() {
    return map_.clear();
  }

 private:
  V* getImpl(const K& k) const {
    auto vsp = ref(k);
    if (!vsp) {
      return nullptr;
    }
    return vsp.get();
  }

  MapType map_;
};

template <typename K, typename V>
using UnorderedRefMap = RefMap<RefMapUMap, K, V>;

template <typename K, typename V>
using FlatRefMap = RefMap<RefMapFlatMap, K, V>;

} // namespace facebook::fboss
