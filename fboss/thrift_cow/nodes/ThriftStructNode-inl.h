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
#include <folly/Conv.h>
#include <folly/FBString.h>
#include <folly/json/dynamic.h>
#include <thrift/lib/cpp2/folly_dynamic/folly_dynamic.h>
#include <thrift/lib/cpp2/protocol/Serializer.h>
#include <thrift/lib/cpp2/reflection/reflection.h>
#include "fboss/agent/state/NodeBase-defs.h"
#include "fboss/fsdb/if/gen-cpp2/fsdb_oper_types.h"
#include "fboss/thrift_cow/nodes/Serializer.h"
#include "fboss/thrift_cow/nodes/Types.h"
#include "fboss/thrift_cow/visitors/PathVisitor.h"

namespace facebook::fboss::thrift_cow {

namespace struct_helpers {

// helper to create a type compatible w/ fatal::transform that can
// extract the 'ttype' member type variable of another type
FATAL_GET_MEMBER_TYPE(ttype);

template <typename Member>
constexpr bool isOptional() {
  return Member::optional::value == apache::thrift::optionality::optional;
}

struct HasSkipThriftCow {
  template <typename Traits>
  using apply = typename std::conditional<
      Traits::allowSkipThriftCow,
      std::true_type,
      std::false_type>::type;
};

struct IsChildNode {
  template <typename Traits>
  using apply = typename Traits::isChild;
};

// This is templated on the ThriftStructFields type and is used in
// fatal::foreach to copy from a thrift struct to our underlying
// storage type (NamedMemberTypes)
template <typename FieldsT>
struct CopyFromMember {
  using NamedMemberTypes = typename FieldsT::NamedMemberTypes;
  using TType = typename FieldsT::ThriftType;

  template <typename T>
  void
  operator()(fatal::tag<T>, TType& thrift, const NamedMemberTypes& storage) {
    using name = typename T::name;
    using member = typename T::member;

    auto& stored = storage.template get<name>();
    if (stored) {
      typename member::field_ref_getter{}(thrift) = stored->toThrift();
    }
  }
};

// This is templated on the ThriftStructFields type and is used in
// fatal::foreach to copy from our underlying storage type
// (NamedMemberTypes) to a raw thrift struct
template <typename FieldsT>
struct MemberConstruct {
  using NamedMemberTypes = typename FieldsT::NamedMemberTypes;

  template <typename T>
  void operator()(fatal::tag<T>, NamedMemberTypes& storage) {
    using name = typename T::name;
    if constexpr (!isOptional<typename T::member>()) {
      if constexpr (FieldsT::template HasSkipThriftCow<name>::value) {
        using UnderlyingType =
            typename FieldsT::template TypeFor<name>::element_type;
        storage.template get<name>() = std::make_shared<UnderlyingType>();
      } else if constexpr (FieldsT::template IsChildNode<name>::value) {
        using ChildType =
            typename FieldsT::template TypeFor<name>::element_type;
        storage.template get<name>() = std::make_shared<ChildType>();
      } else {
        using FieldType = typename FieldsT::template TypeFor<name>::value_type;
        storage.template get<name>() = FieldType{};
      }
    }
  }
};

// Copy thrift data to a member. If a child, we will create a fully
// new child node and replace the existing one (if present).
template <typename FieldsT>
struct CopyToMember {
  using NamedMemberTypes = typename FieldsT::NamedMemberTypes;
  using TType = typename FieldsT::ThriftType;

  template <typename T>
  void
  operator()(fatal::tag<T>, NamedMemberTypes& storage, const TType& thrift) {
    using name = typename T::name;
    using member = typename T::member;

    if (!member::is_set(thrift) && isOptional<member>()) {
      storage.template get<name>().reset();
      return;
    }

    if constexpr (FieldsT::template HasSkipThriftCow<name>::value) {
      using UnderlyingType =
          typename FieldsT::template TypeFor<name>::element_type;
      storage.template get<name>() =
          std::make_shared<UnderlyingType>(typename member::getter{}(thrift));
    } else if constexpr (FieldsT::template IsChildNode<name>::value) {
      using ChildType = typename FieldsT::template TypeFor<name>::element_type;
      storage.template get<name>() =
          std::make_shared<ChildType>(typename member::getter{}(thrift));
    } else {
      storage.template get<name>() =
          typename member::field_ref_getter{}(thrift).value();
    }
  }
};

// Invoke a function on each child. Expects functions that take a raw
// pointer to a node.
template <typename FieldsT, bool withName = false>
struct ChildInvoke {
  using NamedMemberTypes = typename FieldsT::NamedMemberTypes;

