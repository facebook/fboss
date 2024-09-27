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

#include <fatal/container/tuple.h>
#include <folly/json/dynamic.h>
#include <thrift/lib/cpp2/folly_dynamic/folly_dynamic.h>
#include <thrift/lib/cpp2/protocol/detail/protocol_methods.h>
#include <thrift/lib/cpp2/reflection/reflection.h>
#include "fboss/agent/state/NodeBase-defs.h"
#include "fboss/thrift_cow/nodes/NodeUtils.h"
#include "fboss/thrift_cow/nodes/Serializer.h"
#include "fboss/thrift_cow/nodes/Types.h"

#include "fboss/agent/state/MapDelta.h"

namespace facebook::fboss::thrift_cow {

namespace map_helpers {

template <typename TC>
struct ExtractTypeClass {
  using type = TC;
};

template <typename KeyTypeClass, typename ValueTypeClass>
struct ExtractTypeClass<
    apache::thrift::type_class::map<KeyTypeClass, ValueTypeClass>> {
  using key_type = KeyTypeClass;
  using value_type = ValueTypeClass;
};

} // namespace map_helpers

template <typename Traits>
struct ThriftMapFields {
  using TypeClass = typename Traits::TC;
  using TType = typename Traits::Type;
  using Self = ThriftMapFields<Traits>;
  using CowType = FieldsType;
  using ThriftType = TType;
  using KeyTypeClass =
      typename map_helpers::ExtractTypeClass<TypeClass>::key_type;
  using ValueTypeClass =
      typename map_helpers::ExtractTypeClass<TypeClass>::value_type;
  using ValueTType = typename TType::mapped_type;
  using ValueTraits =
      typename Traits::template ConvertToNodeTraits<ValueTypeClass, ValueTType>;
  using key_type = typename TType::key_type;
  using value_type = typename ValueTraits::type;
  using StorageType =
      std::map<key_type, value_type, typename Traits::KeyCompare>;
  using iterator = typename StorageType::iterator;
  using const_iterator = typename StorageType::const_iterator;
  using Tag = apache::thrift::type::map<
      apache::thrift::type::infer_tag<key_type, true /* GuessStringTag */>,
      apache::thrift::type::infer_tag<ValueTType, true /* GuessStringTag */>>;

  // whether the contained type is another Cow node, or a primitive node
  static constexpr bool HasChildNodes = ValueTraits::isChild::value;

  // constructors:
  // One takes a thrift type directly, one starts with empty vector

  ThriftMapFields() {}

  template <typename T>
  explicit ThriftMapFields(T&& thrift)
    requires(std::is_same_v<std::decay_t<T>, TType>)
  {
    fromThrift(std::forward<T>(thrift));
  }

  /* implicit */ ThriftMapFields(StorageType storage)
      : storage_(std::move(storage)) {}

  // TODO(pshaikh): fix this redunant constructor
  ThriftMapFields(const ThriftMapFields&, StorageType storage)
      : storage_(std::move(storage)) {}

  TType toThrift() const {
    TType thrift;

    for (auto&& [key, elem] : storage_) {
      thrift.emplace(key, elem->toThrift());
    }
    return thrift;
  }

  template <typename T>
  void fromThrift(T&& thrift)
    requires(std::is_same_v<std::decay_t<T>, TType>)
  {
    storage_.clear();
    for (const auto& [key, elem] : thrift) {
      emplace(key, elem);
    }
  }

#ifdef ENABLE_DYNAMIC_APIS

  folly::dynamic toDynamic() const {
    folly::dynamic out;
    facebook::thrift::to_dynamic<Tag>(
        out, toThrift(), facebook::thrift::dynamic_format::JSON_1);
    return out;
  }

  void fromDynamic(const folly::dynamic& value) {
    TType thrift;
    facebook::thrift::from_dynamic<Tag>(
        thrift, value, facebook::thrift::dynamic_format::JSON_1);
    fromThrift(thrift);
  }
#endif

  folly::fbstring encode(fsdb::OperProtocol proto) const {
    return serialize<TypeClass>(proto, toThrift());
  }

  folly::IOBuf encodeBuf(fsdb::OperProtocol proto) const {
    return serializeBuf<TypeClass>(proto, toThrift());
  }

  void fromEncoded(fsdb::OperProtocol proto, const folly::fbstring& encoded) {
    fromThrift(deserialize<TypeClass, TType>(proto, encoded));
  }

  void fromEncodedBuf(fsdb::OperProtocol proto, folly::IOBuf&& encoded) {
    fromThrift(deserializeBuf<TypeClass, TType>(proto, std::move(encoded)));
  }

