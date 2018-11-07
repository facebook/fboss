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

#include <unordered_map>

extern "C" {
  #include <sai.h>
}

namespace facebook {
namespace fboss {

template <typename K, typename T>
class FakeManager {
 public:
  template <typename E = K, typename... Args>
  typename std::
      enable_if<std::is_same<E, sai_object_id_t>::value, sai_object_id_t>::type
      create(Args&&... args) {
    sai_object_id_t id = static_cast<sai_object_id_t>(count_++);
    auto ins = map_.emplace(id, T{std::forward<Args>(args)...});
    ins.first->second.id = id;
    return id;
  }

  template <typename E = K, typename... Args>
  typename std::enable_if<!std::is_same<E, sai_object_id_t>::value, void>::type
  create(const K& k, Args&&... args) {
    auto ins = map_.emplace(k, T{std::forward<Args>(args)...});
    count_++;
  }

  size_t remove(const K& k) {
    return map_.erase(k);
  }

  T& get(const K& k) {
    return map_.at(k);
  }
  const T& get(const K& k) const {
    return map_.at(k);
  }

  std::unordered_map<K, T>& map() {
    return map_;
  }
  const std::unordered_map<K, T>& map() const {
    return map_;
  }
 private:
  size_t count_ = 0;
  std::unordered_map<K, T> map_;
};

} // namespace fboss
} // namespace facebook
