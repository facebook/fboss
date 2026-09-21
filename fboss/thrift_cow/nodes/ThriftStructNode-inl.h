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

#include <folly/Conv.h>
#include <folly/FBString.h>
#include <folly/json/dynamic.h>
#include <thrift/lib/cpp2/folly_dynamic/folly_dynamic.h>
#include <thrift/lib/cpp2/op/Get.h>
#include <thrift/lib/cpp2/protocol/Serializer.h>
#include "fboss/agent/state/NodeBase-defs.h"
#include "fboss/fsdb/if/gen-cpp2/fsdb_oper_types.h"
#include "fboss/thrift_cow/nodes/Serializer.h"
#include "fboss/thrift_cow/nodes/Types.h"
#include "fboss/thrift_cow/visitors/PathVisitor.h"

namespace facebook::fboss::thrift_cow {

namespace op = apache::thrift::op;

template <
    typename TType,
    typename Derived = ThriftStructResolver<TType, false>,
    bool EnableHybridStorage = false>
struct ThriftStructFields : public FieldBaseType {
  using Self = ThriftStructFields<TType, Derived, EnableHybridStorage>;
  using CowType = FieldsType;
  using ThriftType = TType;
  using TC = apache::thrift::type_class::structure;

  // Storage is a std::tuple indexed by ordinal
  using Storage = CowStorage<TType, Derived, EnableHybridStorage>;

  template <typename Id>
  using FieldTraits = CowFieldTraits<TType, Id, Derived, EnableHybridStorage>;

  template <typename Id>
  using TypeFor = typename FieldTraits<Id>::type;

  template <typename Id>
  using ThriftTypeFor = op::get_native_type<TType, Id>;

  template <typename Id>
  static constexpr bool IsChildNode = FieldTraits<Id>::isChild::value;

  template <typename Id>
  static constexpr bool HasSkipThriftCow = FieldTraits<Id>::allowSkipThriftCow;

  template <typename Id>
  constexpr bool isSkipThriftCowEnabled() const {
    if constexpr (EnableHybridStorage && HasSkipThriftCow<Id>) {
      return true;
    }
    return false;
  }

  // constructors:
  // One takes a thrift type directly, one default constructs everything

  ThriftStructFields() {
    op::for_each_field_id<TType>([&]<class Id>(Id) {
      using Traits = FieldTraits<Id>;
      if constexpr (!Traits::isOptional) {
        constexpr size_t pos =
            static_cast<size_t>(op::get_ordinal_v<TType, Id>) - 1;
        auto& slot = std::get<pos>(storage_);
        if constexpr (Traits::allowSkipThriftCow) {
          using UnderlyingType = typename Traits::type::element_type;
          slot = std::make_shared<UnderlyingType>();
        } else if constexpr (Traits::isChild::value) {
          using ChildType = typename Traits::type::element_type;
          slot = std::make_shared<ChildType>();
        } else {
          using FieldType = typename Traits::type::value_type;
          slot = FieldType{};
        }
      }
    });
  }

  template <typename T>
  explicit ThriftStructFields(T&& thrift)
    requires(std::is_same_v<std::decay_t<T>, TType>)
  {
    fromThrift(std::forward<T>(thrift));
  }

  TType toThrift() const {
    TType thrift;
    op::for_each_field_id<TType>([&]<class Id>(Id) {
      constexpr size_t pos =
          static_cast<size_t>(op::get_ordinal_v<TType, Id>) - 1;
      auto& stored = std::get<pos>(storage_);
      if (stored) {
        if constexpr (
            FieldTraits<Id>::isChild::value ||
            FieldTraits<Id>::allowSkipThriftCow) {
          op::get<Id>(thrift) = stored->toThrift();
        } else {
          op::get<Id>(thrift) = stored->cref();
        }
      }
    });
    return thrift;
  }

