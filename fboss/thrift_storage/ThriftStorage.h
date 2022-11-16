// (c) Facebook, Inc. and its affiliates. Confidential and proprietary.

#pragma once

#include <fboss/thrift_storage/Storage.h>
#include <fboss/thrift_storage/visitors/ThriftPathVisitor.h>
#include <folly/logging/xlog.h>
#include <thrift/lib/cpp2/reflection/folly_dynamic.h>

namespace facebook::fboss::fsdb {

namespace detail {

template <typename>
struct tc_is_list : std::false_type {};

template <typename V>
struct tc_is_list<apache::thrift::type_class::list<V>> : std::true_type {
  using value_tc = V;
};

template <typename V>
constexpr bool tc_is_list_v = detail::tc_is_list<V>::value;

template <typename>
struct tc_is_map : std::false_type {};

template <typename K, typename V>
struct tc_is_map<apache::thrift::type_class::map<K, V>> : std::true_type {
  using key_tc = K;
  using value_tc = V;
};

template <typename V>
constexpr bool tc_is_map_v = detail::tc_is_map<V>::value;

template <typename>
struct tc_is_set : std::false_type {};

template <typename V>
struct tc_is_set<apache::thrift::type_class::set<V>> : std::true_type {
  using value_tc = V;
};

template <typename V>
constexpr bool tc_is_set_v = detail::tc_is_set<V>::value;

template <typename ContainerValType, typename Value>
constexpr bool isRefAssignable =
    std::is_assignable_v<std::add_lvalue_reference_t<ContainerValType>, Value>;
} // namespace detail

template <typename Root>
class ThriftStorage : public Storage<Root, ThriftStorage<Root>> {
 public:
  using Base = Storage<Root, ThriftStorage<Root>>;
  using Self = ThriftStorage<Root>;
  using PathIter = typename Base::PathIter;

  explicit ThriftStorage(Root obj) : obj_(std::move(obj)) {}

  // ThriftStorage has no notion of publish or not
  bool isPublished_impl() {
    return true;
  }
  void publish_impl() {}

  using Base::get;
  using Base::get_encoded;
  using Base::remove;
  using Base::set;
  using Base::set_encoded;

  template <typename T>
  typename Base::template Result<T> get_impl(PathIter begin, PathIter end)
      const {
    T out;
    auto traverseResult = RootThriftPathVisitor::visit(
        obj_, begin, end, [&](auto&& val, auto /* tc */) {
          if constexpr (std::is_assignable_v<decltype(out)&, decltype(val)>) {
            out = val;
          }
        });
    if (traverseResult == ThriftTraverseResult::OK) {
      return out;
    } else if (traverseResult == ThriftTraverseResult::VISITOR_EXCEPTION) {
      return folly::makeUnexpected(StorageError::TYPE_ERROR);
    } else {
      return folly::makeUnexpected(StorageError::INVALID_PATH);
    }
  }

  typename Base::template Result<OperState> get_encoded_impl(
      PathIter /*begin*/,
      PathIter /*end*/,
      OperProtocol /*protocol*/) const {
    // TODO
    return {};
  }

  template <typename T>
  std::optional<StorageError>
  set_impl(PathIter begin, PathIter end, T&& value) {
    auto traverseResult = RootThriftPathVisitor::visit(
        obj_, begin, end, [&](auto& val, auto /* tc */) {
          if constexpr (std::is_assignable_v<decltype(val), decltype(value)>) {
            val = std::forward<T>(value);
          }
        });
    if (traverseResult == ThriftTraverseResult::OK) {
      return std::nullopt;
    } else if (traverseResult == ThriftTraverseResult::VISITOR_EXCEPTION) {
      return StorageError::TYPE_ERROR;
    } else {
      return StorageError::INVALID_PATH;
    }
  }

  template <typename T>
  std::optional<StorageError> set_encoded_impl(
      PathIter /*begin*/,
      PathIter /*end*/,
      const OperState& /*state*/) {
    // TODO
    throw std::logic_error("Not implemented");
  }

  std::optional<StorageError> patch_impl(const fsdb::OperDelta& /*delta*/) {
    // TODO
    throw std::logic_error("Not implemented");
  }

  std::optional<StorageError> patch_impl(
      const fsdb::TaggedOperState& /*delta*/) {
    // TODO
    throw std::logic_error("Not implemented");
  }

