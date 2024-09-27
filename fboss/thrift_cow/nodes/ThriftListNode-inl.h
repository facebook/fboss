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
#include <thrift/lib/cpp2/reflection/reflection.h>
#include "fboss/agent/state/NodeBase-defs.h"
#include "fboss/thrift_cow/nodes/Serializer.h"
#include "fboss/thrift_cow/nodes/Types.h"

namespace facebook::fboss::thrift_cow {

namespace list_helpers {

template <typename TC>
struct ExtractValueTypeClass {
  static_assert(std::is_same_v<TC, TC>, "Should never get here");
};

template <typename ValueTypeClass>
struct ExtractValueTypeClass<apache::thrift::type_class::list<ValueTypeClass>> {
  using type = ValueTypeClass;
};

} // namespace list_helpers

template <typename TypeClass, typename TType>
struct ThriftListFields {
  using Self = ThriftListFields<TypeClass, TType>;
  using CowType = FieldsType;
  using ThriftType = TType;
  using ChildTypeClass =
      typename list_helpers::ExtractValueTypeClass<TypeClass>::type;
  using ChildTType = typename TType::value_type;
  using ChildTraits = ConvertToNodeTraits<ChildTypeClass, ChildTType>;
  using value_type = typename ChildTraits::type;
  using StorageType = std::vector<value_type>;
  using iterator = typename StorageType::iterator;
  using const_iterator = typename StorageType::const_iterator;
  using Tag = apache::thrift::type::list<
      apache::thrift::type::infer_tag<ChildTType, true /* GuessStringTag */>>;

  // whether the contained type is another Cow node, or a primitive
  static constexpr bool HasChildNodes = ChildTraits::isChild::value;

  // constructors:
  // One takes a thrift type directly, one starts with empty vector

  ThriftListFields() {}

  template <typename T>
  explicit ThriftListFields(T&& thrift)
    requires(std::is_same_v<std::decay_t<T>, TType>)
  {
    fromThrift(std::forward<T>(thrift));
  }

  TType toThrift() const {
    TType thrift;
    for (auto& elem : storage_) {
      thrift.push_back(elem->toThrift());
    }
    return thrift;
  }

  template <typename T>
  void fromThrift(T&& thrift)
    requires(std::is_same_v<std::decay_t<T>, TType>)
  {
    storage_.clear();
    for (const auto& elem : thrift) {
      emplace_back(elem);
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

  value_type at(std::size_t index) const {
    return storage_.at(index);
  }

  value_type& ref(std::size_t index) {
    return storage_.at(index);
  }

  const value_type& ref(std::size_t index) const {
    return storage_.at(index);
  }

  const value_type& cref(std::size_t index) const {
    return storage_.at(index);
  }

  template <typename... Args>
  iterator emplace(const_iterator pos, Args&&... args) {
    return storage_.emplace(pos, childFactory(std::forward<Args>(args)...));
  }

  template <typename... Args>
  value_type& emplace_back(Args&&... args) {
    return storage_.emplace_back(childFactory(std::forward<Args>(args)...));
  }

  void resize(std::size_t size) {
    storage_.resize(size);
  }

  bool remove(const std::string& token) {
    auto index = folly::tryTo<std::size_t>(token);
    if (index.hasValue()) {
      return remove(index.value());
    }

    // could raise on incompatible index strings...
    return false;
  }

  bool remove(std::size_t index) {
    if (index < this->size()) {
      storage_.erase(begin() + index);
      return true;
    }
    return false;
  }

  // iterators

  iterator begin() {
    return storage_.begin();
  }

  iterator end() {
    return storage_.end();
  }

  const_iterator begin() const {
    return storage_.cbegin();
  }

  const_iterator end() const {
    return storage_.cend();
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
      for (auto& child : storage_) {
        fn(child.get());
      }
    }
  }

  const StorageType& impl() const {
    return storage_;
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

template <typename TypeClass, typename TType>
class ThriftListNode : public NodeBaseT<
                           ThriftListNode<TypeClass, TType>,
                           ThriftListFields<TypeClass, TType>>,
                       public thrift_cow::Serializable {
 public:
  using TC = TypeClass;
  using Self = ThriftListNode<TypeClass, TType>;
  using Fields = ThriftListFields<TypeClass, TType>;
  using ThriftType = typename Fields::ThriftType;
  using BaseT = NodeBaseT<ThriftListNode<TypeClass, TType>, Fields>;
  using CowType = NodeType;
  using value_type = typename Fields::value_type;
  using PathIter = typename std::vector<std::string>::const_iterator;

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

  value_type at(std::size_t index) const {
    return this->getFields()->at(index);
  }

  value_type& ref(std::size_t index) {
    return this->writableFields()->ref(index);
  }

  const value_type& ref(std::size_t index) const {
    return this->writableFields()->ref(index);
  }

  const value_type& cref(std::size_t index) const {
    return this->getFields()->cref(index);
  }

  // prefer safe_ref/safe_cref for safe access
  auto safe_ref(std::size_t index) {
    return detail::ref(this->writableFields()->ref(index));
  }

  auto safe_cref(std::size_t index) const {
    return detail::cref(this->getFields()->cref(index));
  }

  template <typename... Args>
  typename Fields::iterator emplace(
      typename Fields::const_iterator pos,
      Args&&... args) {
    return this->writableFields()->emplace(pos, std::forward<Args>(args)...);
  }

  template <typename... Args>
  value_type& emplace_back(Args&&... args) {
    return this->writableFields()->emplace_back(std::forward<Args>(args)...);
  }

  // iterators

  typename Fields::iterator begin() {
    return this->writableFields()->begin();
  }

  typename Fields::iterator end() {
    return this->writableFields()->end();
  }

  typename Fields::const_iterator begin() const {
    return this->getFields()->cbegin();
  }

  typename Fields::const_iterator end() const {
    return this->getFields()->cend();
  }

  typename Fields::const_iterator cbegin() const {
    return this->getFields()->cbegin();
  }

  typename Fields::const_iterator cend() const {
    return this->getFields()->cend();
  }

  std::size_t size() const {
    return this->getFields()->size();
  }

  void resize(std::size_t size) {
    this->writableFields()->resize(size);
  }

  bool remove(const std::string& token) {
    return this->writableFields()->remove(token);
  }

  bool remove(std::size_t index) {
    return this->writableFields()->remove(index);
  }

  void modify(std::string token, bool construct = true) {
    modify(folly::to<std::size_t>(token), construct);
  }

  virtual void modify(std::size_t index, bool construct = true) {
    DCHECK(!this->isPublished());

    if (index < this->size()) {
      if constexpr (Fields::HasChildNodes) {
        auto& child = this->ref(index);
        if (child->isPublished()) {
          auto clonedChild = child->clone();
          child.swap(clonedChild);
        }
      }
    } else if (construct) {
      // create unpublished default constructed child if missing
      while (this->size() <= index) {
        emplace_back();
      }
    }
  }

  bool empty() const {
    return cbegin() == cend();
  }

  static void modify(std::shared_ptr<Self>* node, const std::string& token) {
    auto newNode = ((*node)->isPublished()) ? (*node)->clone() : *node;
    newNode->modify(token);
    node->swap(newNode);
  }

  const auto& impl() const {
    return this->getFields()->impl();
  }

 private:
  friend class CloneAllocator;
};

} // namespace facebook::fboss::thrift_cow
