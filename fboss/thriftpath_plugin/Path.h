// (c) Facebook, Inc. and its affiliates. Confidential and proprietary.

#pragma once

#include <folly/String.h>
#include <folly/Unit.h>
#include <thrift/lib/cpp2/TypeClass.h>

namespace thriftpath {

#define STRUCT_CHILD_GETTERS(child)                                          \
  TypeFor<strings::child> child() const& {                                   \
    return TypeFor<strings::child>(copyAndExtendVec(this->tokens_, #child)); \
  }                                                                          \
  TypeFor<strings::child> child()&& {                                        \
    this->tokens_.push_back(#child);                                         \
    return TypeFor<strings::child>(std::move(this->tokens_));                \
  }
#define CONTAINER_CHILD_GETTERS(key_type)                                \
  Child operator[](key_type token) const& {                              \
    return Child(                                                        \
        copyAndExtendVec(this->tokens_, folly::to<std::string>(token))); \
  }                                                                      \
  Child operator[](key_type token)&& {                                   \
    this->tokens_.push_back(folly::to<std::string>(token));              \
    return Child(std::move(this->tokens_));                              \
  }

template <typename _DataT, typename _RootT, typename _TC, typename _ParentT>
class Path {
 public:
  using DataT = _DataT;
  using RootT = _RootT;
  using TC = _TC;
  using ParentT = _ParentT;

  Path(std::vector<std::string> tokens, ParentT&& parent)
      : tokens_(std::move(tokens)), parent_(std::forward<ParentT>(parent)) {}

  explicit Path(std::vector<std::string> tokens)
      : Path(tokens, [&]() -> ParentT {
          if constexpr (std::is_same_v<ParentT, folly::Unit>) {
            assert(tokens.empty());
            return folly::unit;
          } else {
            assert(!tokens.empty());
            return ParentT(std::vector<std::string>(
                tokens.begin(), std::prev(tokens.end())));
          }
        }()) {}

  std::string str() const {
    // TODO: better format
    return "/" + folly::join('/', tokens_.begin(), tokens_.end());
  }

  constexpr bool root() const {
    return std::is_same_v<ParentT, folly::Unit>;
  }

  auto begin() const {
    return tokens_.cbegin();
  }

  auto end() const {
    return tokens_.cend();
  }

  const std::vector<std::string>& tokens() const {
    return tokens_;
  }

  const ParentT& parent() {
    return parent_;
  }

 protected:
  std::vector<std::string> tokens_;

 private:
  // TODO: reduce memory usage by only storing begin/end in parent
  // paths as a view type
  ParentT parent_;
};

template <typename T>
class RootThriftPath {
 public:
  // While this is always-false, it is dependent and therefore fires only
  // at instantiation time.
  static_assert(
      !std::is_same<T, T>::value,
      "You need to include the header file that the thriftpath plugin "
      "generated for T in order to use RootThriftPath<T>. Also ensure that "
      "you have annotated your root struct with (thriftpath.root)");
};

template <typename T, typename Root, typename Parent>
class ChildThriftPath {
 public:
  // While this is always-false, it is dependent and therefore fires only
  // at instantiation time.
  static_assert(
      !std::is_same<T, T>::value,
      "You need to include the header file that the thriftpath plugin "
      "generated for T in order to use ChildThriftPath<T>.");
};

std::vector<std::string> copyAndExtendVec(
    const std::vector<std::string>& parents,
    std::string last);

} // namespace thriftpath
