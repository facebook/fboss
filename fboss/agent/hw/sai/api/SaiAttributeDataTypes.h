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

#include "fboss/agent/hw/sai/api/SaiApiError.h"

#include <folly/Optional.h>
#include <folly/logging/xlog.h>

#include <tuple>

#include <boost/variant.hpp>

extern "C" {
#include <sai.h>
}

namespace facebook {
namespace fboss {

/*
 * SaiAttributeTuple, SaiAttributeVariant, and SaiAttributeOptional have the
 * same interface as a normal attribute, but actually hide a tuple or variant of
 * attributes beneath the "surface". They are essentially thin, helpful wrappers
 * on top of std::tuple, boost::variant, and folly::Optional which are
 * simultaneously a model for the informal SaiAttribute "concept".
 *
 * The main advantage of this approach, is that we can now compose
 * them to create tuples with member variants, vice-versa, and more...
 *
 * The ValueType of each is the tuple or variant of the corresponding ValueTypes
 * that is:
 * SaiAttributeTuple<A, B, C>::ValueType is
 * std::tuple<A::ValueType, B::ValueType, C::ValueType>
 * and
 * SaiAttributeVariant<A, B, C>::ValueType is
 * boost::variant<A::ValueType, B::ValueType, C::ValueType>
 */
template <typename... AttrTs>
class SaiAttributeTuple {
 public:
  using ValueType = std::tuple<typename AttrTs::ValueType...>;
  static constexpr std::size_t size =
      std::tuple_size<std::tuple<AttrTs...>>::value;

  SaiAttributeTuple(const AttrTs&... ts) : tuple_(ts...) {}

  ValueType value() const {
    return valueImpl(std::make_index_sequence<size>{});
  }

  // TODO: once all *Apis move to the new model, stop calling this
  // saiAttrV and takeover saiAttr
  std::vector<sai_attribute_t> saiAttrs() const {
    return saiAttrVector();
  }

  bool operator==(const SaiAttributeTuple& other) const {
    return tuple_ == other.tuple_;
  }
  bool operator!=(const SaiAttributeTuple& other) const {
    return !(*this == other);
  }

 private:
  template <std::size_t... I>
  ValueType valueImpl(std::index_sequence<I...>) const {
    return std::make_tuple(std::get<I>(tuple_).value()...);
  }
  std::vector<sai_attribute_t> saiAttrVector() const {
    return saiAttrVectorImpl(std::make_index_sequence<size>{});
  }
  template <std::size_t... I>
  std::vector<sai_attribute_t> saiAttrVectorImpl(
      std::index_sequence<I...>) const {
    std::vector<sai_attribute_t> saiAttrs;
    std::vector<std::vector<sai_attribute_t>> saiAttrVs{
        std::get<I>(tuple_).saiAttrs()...};
    std::for_each(
        saiAttrVs.begin(),
        saiAttrVs.end(),
        [&saiAttrs](const std::vector<sai_attribute_t>& saiAttrV) {
          saiAttrs.insert(saiAttrs.end(), saiAttrV.begin(), saiAttrV.end());
        });
    return saiAttrs;
  }
  std::tuple<AttrTs...> tuple_;
};

template <typename... AttrTs>
class SaiAttributeVariant {
 public:
  using ValueType = boost::variant<typename AttrTs::ValueType...>;

  template <typename AttrT>
  SaiAttributeVariant(AttrT&& attr) : variant_(std::forward<AttrT>(attr)){};
  /*
   * SaiAttributeVariant also provides a helper, parameterized value() method
   * which takes one of the AttrTs in the variant and returns its ValueType
   * rather than a cumbersome boost::variant. If the AttrT is not one of the
   * AttrTs... then the caller code won't compile. If it is a member, but not
   * the one with which SaiAttributeVariant was constructed, then it is
   * unfortuantely a runtime error of type boost::bad_get;
   */
  template <typename AttrT>
  typename AttrT::ValueType value() const {
    return boost::get<AttrT>(variant_).value();
  }
  ValueType value() const {
    return boost::apply_visitor(valueVisitor(), variant_);
  }
  std::vector<sai_attribute_t> saiAttrs() const {
    return boost::apply_visitor(saiAttrVisitor(), variant_);
  }
  bool operator==(const SaiAttributeVariant& other) {
    return variant_ == other.variant_;
  }
  bool operator!=(const SaiAttributeVariant& other) {
    return !(*this == other);
  }
 private:
  class saiAttrVisitor
      : public boost::static_visitor<std::vector<sai_attribute_t>> {
   public:
    template <typename AttrT>
    std::vector<sai_attribute_t> operator()(const AttrT& attr) const {
      return attr.saiAttrs();
    }
    template <>
    std::vector<sai_attribute_t> operator()(
        const boost::blank& /* blank */) const {
      saiCheckError(
          SAI_STATUS_FAILURE,
          "Attempted to get saiAttr from blank SaiAttributeVariant");
      folly::assume_unreachable();
    }
  };
  class valueVisitor : public boost::static_visitor<ValueType> {
   public:
    template <typename AttrT>
    ValueType operator()(const AttrT& attr) const {
      return attr.value();
    }

    template <typename AttrT>
    ValueType operator()(const boost::blank& /* blank */) const {
      saiCheckError(
          SAI_STATUS_FAILURE,
          "Attempted to get saiAttr from blank SaiAttributeVariant");
      folly::assume_unreachable();
    }
  };
  boost::variant<AttrTs...> variant_;
};

template <typename AttrT>
class SaiAttributeOptional {
 public:
  using ValueType = folly::Optional<typename AttrT::ValueType>;
  SaiAttributeOptional() {}
  /* implicit */ SaiAttributeOptional(const folly::None&) {}
  /* implicit */ SaiAttributeOptional(const typename AttrT::ValueType& v)
      : SaiAttributeOptional(AttrT{v}) {}
  /* implicit */ SaiAttributeOptional(const ValueType& v) {
    if (v) {
      optional_ = AttrT{v.value()};
    }
  }
  /* implicit */ SaiAttributeOptional(const AttrT& attr) : optional_(attr) {}

  ValueType value() const {
    if (!optional_) {
      return folly::none;
    }
    return optional_.value().value();
  }
  std::vector<sai_attribute_t> saiAttrs() const {
    if (!optional_) {
      return std::vector<sai_attribute_t>{};
    }
    return optional_.value().saiAttrs();
  }
  bool operator==(const SaiAttributeOptional& other) const {
    return optional_ == other.optional_;
  }
  bool operator!=(const SaiAttributeOptional& other) const {
    return !(*this == other);
  }
 private:
  folly::Optional<AttrT> optional_;
};

} // namespace fboss
} // namespace facebook