  template <typename T, typename Fn>
  void operator()(fatal::tag<T>, NamedMemberTypes& storage, Fn fn) {
    using ChildType =
        typename NamedMemberTypes::template type_of<typename T::name>;
    ChildType value = storage.template get<typename T::name>();
    if (value) {
      if constexpr (!withName) {
        fn(value.get());
      } else {
        fn(value.get(), typename T::name());
      }
    }
  }
};

} // namespace struct_helpers

template <typename TType, typename Derived = ThriftStructResolver<TType>>
struct ThriftStructFields {
  using Self = ThriftStructFields<TType, Derived>;
  using Info = apache::thrift::reflect_struct<TType>;
  using CowType = FieldsType;
  using ThriftType = TType;
  using TC = apache::thrift::type_class::structure;

  // type list of reflected_struct_member
  using Members = typename Info::members;

  // Extracting useful common types out of each member via Traits.h
  using MemberTypes = fatal::transform<Members, ExtractStructFields<Derived>>;

  // type list of members with SkipThriftCow enabled
  using MemberTypesWithSkipThriftCow =
      fatal::filter<MemberTypes, struct_helpers::HasSkipThriftCow>;

  template <typename Name>
  using HasSkipThriftCow = typename fatal::
      contains<MemberTypesWithSkipThriftCow, Name, fatal::get_type::name>;

  template <typename Name>
  constexpr bool isSkipThriftCowEnabled() const {
    if constexpr (HasSkipThriftCow<Name>::value) {
      return true;
    } else {
      return false;
    }
  }

  // This is our ultimate storage type, which is effectively a
  // std::tuple with syntactic sugar for accessing based on
  // these constexpr strings.
  using NamedMemberTypes = typename fatal::tuple_from<MemberTypes>::
      template list<fatal::get_member_type::type, fatal::get_member_type::name>;
  using ChildrenTypes = fatal::filter<MemberTypes, struct_helpers::IsChildNode>;

  template <typename Name>
  using TypeFor = typename fatal::find<
      MemberTypes,
      Name,
      std::false_type,
      fatal::get_type::name,
      fatal::get_type::type>;

  template <typename Name>
  using MemberFor = typename fatal::find<
      Members,
      Name,
      std::false_type,
      fatal::get_type::name,
      fatal::get_identity>;

  template <typename Name>
  using ThriftTypeFor = typename MemberFor<Name>::type;

  template <typename Name>
  using IsChildNode =
      typename fatal::contains<ChildrenTypes, Name, fatal::get_type::name>;

  // constructors:
  // One takes a thrift type directly, one default constructs everything

  ThriftStructFields() {
    fatal::foreach<MemberTypes>(
        struct_helpers::MemberConstruct<Self>(), storage_);
  }

  template <typename T>
  explicit ThriftStructFields(T&& thrift)
    requires(std::is_same_v<std::decay_t<T>, TType>)
  {
    fromThrift(std::forward<T>(thrift));
  }

  TType toThrift() const {
    TType thrift;
    fatal::foreach<MemberTypes>(
        struct_helpers::CopyFromMember<Self>(), thrift, storage_);
    return thrift;
  }

