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

template <typename T>
class FakeManager {
 public:
  sai_object_id_t create(const T& t) {
    auto ins = map_.insert({++id_, t});
    ins.first->second.id = id_;
    return id_;
  }
  sai_object_id_t create(T&& t) {
    auto ins = map_.insert({++id_, std::move(t)});
    ins.first->second.id = id_;
    return id_;
  }

  size_t remove(const sai_object_id_t id) {
    return map_.erase(id);
  }

  T& get(const sai_object_id_t& id) {
    return map_.at(id);
  }
  const T& get(const sai_object_id_t& id) const {
    return map_.at(id);
  }

  std::unordered_map<sai_object_id_t, T>& map() {
    return map_;
  }
  const std::unordered_map<sai_object_id_t, T>& map() const {
    return map_;
  }
 private:
  size_t id_ = 0;
  std::unordered_map<sai_object_id_t, T> map_;
};

} // namespace fboss
} // namespace facebook