  template <typename T>
  std::optional<StorageError>
  add_impl(PathIter begin, PathIter end, T&& value) {
    if (begin == end) {
      return std::nullopt;
    }

    auto parentEnd = std::prev(end);

    auto strKey = *parentEnd;

    auto traverseResult = RootThriftPathVisitor::visit(
        obj_, begin, parentEnd, [&](auto& val, auto tc) {
          using ValT = folly::remove_cvref_t<decltype(val)>;
          if constexpr (detail::tc_is_list_v<decltype(tc)>) {
            if constexpr (detail::
                              isRefAssignable<typename ValT::value_type, T>) {
              auto tryKey = folly::tryTo<int>(strKey);
              if (tryKey.hasError()) {
                throw std::runtime_error(
                    "Must use integer index (or -1) for adding to list");
              }
              auto key = tryKey.value();
              // semantics for list are:
              // 1. if -1, that signals adding to the end
              // 2. if 0 <= i <= list.size(), we insert the value at that index,
              // shifting others if needed.
              // 3. error otherwise
              if (key == -1) {
                // TODO: might not be safe to assume vector type.
                val.push_back(std::forward<T>(value));
              } else if (key >= 0 && key <= val.size()) {
                auto it = std::next(val.begin(), key);
                val.insert(it, std::forward<T>(value));
              } else {
                throw std::runtime_error(
                    folly::to<std::string>("Invalid index for list: ", key));
              }
            }
          } else if constexpr (detail::tc_is_map_v<decltype(tc)>) {
            if constexpr (detail::
                              isRefAssignable<typename ValT::mapped_type, T>) {
              auto tryKey = folly::tryTo<typename ValT::key_type>(strKey);
              if (tryKey.hasError()) {
                throw std::runtime_error(folly::to<std::string>(
                    "Unable to convert to proper key type: ", strKey));
              }
              auto key = tryKey.value();

              if (val.find(key) != val.end()) {
                throw std::runtime_error(
                    "Key already exists in map: " + strKey);
              }
              val.emplace(std::move(key), std::forward<T>(value));
            } else {
              throw std::runtime_error(
                  "Invalid value add for map at key: " + strKey);
            }
          } else if constexpr (detail::tc_is_set_v<decltype(tc)>) {
            if constexpr (detail::
                              isRefAssignable<typename ValT::value_type, T>) {
              if (val.find(value) != val.end()) {
                throw std::runtime_error("Value already exists in set");
              }
              auto tryKey = folly::tryTo<typename ValT::key_type>(strKey);
              if (tryKey.hasError()) {
                throw std::runtime_error(folly::to<std::string>(
                    "Unable to convert to proper key type: ", strKey));
              }
              if (tryKey.value() != std::forward<T>(value)) {
                throw std::runtime_error("For sets, key and value must match");
              }
              val.emplace(std::forward<T>(value));
            } else {
              throw std::runtime_error(
                  "Invalid value add for set at : " + strKey);
            }
          } else {
            throw std::runtime_error(
                "Add operation is only supported on list, map or set types");
          }
        });
    if (traverseResult == ThriftTraverseResult::OK) {
      return std::nullopt;
    } else if (traverseResult == ThriftTraverseResult::VISITOR_EXCEPTION) {
      return StorageError::TYPE_ERROR;
    } else {
      return StorageError::INVALID_PATH;
    }
  }

  std::optional<StorageError> remove_impl(PathIter begin, PathIter end) {
    if (begin == end) {
      // TODO
      return std::nullopt;
    }

    auto parentEnd = std::prev(end);

    auto strKey = *parentEnd;

    auto traverseResult = RootThriftPathVisitor::visit(
        obj_, begin, parentEnd, [&](auto& val, auto tc) {
          if constexpr (detail::tc_is_list_v<decltype(tc)>) {
            auto tryKey = folly::tryTo<int>(strKey);
            if (tryKey.hasError()) {
              throw std::runtime_error(
                  "Must use integer index (or -1) for adding to list");
            }
            auto key = tryKey.value();
            // semantics for list are:
            // 1. if -1, pop from end
            // 2. if 0 <= i < list.size(), we remove the value at that index,
            // shifting others if needed.
            // 3. error otherwise
            if (key == -1) {
              // TODO: might not be safe to assume vector type.
              val.pop_back();
            } else if (key >= 0 && key <= val.size()) {
              auto it = std::next(val.begin(), key);
              val.erase(it);
            } else {
              throw std::runtime_error(
                  folly::to<std::string>("Invalid index for list: ", key));
            }
          } else if constexpr (
              detail::tc_is_map_v<decltype(tc)> ||
              detail::tc_is_set_v<decltype(tc)>) {
            auto tryKey = folly::tryTo<
                typename folly::remove_cvref_t<decltype(val)>::key_type>(
                strKey);
            if (tryKey.hasError()) {
              throw std::runtime_error(folly::to<std::string>(
                  "Unable to convert to proper key type: ", strKey));
            }
            auto key = tryKey.value();

            if (auto it = val.find(key); it != val.end()) {
              val.erase(it);
            } else {
              throw std::runtime_error(
                  "Key does not exist in container: " + strKey);
            }
          } else {
            throw std::runtime_error(
                "Add operation is only supported on list, map or set types");
          }
        });

    if (traverseResult == ThriftTraverseResult::OK) {
      return std::nullopt;
    } else {
      return StorageError::INVALID_PATH;
    }
  }

#ifdef ENABLE_DYNAMIC_APIS
  using Base::add;
  using Base::add_dynamic;
  using Base::get_dynamic;
  using Base::set_dynamic;