  template <typename T>
  void fromThrift(T&& thrift)
    requires(std::is_same_v<std::decay_t<T>, TType>)
  {
    fatal::foreach<MemberTypes>(
        struct_helpers::CopyToMember<Self>(),
        storage_,
        std::forward<T>(thrift));
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

  template <typename Name>
  TypeFor<Name> get() const {
    return storage_.template get<Name>();
  }

  /*
   * References to underlying field.
   *
   * TODO: create ref() wrapper class that respects if the node is
   * published.
   */
  template <typename Name>
  TypeFor<Name>& ref() {
    return storage_.template get<Name>();
  }

  template <typename Name>
  const TypeFor<Name>& ref() const {
    return storage_.template get<Name>();
  }

  template <typename Name>
  const TypeFor<Name>& cref() const {
    return storage_.template get<Name>();
  }

  template <typename Name>
  bool isSet() const {
    // could probably also look at optionality, but this should work.
    return bool(cref<Name>());
  }

  /*
   * Setters. If you call set on a child node, a new child will be
   * created and we replace the old one wholesale.
   */
  template <typename Name, typename TTypeFor>
  void set(TTypeFor&& value) {
    static_assert(
        std::is_convertible_v<std::decay_t<TTypeFor>, ThriftTypeFor<Name>>,
        "Unexpected thrift type for set()");

    if constexpr (HasSkipThriftCow<Name>::value) {
      using MemberType = typename TypeFor<Name>::element_type;
      ref<Name>() = std::make_shared<MemberType>(std::forward<TTypeFor>(value));
    } else if constexpr (IsChildNode<Name>::value) {
      using MemberType = typename TypeFor<Name>::element_type;
      ref<Name>() = std::make_shared<MemberType>(std::forward<TTypeFor>(value));
    } else {
      ref<Name>() = std::forward<TTypeFor>(value);
    }
  }

  template <typename Name>
  bool remove() {
    // TODO: use SFINAE to disallow non-children names

    bool ret{false};

    fatal::foreach<Members>([&](auto tag) {
      using member = decltype(fatal::tag_type(tag));
      if constexpr (std::is_same_v<typename member::name, Name>) {
        ret = this->remove_impl<member>();
      }
    });

    return ret;
  }

  bool remove(const std::string& token) {
    bool ret{false}, found{false};
    visitMember<Members>(token, [&](auto tag) {
      using member = decltype(fatal::tag_type(tag));
      found = true;
      ret = this->remove_impl<member>();
    });

    if (!found) {
      // should we throw here?
      throw std::runtime_error(
          folly::to<std::string>("Unable to find struct child named ", token));
    }

    return ret;
  }

  template <typename Name, typename... Args>
  TypeFor<Name>& constructMember(Args&&... args) {
    if constexpr (HasSkipThriftCow<Name>::value) {
      using UnderlyingType = typename TypeFor<Name>::element_type;
      return ref<Name>() =
                 std::make_shared<UnderlyingType>(std::forward<Args>(args)...);
    } else if constexpr (IsChildNode<Name>::value) {
      using ChildType = typename TypeFor<Name>::element_type;
      return ref<Name>() =
                 std::make_shared<ChildType>(std::forward<Args>(args)...);
    } else {
      using ChildType = typename TypeFor<Name>::value_type;
      return ref<Name>() =
                 std::make_optional<ChildType>(std::forward<Args>(args)...);
    }
  }

  template <typename Fn>
  void forEachChild(Fn fn) {
    fatal::foreach<ChildrenTypes>(
        struct_helpers::ChildInvoke<Self>(), storage_, std::forward<Fn>(fn));
  }

  template <typename Fn>
  void forEachChildName(Fn fn) {
    fatal::foreach<ChildrenTypes>(
        struct_helpers::ChildInvoke<Self, true>(),
        storage_,
        std::forward<Fn>(fn));
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
  template <typename Member>
  bool remove_impl() {
    if constexpr (struct_helpers::isOptional<Member>()) {
      auto& field = ref<typename Member::name>();
      if (field) {
        field.reset();
        return true;
      }
    } else {
      throw std::runtime_error("Cannot remove non-optional member");
    }
    return false;
  }

  NamedMemberTypes storage_;
};

template <typename TType, typename Resolver = ThriftStructResolver<TType>>
class ThriftStructNode
    : public NodeBaseT<
          typename Resolver::type,
          ThriftStructFields<TType, typename Resolver::type>>,
      public thrift_cow::Serializable {
 public:
  using Self = ThriftStructNode<TType, Resolver>;
  using Derived = typename Resolver::type;
  using Fields = ThriftStructFields<TType, Derived>;
  using ThriftType = typename Fields::ThriftType;
  using BaseT = NodeBaseT<Derived, Fields>;
  using CowType = NodeType;
  using TC = typename Fields::TC;
  using PathIter = typename std::vector<std::string>::const_iterator;

  using BaseT::BaseT;

  ThriftStructNode() : BaseT(ThriftType{}) {}

  template <typename Name>
  constexpr bool isSkipThriftCowEnabled() const {
    return this->getFields()->template isSkipThriftCowEnabled<Name>();
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

  template <typename Name>
  auto get() const {
    return this->getFields()->template get<Name>();
  }

  template <typename Name, typename Arg>
  void set(Arg&& value) {
    this->writableFields()->template set<Name>(std::forward<Arg>(value));
  }

  template <typename Name>
  bool isSet() const {
    return this->getFields()->template isSet<Name>();
  }

  template <typename Name>
  typename Fields::template TypeFor<Name>& ref() {
    return this->writableFields()->template ref<Name>();
  }

  template <typename Name>
  const typename Fields::template TypeFor<Name>& ref() const {
    return this->writableFields()->template ref<Name>();
  }

  template <typename Name>
  const typename Fields::template TypeFor<Name>& cref() const {
    return this->getFields()->template cref<Name>();
  }

  // prefer safe_ref/safe_cref for safe access
  template <typename Name>
  auto safe_ref() {
    return detail::ref(this->writableFields()->template ref<Name>());
  }

  template <typename Name>
  auto safe_cref() const {
    return detail::cref(this->getFields()->template cref<Name>());
  }

  template <typename Name, typename... Args>
  auto& constructMember(Args&&... args) {
    return this->writableFields()->template constructMember<Name>(
        std::forward<Args>(args)...);
  }

  template <typename Name>
  bool remove() {
    return this->writableFields()->template remove<Name>();
    // TODO: use SFINAE to disallow non-children names (other apis too)
  }

  bool remove(const std::string& token) {
    return this->writableFields()->remove(token);
  }

  template <typename Name>
  auto& modify(bool construct = true) {
    DCHECK(!this->isPublished());

    auto& child = this->template ref<Name>();
    if (child) {
      if constexpr (Fields::template IsChildNode<Name>::value) {
        if (child->isPublished()) {
          auto clonedChild = child->clone();
          child.swap(clonedChild);
        }
      }
    } else if (construct) {
      this->template constructMember<Name>();
    }
    return this->template ref<Name>();
  }

  template <typename Name>
  static auto& modify(std::shared_ptr<Derived>* node) {
    auto newNode = ((*node)->isPublished()) ? (*node)->clone() : *node;
    auto& child = newNode->template modify<Name>();
    node->swap(newNode);
    return child;
  }

  virtual void modify(const std::string& token, bool construct = true) {
    visitMember<typename Fields::Members>(token, [&](auto tag) {
      using name = typename decltype(fatal::tag_type(tag))::name;
      this->modify<name>(construct);
    });
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

    auto result = ThriftTraverseResult::OK;
    if (begin != end) {
      // TODO: can probably remove lambda use here
      auto op = pvlambda([](auto&& node, auto begin, auto end) {
        if (begin == end) {
          return;
        }
        auto tok = *begin;
        node.modify(tok);
      });

      result =
          PathVisitor<TC>::visit(*newRoot, begin, end, PathVisitMode::FULL, op);
    }

    // if successful and changed, reset root
    if (result == ThriftTraverseResult::OK && newRoot.get() != root->get()) {
      (*root).swap(newRoot);
    }
    return result;
  }

  static ThriftTraverseResult
  removePath(std::shared_ptr<Derived>* root, PathIter begin, PathIter end) {
    if (begin == end) {
      return ThriftTraverseResult::OK;
    }

    // first clone root if needed
    auto newRoot = ((*root)->isPublished()) ? (*root)->clone() : *root;

    // TODO: can probably remove lambda use here
    auto op = pvlambda([](auto&& node, auto begin, auto end) {
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
        *newRoot, begin, end - 1, PathVisitMode::FULL, op);

    // if successful, reset root
    if (result == ThriftTraverseResult::OK) {
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