  value_type at(key_type key) const {
    return storage_.at(key);
  }

  value_type& operator[](key_type key) {
    return storage_[key];
  }

  value_type& ref(key_type key) {
    return storage_.at(key);
  }

  const value_type& ref(key_type key) const {
    return storage_.at(key);
  }

  const value_type& cref(key_type key) const {
    return storage_.at(key);
  }

  std::pair<iterator, bool> insert(key_type key, value_type&& val) {
    return storage_.insert({key, val});
  }

  template <typename... Args>
  std::pair<iterator, bool> emplace(key_type key, Args&&... args) {
    return storage_.emplace(key, childFactory(std::forward<Args>(args)...));
  }

  template <typename... Args>
  std::pair<iterator, bool> try_emplace(key_type key, Args&&... args) {
    return storage_.try_emplace(key, childFactory(std::forward<Args>(args)...));
  }

  bool remove(const std::string& token) {
    // avoid infinite recursion in case key is string
    if constexpr (std::is_same_v<
                      KeyTypeClass,
                      apache::thrift::type_class::string>) {
      return storage_.erase(token);
    } else if (auto key = tryParseKey<key_type, KeyTypeClass>(token)) {
      return remove(key.value());
    }

    return false;
  }

  template <typename T = Self>
  auto remove(const key_type& key) -> std::enable_if_t<
                                       !std::is_same_v<
                                           typename T::KeyTypeClass,
                                           apache::thrift::type_class::string>,
                                       bool> {
    return storage_.erase(key);
  }

  auto erase(iterator it) {
    return storage_.erase(it);
  }
  // iterators

  iterator begin() {
    return storage_.begin();
  }

  const_iterator begin() const {
    return storage_.cbegin();
  }

  iterator end() {
    return storage_.end();
  }

  const_iterator end() const {
    return storage_.end();
  }

  iterator find(const key_type& key) {
    return storage_.find(key);
  }

  const_iterator find(const key_type& key) const {
    return storage_.find(key);
  }

  size_t count(const key_type& key) const {
    return storage_.count(key);
  }

  const_iterator cbegin() const {
    return storage_.cbegin();
  }

  const_iterator cend() const {
    return storage_.cend();
  }

  std::size_t size() const {
    return storage_.size();
  }

  template <typename Fn>
  void forEachChild(Fn fn) {
    if constexpr (HasChildNodes) {
      for (auto&& [key, value] : storage_) {
        fn(value.get());
      }
    }
  }

  bool operator==(const Self& that) const {
    return storage_ == that.storage_;
  }

  bool operator!=(const Self& that) const {
    return !(*this == that);
  }

  void clear() {
    storage_.clear();
  }

 private:
  template <typename... Args>
  value_type childFactory(Args&&... args) {
    if constexpr (HasChildNodes) {
      return std::make_shared<typename value_type::element_type>(
          std::forward<Args>(args)...);
    } else {
      return std::make_optional<typename value_type::value_type>(
          std::forward<Args>(args)...);
    }
  }

