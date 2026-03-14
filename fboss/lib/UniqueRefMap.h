// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include <cstdint>
#include <map>
#include <stdexcept>
#include <utility>

namespace facebook::fboss {

/*
 * A reference-counted map where each key maps to exactly one value.
 * Multiple refs to the same (key, value) are counted. Attempting to
 * ref a key with a different value returns false (conflict).
 * Entry is removed when refcount drops to zero.
 */
template <typename K, typename V>
class UniqueRefMap {
 public:
  // Returns true if new entry placed or refcount incremented (same value).
  // Returns false if key exists with a different value (no-op).
  bool ref(const K& key, const V& value) {
    auto it = map_.find(key);
    if (it != map_.end()) {
      if (it->second.first != value) {
        return false;
      }
      ++it->second.second;
      return true;
    }
    map_.emplace(key, std::make_pair(value, 1));
    return true;
  }

  // Decrement refcount for key. Removes entry when refcount hits 0.
  // Returns true if entry removed, false if only decremented.
  // Throws std::out_of_range if key not found.
  bool unref(const K& key) {
    auto it = map_.find(key);
    if (it == map_.end()) {
      throw std::out_of_range("UniqueRefMap::unref: key not found");
    }
    if (--it->second.second == 0) {
      map_.erase(it);
      return true;
    }
    return false;
  }

  // Get value for key, or nullptr if not present.
  const V* get(const K& key) const {
    auto it = map_.find(key);
    if (it == map_.end()) {
      return nullptr;
    }
    return &it->second.first;
  }

  std::size_t size() const {
    return map_.size();
  }

  bool empty() const {
    return map_.empty();
  }

  // Force-remove a key regardless of refcount.
  void erase(const K& key) {
    map_.erase(key);
  }

  // Directly set a key-value pair with a specific refcount.
  void set(const K& key, const V& value, uint32_t refCount) {
    map_[key] = std::make_pair(value, refCount);
  }

  void clear() {
    map_.clear();
  }

 private:
  std::map<K, std::pair<V, uint32_t>> map_;
};

} // namespace facebook::fboss