  template <typename T>
  void fromThrift(T&& thrift)
    requires(std::is_same_v<std::decay_t<T>, TType>)
  {
    op::for_each_field_id<TType>([&]<class Id>(Id) {
      using Traits = FieldTraits<Id>;
      constexpr size_t pos =
          static_cast<size_t>(op::get_ordinal_v<TType, Id>) - 1;

      if constexpr (Traits::isOptional) {
        if (op::get_value_or_null(op::get<Id>(thrift)) == nullptr) {
          std::get<pos>(storage_).reset();
          return;
        }
      }

      auto& val = *op::get<Id>(thrift);
      if constexpr (Traits::allowSkipThriftCow) {
        using UnderlyingType = typename Traits::type::element_type;
        std::get<pos>(storage_) = std::make_shared<UnderlyingType>(val);
      } else if constexpr (Traits::isChild::value) {
        using ChildType = typename Traits::type::element_type;
        std::get<pos>(storage_) = std::make_shared<ChildType>(val);
      } else {
        std::get<pos>(storage_) = val;
      }
    });
  }

#ifdef ENABLE_DYNAMIC_APIS

  folly::dynamic toDynamic() const {
    folly::dynamic out;
    facebook::thrift::to_dynamic(
        out, toThrift(), facebook::thrift::dynamic_format::JSON_1);
    return out;
  }

  void fromDynamic(const folly::dynamic& value) {
    TType thrift;
    facebook::thrift::from_dynamic(
        thrift, value, facebook::thrift::dynamic_format::JSON_1);
    fromThrift(thrift);
  }

#endif

  template <typename Id>
  TypeFor<Id> get() const {
    constexpr size_t pos =
        static_cast<size_t>(op::get_ordinal_v<TType, Id>) - 1;
    return std::get<pos>(storage_);
  }

  /*
   * References to underlying field.
   *
   * TODO: create ref() wrapper class that respects if the node is
   * published.
   */
  template <typename Id>
  TypeFor<Id>& ref() {
    constexpr size_t pos =
        static_cast<size_t>(op::get_ordinal_v<TType, Id>) - 1;
    return std::get<pos>(storage_);
  }

  template <typename Id>
  const TypeFor<Id>& ref() const {
    constexpr size_t pos =
        static_cast<size_t>(op::get_ordinal_v<TType, Id>) - 1;
    return std::get<pos>(storage_);
  }

  template <typename Id>
  const TypeFor<Id>& cref() const {
    constexpr size_t pos =
        static_cast<size_t>(op::get_ordinal_v<TType, Id>) - 1;
    return std::get<pos>(storage_);
  }

  template <typename Id>
  bool isSet() const {
    // could probably also look at optionality, but this should work.
    return bool(cref<Id>());
  }

  /*
   * Setters. If you call set on a child node, a new child will be
   * created and we replace the old one wholesale.
   */
  template <typename Id, typename TTypeFor>
  void set(TTypeFor&& value) {
    static_assert(
        std::is_convertible_v<std::decay_t<TTypeFor>, ThriftTypeFor<Id>>,
        "Unexpected thrift type for set()");

    if constexpr (HasSkipThriftCow<Id>) {
      using MemberType = typename TypeFor<Id>::element_type;
      ref<Id>() = std::make_shared<MemberType>(std::forward<TTypeFor>(value));
    } else if constexpr (IsChildNode<Id>) {
      using MemberType = typename TypeFor<Id>::element_type;
      ref<Id>() = std::make_shared<MemberType>(std::forward<TTypeFor>(value));
    } else {
      ref<Id>() = std::forward<TTypeFor>(value);
    }
  }

  template <typename Id>
  bool remove() {
    if constexpr (CowFieldTraits<TType, Id, Derived, EnableHybridStorage>::
                      isOptional) {
      auto& field = ref<Id>();
      if (field) {
        field.reset();
        return true;
      }
    } else {
      throw std::runtime_error("Cannot remove non-optional member");
    }
    return false;
  }

  bool remove(const std::string& token) {
    bool ret{false}, found{false};
    visitMember<TType>(token, [&]<class Id>(Id) {
      found = true;
      ret = this->template remove<Id>();
    });

    if (!found) {
      // should we throw here?
      throw std::runtime_error(
          folly::to<std::string>("Unable to find struct child named ", token));
    }

    return ret;
  }

