// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include <thrift/lib/cpp2/reflection/reflection.h>
#include "folly/Conv.h"

namespace facebook::fboss::thrift_cow {

template <typename Meta>
std::string getMemberName(bool useId) {
  using name = typename Meta::name;
  using id = typename Meta::id;
  if (useId) {
    return folly::to<std::string>(id::value);
  } else {
    return std::string(fatal::z_data<name>(), fatal::size<name>::value);
  }
}

template <typename MemberTypes, typename Func>
void visitMember(const std::string& key, Func&& f) {
  // try to find child using id first, then fallback to name
  auto idTry = folly::tryTo<apache::thrift::field_id_t>(key);
  if (!idTry.hasError()) {
    fatal::scalar_search<MemberTypes, fatal::get_type::id>(
        idTry.value(), std::forward<Func>(f));
  } else {
    fatal::trie_find<MemberTypes, fatal::get_type::name>(
        key.begin(), key.end(), std::forward<Func>(f));
  }
}

} // namespace facebook::fboss::thrift_cow