  typename Base::DynamicResult get_dynamic_impl(PathIter begin, PathIter end)
      const {
    folly::dynamic out;
    auto traverseResult = RootThriftPathVisitor::visit(
        obj_, begin, end, [&](auto&& val, auto tc) {
          out = apache::thrift::to_dynamic<decltype(tc)>(
              val, apache::thrift::dynamic_format::JSON_1);
        });
    if (traverseResult == ThriftTraverseResult::OK) {
      return out;
    } else if (traverseResult == ThriftTraverseResult::VISITOR_EXCEPTION) {
      return folly::makeUnexpected(StorageError::TYPE_ERROR);
    } else {
      return folly::makeUnexpected(StorageError::INVALID_PATH);
    }
  }

  std::optional<StorageError>
  set_dynamic_impl(PathIter begin, PathIter end, const folly::dynamic& value) {
    auto traverseResult =
        RootThriftPathVisitor::visit(obj_, begin, end, [&](auto& val, auto tc) {
          apache::thrift::from_dynamic<decltype(tc)>(
              val, value, apache::thrift::dynamic_format::JSON_1);
        });
    if (traverseResult == ThriftTraverseResult::OK) {
      return std::nullopt;
    } else if (traverseResult == ThriftTraverseResult::VISITOR_EXCEPTION) {
      return StorageError::TYPE_ERROR;
    } else {
      return StorageError::INVALID_PATH;
    }
  }

  std::optional<StorageError>
  add_dynamic_impl(PathIter begin, PathIter end, const folly::dynamic& value) {
    if (begin == end) {
      // TODO
      return std::nullopt;
    }

    auto parentEnd = std::prev(end);

    auto strKey = *parentEnd;

    auto traverseResult = RootThriftPathVisitor::visit(
        obj_, begin, parentEnd, [&](auto& val, auto tc) {
          using ValT = folly::remove_cvref_t<decltype(val)>;
          if constexpr (detail::tc_is_list_v<decltype(tc)>) {
            auto tryKey = folly::tryTo<int>(strKey);
            if (tryKey.hasError()) {
              throw std::runtime_error(
                  "Must use integer index (or -1) for adding to list");
            }
            auto key = tryKey.value();

            using value_tc =
                typename detail::tc_is_list<decltype(tc)>::value_tc;
            typename ValT::value_type toAdd;
            apache::thrift::from_dynamic<value_tc>(
                toAdd, value, apache::thrift::dynamic_format::JSON_1);
            // semantics for list are:
            // 1. if -1, that signals adding to the end
            // 2. if 0 <= i <= list.size(), we insert the value at that index,
            // shifting others if needed.
            // 3. error otherwise
            if (key == -1) {
              // TODO: might not be safe to assume vector type.
              val.push_back(std::move(toAdd));
            } else if (key >= 0 && key <= val.size()) {
              auto it = std::next(val.begin(), key);
              val.insert(it, std::move(toAdd));
            } else {
              throw std::runtime_error(
                  folly::to<std::string>("Invalid index for list: ", key));
            }
          } else if constexpr (detail::tc_is_map_v<decltype(tc)>) {
            auto tryKey = folly::tryTo<typename ValT::key_type>(strKey);
            if (tryKey.hasError()) {
              throw std::runtime_error(folly::to<std::string>(
                  "Unable to convert to proper key type: ", strKey));
            }
            auto key = tryKey.value();

            if (val.find(key) != val.end()) {
              throw std::runtime_error("Key already exists in map: " + strKey);
            }

            using value_tc = typename detail::tc_is_map<decltype(tc)>::value_tc;
            typename ValT::mapped_type toAdd;
            apache::thrift::from_dynamic<value_tc>(
                toAdd, value, apache::thrift::dynamic_format::JSON_1);

            val.emplace(std::move(key), std::move(toAdd));
          } else if constexpr (detail::tc_is_set_v<decltype(tc)>) {
            auto tryKey = folly::tryTo<typename ValT::key_type>(strKey);
            if (tryKey.hasError()) {
              throw std::runtime_error(folly::to<std::string>(
                  "Unable to convert to proper key type: ", strKey));
            }
            if (val.find(tryKey.value()) != val.end()) {
              throw std::runtime_error(
                  "Value already exists in set: " + strKey);
            }
            using value_tc = typename detail::tc_is_set<decltype(tc)>::value_tc;
            typename ValT::value_type toAdd;
            apache::thrift::from_dynamic<value_tc>(
                toAdd, value, apache::thrift::dynamic_format::JSON_1);
            if (tryKey.value() != toAdd) {
              throw std::runtime_error("For sets, key and value must match");
            }
            val.emplace(std::move(toAdd));
          } else {
            throw std::runtime_error(
                "Add operation is only supported on list, map or set types");
          }
        });
    if (traverseResult == ThriftTraverseResult::OK) {
      return std::nullopt;
    } else if (traverseResult == ThriftTraverseResult::VISITOR_EXCEPTION) {
      return StorageError::TYPE_ERROR;
    } else {
      return StorageError::INVALID_PATH;
    }
  }
#endif

  const Root& root() {
    return obj_;
  }

 private:
  Root obj_;
};

} // namespace facebook::fboss::fsdb
