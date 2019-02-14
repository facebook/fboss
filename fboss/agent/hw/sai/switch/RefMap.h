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

namespace facebook {
namespace fboss {

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
template <typename K, typename V>
class RefMap {
 public:
  using MapType = std::unordered_map<K, std::weak_ptr<V>>;
  RefMap() {}
  RefMap(const RefMap& other) = delete;
  RefMap& operator=(const RefMap& other) = delete;

  template <typename... Args>
  std::shared_ptr<V> refOrEmplace(const K& k, Args&&... args) {
    std::shared_ptr<V> vsp;
    auto itr = map_.find(k);
    if (itr != map_.end()) {
      vsp = itr->second.lock();
    } else {
      auto del = [&m = map_, k](V* v) {
        m.erase(k);
        std::default_delete<V>()(v);
      };
      vsp = std::shared_ptr<V>(new V(std::forward<Args>(args)...), del);
      map_[k] = vsp;
    }
    return vsp;
  }

  std::size_t size() const {
    return map_.size();
  }

  V* get(const K& k) {
    return getImpl(k);
  }

  const V* get(const K& k) const {
    return getImpl(k);
  }

 private:
  V* getImpl(const K& k) const {
    auto itr = map_.find(k);
    if (itr == map_.end()) {
      return nullptr;
    }
    return itr->second.lock().get();
  }

  MapType map_;
};
} // namespace fboss
} // namespace facebook
