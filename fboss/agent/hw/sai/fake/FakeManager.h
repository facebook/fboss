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

#include <folly/logging/xlog.h>

#include <stdexcept>
#include <unordered_map>

extern "C" {
#include <sai.h>
}

#include "fboss/agent/hw/sai/api/SaiVersion.h"

namespace facebook::fboss {

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
    if (!ins.second) {
      throw std::runtime_error("Object already exists, create failed");
    }
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

  void clear() {
    count_ = 0;
    map_.clear();
  }

 private:
  static size_t count_;
  std::unordered_map<K, T> map_;
};

template <typename K, typename T>
size_t FakeManager<K, T>::count_ = 0;

/*
 * For managing fakes of sai apis that have a membership concept, we will
 * nest fake managers. In this class template, GroupT denotes an owning "group"
 * fake object for something like a vlan or a bridge. MemberT is the fake for
 * the member type, like a vlan member or a bridge port.
 *
 * The parent fake needs to have a FakeManager<sai_object_id_t, MemberT> of its
 * own for managing its members, exposed by the function fm()
 */
template <typename GroupT, typename MemberT>
class FakeManagerWithMembers : public FakeManager<sai_object_id_t, GroupT> {
 public:
  template <typename... Args>
  sai_object_id_t createMember(sai_object_id_t groupId, Args&&... args) {
    GroupT& group = this->get(groupId);
    sai_object_id_t memberId = group.fm().create(std::forward<Args>(args)...);
    memberToGroupMap_[memberId] = groupId;
    return memberId;
  }
  size_t removeMember(sai_object_id_t memberId) {
    GroupT& group = this->get(memberToGroupMap_.at(memberId));
    memberToGroupMap_.erase(memberId);
    return group.fm().remove(memberId);
  }
  MemberT& getMember(sai_object_id_t memberId) {
    GroupT& group = this->get(memberToGroupMap_.at(memberId));
    return group.fm().get(memberId);
  }
  const MemberT& getMember(sai_object_id_t memberId) const {
    const GroupT& group = this->get(memberToGroupMap_.at(memberId));
    return group.fm().get(memberId);
  }
  void clearWithMembers() {
    for (auto entry : memberToGroupMap_) {
      GroupT& group = this->get(memberToGroupMap_.at(entry.first));
      group.fm().clear();
    }
    memberToGroupMap_.clear();
    this->clear();
  }

 private:
  std::unordered_map<sai_object_id_t, sai_object_id_t> memberToGroupMap_;
};

} // namespace facebook::fboss