  template <typename Id, typename... Args>
  TypeFor<Id>& constructMember(Args&&... args) {
    if constexpr (HasSkipThriftCow<Id>) {
      using UnderlyingType = typename TypeFor<Id>::element_type;
      return ref<Id>() =
                 std::make_shared<UnderlyingType>(std::forward<Args>(args)...);
    } else if constexpr (IsChildNode<Id>) {
      using ChildType = typename TypeFor<Id>::element_type;
      return ref<Id>() =
                 std::make_shared<ChildType>(std::forward<Args>(args)...);
    } else {
      using ChildType = typename TypeFor<Id>::value_type;
      return ref<Id>() =
                 std::make_optional<ChildType>(std::forward<Args>(args)...);
    }
  }

  template <typename Fn>
  void forEachChild(Fn fn) {
    op::for_each_field_id<TType>([&]<class Id>(Id) {
      if constexpr (
          FieldTraits<Id>::isChild::value ||
          FieldTraits<Id>::allowSkipThriftCow) {
        constexpr size_t pos =
            static_cast<size_t>(op::get_ordinal_v<TType, Id>) - 1;
        auto& child = std::get<pos>(storage_);
        if (child) {
          fn(child.get());
        }
      }
    });
  }

  template <typename Fn>
  void forEachChildName(Fn fn) {
    op::for_each_field_id<TType>([&]<class Id>(Id) {
      if constexpr (
          FieldTraits<Id>::isChild::value ||
          FieldTraits<Id>::allowSkipThriftCow) {
        constexpr size_t pos =
            static_cast<size_t>(op::get_ordinal_v<TType, Id>) - 1;
        auto& child = std::get<pos>(storage_);
        if (child) {
          fn(child.get(), Id{});
        }
      }
    });
  }

  folly::fbstring encode(fsdb::OperProtocol proto) const {
    return serialize<TC>(proto, toThrift());
  }

  folly::IOBuf encodeBuf(fsdb::OperProtocol proto) const {
    return serializeBuf<TC>(proto, toThrift());
  }

  void fromEncoded(fsdb::OperProtocol proto, const folly::fbstring& encoded) {
    fromThrift(deserialize<TC, TType>(proto, encoded));
  }

  void fromEncodedBuf(fsdb::OperProtocol proto, folly::IOBuf&& encoded) {
    fromThrift(deserializeBuf<TC, TType>(proto, std::move(encoded)));
  }