  StorageType storage_;
};

template <typename Traits, typename Resolver = ThriftMapResolver<Traits>>
class ThriftMapNode
    : public NodeBaseT<typename Resolver::type, ThriftMapFields<Traits>>,
      public thrift_cow::Serializable {
 public:
  using TC = typename Traits::TC;
  using TType = typename Traits::Type;

  using Self = ThriftMapNode<Traits, Resolver>;
  using Fields = ThriftMapFields<Traits>;
  using ThriftType = typename Fields::ThriftType;
  using Derived = typename Resolver::type;
  using BaseT = NodeBaseT<Derived, Fields>;
  using CowType = NodeType;
  using key_type = typename Fields::key_type;
  using value_type = typename Fields::value_type;
  using PathIter = typename std::vector<std::string>::const_iterator;
  using mapped_type = value_type;
  using const_iterator = typename Fields::const_iterator;

  using BaseT::BaseT;

  TType toThrift() const {
    return this->getFields()->toThrift();
  }

  void fromThrift(const TType& thrift) {
    return this->writableFields()->fromThrift(thrift);
  }

#ifdef ENABLE_DYNAMIC_APIS
  folly::dynamic toFollyDynamic() const override {
    return this->getFields()->toDynamic();
  }

  void fromFollyDynamic(const folly::dynamic& value) {
    return this->writableFields()->fromDynamic(value);
  }
#else
  folly::dynamic toFollyDynamic() const override {
    return {};
  }
#endif

  folly::IOBuf encodeBuf(fsdb::OperProtocol proto) const override {
    return this->getFields()->encodeBuf(proto);
  }

  void fromEncodedBuf(fsdb::OperProtocol proto, folly::IOBuf&& encoded)
      override {
    return this->writableFields()->fromEncodedBuf(proto, std::move(encoded));
  }

  value_type at(key_type key) const {
    return this->getFields()->at(key);
  }

  value_type& operator[](key_type key) {
    return this->writableFields()->operator[](key);
  }

  value_type& ref(key_type key) {
    return this->writableFields()->ref(key);
  }

  const value_type& ref(key_type key) const {
    return this->getFields()->ref(key);
  }

  const value_type& cref(key_type key) const {
    return this->getFields()->cref(key);
  }

  // prefer safe_ref/safe_cref for safe access
  auto safe_ref(key_type key) {
    return detail::ref(this->writableFields()->ref(key));
  }

  auto safe_cref(key_type key) const {
    return detail::cref(this->getFields()->cref(key));
  }

  std::pair<typename Fields::iterator, bool> insert(
      key_type key,
      value_type&& val) {
    return this->writableFields()->insert(key, std::move(val));
  }

  template <typename... Args>
  typename std::pair<typename Fields::iterator, bool> emplace(
      key_type key,
      Args&&... args) {
    return this->writableFields()->emplace(key, std::forward<Args>(args)...);
  }

  template <typename... Args>
  typename std::pair<typename Fields::iterator, bool> try_emplace(
      key_type key,
      Args&&... args) {
    return this->writableFields()->try_emplace(
        key, std::forward<Args>(args)...);
  }

  bool remove(const std::string& token) {
    return this->writableFields()->remove(token);
  }

  template <typename T = Fields>
  auto remove(const key_type& key) -> std::enable_if_t<
                                       !std::is_same_v<
                                           typename T::KeyTypeClass,
                                           apache::thrift::type_class::string>,
                                       bool> {
    return this->writableFields()->remove(key);
  }

  auto erase(typename Fields::iterator it) {
    return this->writableFields()->erase(it);
  }

  // iterators

  typename Fields::iterator begin() {
    return this->writableFields()->begin();
  }

  typename Fields::const_iterator begin() const {
    return this->getFields()->begin();
  }

  typename Fields::iterator end() {
    return this->writableFields()->end();
  }

  typename Fields::const_iterator end() const {
    return this->getFields()->end();
  }

  typename Fields::const_iterator cbegin() const {
    return this->getFields()->cbegin();
  }

  typename Fields::const_iterator cend() const {
    return this->getFields()->cend();
  }

  typename Fields::iterator find(const key_type& key) {
    return this->writableFields()->find(key);
  }

  typename Fields::const_iterator find(const key_type& key) const {
    return this->getFields()->find(key);
  }

  size_t count(const key_type& key) const {
    return this->getFields()->count(key);
  }

  std::size_t size() const {
    return this->getFields()->size();
  }

  bool empty() const {
    return size() == 0;
  }

  bool tryModify(const std::string& token, bool construct = true) {
    if (auto parsedKey =
            tryParseKey<key_type, typename Fields::KeyTypeClass>(token)) {
      modifyTyped(parsedKey.value(), construct);
      return true;
    }
    return false;
  }

  void modify(const std::string& token, bool construct = true) {
    if (!tryModify(token, construct)) {
      throw std::runtime_error(folly::to<std::string>("Invalid key: ", token));
    }
  }

  virtual void modifyTyped(key_type key, bool construct = true) {
    DCHECK(!this->isPublished());

    if (auto it = this->find(key); it != this->end()) {
      if constexpr (Fields::HasChildNodes) {
        auto& child = it->second;
        if (child->isPublished()) {
          auto clonedChild = child->clone();
          child.swap(clonedChild);
        }
      }
    } else if (construct) {
      // create unpublished default constructed child if missing
      this->emplace(key).first->second;
    }
  }

  static void modify(std::shared_ptr<Self>* node, std::string token) {
    auto newNode = ((*node)->isPublished()) ? (*node)->clone() : *node;
    newNode->modify(token);
    node->swap(newNode);
  }

  bool operator==(const Self& that) const {
    return *this->getFields() == *that.getFields();
  }
  bool operator!=(const Self& that) const {
    return !(*this == that);
  }

  void clear() {
    this->writableFields()->clear();
  }

 private:
  friend class CloneAllocator;
};

} // namespace facebook::fboss::thrift_cow
