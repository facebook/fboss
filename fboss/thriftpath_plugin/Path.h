// (c) Facebook, Inc. and its affiliates. Confidential and proprietary.

#pragma once

#include <folly/String.h>
#include <folly/Unit.h>
#include <string>

namespace thriftpath {

#define STRUCT_CHILD_GETTERS(child, childId)                                 \
  TypeFor<strings::child> child() const& {                                   \
    return TypeFor<strings::child>(                                          \
        copyAndExtendVec(this->tokens_, #child),                             \
        copyAndExtendVec(this->idTokens_, folly::to<std::string>(childId))); \
  }                                                                          \
  TypeFor<strings::child> child()&& {                                        \
    this->tokens_.push_back(#child);                                         \
    this->idTokens_.push_back(folly::to<std::string>(childId));              \
    return TypeFor<strings::child>(                                          \
        std::move(this->tokens_), std::move(this->idTokens_));               \
  }
#define CONTAINER_CHILD_GETTERS(key_type)                               \
  Child operator[](key_type token) const& {                             \
    const std::string strToken = folly::to<std::string>(token);         \
    return Child(                                                       \
        copyAndExtendVec(this->tokens_, strToken),                      \
        copyAndExtendVec(this->idTokens_, strToken));                   \
  }                                                                     \
  Child operator[](key_type token)&& {                                  \
    const std::string strToken = folly::to<std::string>(token);         \
    this->tokens_.push_back(strToken);                                  \
    this->idTokens_.push_back(strToken);                                \
    return Child(std::move(this->tokens_), std::move(this->idTokens_)); \
  }

template <
    typename _DataT,
    typename _RootT,
    typename _TC,
    typename _Tag,
    typename _ParentT>
class Path {
 public:
  using DataT = _DataT;
  using RootT = _RootT;
  using TC = _TC;
  using Tag = _Tag;
  using ParentT = _ParentT;

  Path(
      std::vector<std::string> tokens,
      std::vector<std::string> idTokens,
      ParentT&& parent)
      : tokens_(std::move(tokens)),
        idTokens_(std::move(idTokens)),
        parent_(std::forward<ParentT>(parent)) {}

  Path(std::vector<std::string> tokens, std::vector<std::string> idTokens)
      : Path(tokens, idTokens, [&]() -> ParentT {
          if constexpr (std::is_same_v<ParentT, folly::Unit>) {
            assert(tokens.empty());
            assert(idTokens.empty());
            return folly::unit;
          } else {
            assert(!tokens.empty());
            return ParentT(
                std::vector<std::string>(
                    tokens.begin(), std::prev(tokens.end())),
                std::vector<std::string>(
                    idTokens.begin(), std::prev(idTokens.end())));
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

  const std::vector<std::string>& idTokens() const {
    return idTokens_;
  }

  const ParentT& parent() {
    return parent_;
  }

  bool matchesPath(const std::vector<std::string>& other) const {
    return other == idTokens_ || other == tokens_;
  }

 protected:
  std::vector<std::string> tokens_;
  std::vector<std::string> idTokens_;

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