 private:
  Storage storage_;
};

template <
    typename TType,
    typename Resolver = ThriftStructResolver<TType, false>,
    bool EnableHybridStorage = false>
class ThriftStructNode : public NodeBaseT<
                             typename Resolver::type,
                             ThriftStructFields<
                                 TType,
                                 typename Resolver::type,
                                 EnableHybridStorage>>,
                         public thrift_cow::Serializable {
 public:
  using Self = ThriftStructNode<TType, Resolver, EnableHybridStorage>;
  using Derived = typename Resolver::type;
  using Fields = ThriftStructFields<TType, Derived, EnableHybridStorage>;
  using ThriftType = typename Fields::ThriftType;
  using BaseT = NodeBaseT<Derived, Fields>;
  using CowType = NodeType;
  using TC = typename Fields::TC;
  using PathIter = typename std::vector<std::string>::const_iterator;

  using BaseT::BaseT;

  ThriftStructNode() : BaseT(ThriftType{}) {}

  template <typename Id>
  constexpr bool isSkipThriftCowEnabled() const {
    return this->getFields()->template isSkipThriftCowEnabled<Id>();
  }

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

  template <typename Id>
  auto get() const {
    return this->getFields()->template get<Id>();
  }

  template <typename Id, typename Arg>
  void set(Arg&& value) {
    this->writableFields()->template set<Id>(std::forward<Arg>(value));
  }

  template <typename Id>
  bool isSet() const {
    return this->getFields()->template isSet<Id>();
  }

  template <typename Id>
  typename Fields::template TypeFor<Id>& ref() {
    return this->writableFields()->template ref<Id>();
  }

  template <typename Id>
  const typename Fields::template TypeFor<Id>& ref() const {
    return this->writableFields()->template ref<Id>();
  }

  template <typename Id>
  const typename Fields::template TypeFor<Id>& cref() const {
    return this->getFields()->template cref<Id>();
  }

  // prefer safe_ref/safe_cref for safe access
  template <typename Id>
  auto safe_ref() {
    return detail::ref(this->writableFields()->template ref<Id>());
  }

  template <typename Id>
  auto safe_cref() const {
    return detail::cref(this->getFields()->template cref<Id>());
  }

  template <typename Id, typename... Args>
  auto& constructMember(Args&&... args) {
    return this->writableFields()->template constructMember<Id>(
        std::forward<Args>(args)...);
  }

  template <typename Id>
  bool remove() {
    return this->writableFields()->template remove<Id>();
    // TODO: use SFINAE to disallow non-children names (other apis too)
  }

  bool remove(const std::string& token) override {
    return this->writableFields()->remove(token);
  }

  template <typename Id>
  auto& modify(bool construct = true) {
    DCHECK(!this->isPublished());

    auto& child = this->template ref<Id>();
    if (child) {
      if constexpr (Fields::template IsChildNode<Id>) {
        if (child->isPublished()) {
          auto clonedChild = child->clone();
          child.swap(clonedChild);
        }
      }
    } else if (construct) {
      this->template constructMember<Id>();
    }
    return this->template ref<Id>();
  }

  template <typename Id>
  static auto& modify(std::shared_ptr<Derived>* node) {
    auto newNode = ((*node)->isPublished()) ? (*node)->clone() : *node;
    auto& child = newNode->template modify<Id>();
    node->swap(newNode);
    return child;
  }

  virtual void modify(const std::string& token, bool construct = true)
      override {
    visitMember<typename Fields::ThriftType>(
        token, [&]<class Id>(Id) { this->template modify<Id>(construct); });
  }

  static void modify(std::shared_ptr<Derived>* node, std::string token) {
    auto newNode = ((*node)->isPublished()) ? (*node)->clone() : *node;
    newNode->modify(token);
    node->swap(newNode);
  }

  /*
   * Visitors by string path
   */
  static ThriftTraverseResult
  modifyPath(std::shared_ptr<Derived>* root, PathIter begin, PathIter end) {
    // first clone root if needed
    auto newRoot = ((*root)->isPublished()) ? (*root)->clone() : *root;

    ThriftTraverseResult result;
    if (begin != end) {
      auto op = BasePathVisitorOperator(
          [](Serializable& node, PathIter begin, PathIter end) {
            if (begin == end) {
              return;
            }
            auto tok = *begin;
            node.modify(tok);
          });

      result = PathVisitor<TC>::visit(
          *newRoot, begin, end, PathVisitOptions::visitFull(), op);
    }

    // if successful and changed, reset root
    if (result && newRoot.get() != root->get()) {
      (*root).swap(newRoot);
    }
    return result;
  }

  static ThriftTraverseResult
  removePath(std::shared_ptr<Derived>* root, PathIter begin, PathIter end) {
    if (begin == end) {
      return ThriftTraverseResult();
    }

    // first clone root if needed
    auto newRoot = ((*root)->isPublished()) ? (*root)->clone() : *root;

    auto op = BasePathVisitorOperator(
        [](Serializable& node, PathIter begin, PathIter end) {
          auto tok = *begin;
          if (begin == end) {
            node.remove(tok);

          } else {
            node.modify(tok, false);
          }
        });

    // Traverse to second to last hop and call remove. Modify parents
    // along the way
    auto result = PathVisitor<TC>::visit(
        *newRoot, begin, end - 1, PathVisitOptions::visitFull(), op);

    // if successful, reset root
    if (result) {
      (*root).swap(newRoot);
    }
    return result;
  }

  bool operator==(const Self& that) const {
    return this->toThrift() == that.toThrift();
  }

  bool operator!=(const Self& that) const {
    return !(*this == that);
  }

 private:
  friend class CloneAllocator;
};

} // namespace facebook::fboss::thrift_cow
