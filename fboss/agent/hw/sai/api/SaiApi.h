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
#include "fboss/agent/hw/sai/api/SaiAttribute.h"
#include "fboss/agent/hw/sai/api/Traits.h"
#include "fboss/lib/TupleUtils.h"

#include <folly/Format.h>
#include <folly/logging/xlog.h>

#include <boost/variant.hpp>

#include <exception>
#include <stdexcept>
#include <type_traits>
#include <vector>

extern "C" {
#include <sai.h>
}

namespace facebook {
namespace fboss {

template <typename ApiT, typename ApiParameters>
class SaiApi {
 public:
  virtual ~SaiApi() = default;
  SaiApi() = default;
  SaiApi(const SaiApi& other) = delete;
  SaiApi& operator=(const SaiApi& other) = delete;

  // Note: we need to parameterize by T = ApiParameters, then immediately
  // statically check that T = ApiParameters in order to enable type
  // deduction to allow the use of SFINAE to select this method rather
  // than the version that expects an EntryType and has no return value
  template <typename T = ApiParameters, typename... Args>
  typename std::enable_if<apiUsesObjectId<T>::value, sai_object_id_t>::type
  create(
      const typename T::Attributes::CreateAttributes& createAttrs,
      Args&&... args) {
    static_assert(
        std::is_same<T, ApiParameters>::value,
        "Attributes must come from correct ApiParameters");
    sai_object_id_t id;
    std::vector<sai_attribute_t> saiAttributeTs = createAttrs.saiAttrs();
    sai_status_t status = impl()._create(
        &id,
        saiAttributeTs.data(),
        saiAttributeTs.size(),
        std::forward<Args>(args)...);
    saiApiCheckError(status, T::ApiType, "Failed to create sai entity");
    return id;
  }

  template <typename T = ApiParameters, typename... Args>
  typename std::enable_if<apiUsesEntry<T>::value, void>::type create(
      const typename T::EntryType& entry,
      const typename T::Attributes::CreateAttributes& createAttrs,
      Args&&... args) {
    static_assert(
        std::is_same<T, ApiParameters>::value,
        "Attributes and EntryType must come from correct ApiParameters");
    std::vector<sai_attribute_t> saiAttributeTs = createAttrs.saiAttrs();
    sai_status_t status = impl()._create(
        entry,
        saiAttributeTs.data(),
        saiAttributeTs.size(),
        std::forward<Args>(args)...);
    saiApiCheckError(status, T::ApiType, "Failed to create sai entity");
  }

  template <typename T>
  void remove(const T& t) {
    sai_status_t status = impl()._remove(t);
    saiApiCheckError(
        status, ApiParameters::ApiType, "Failed to remove sai entity");
  }

  template <typename AttrT, typename... Args>
  typename std::remove_reference<AttrT>::type::ValueType getAttribute(
      AttrT&& attr,
      Args&&... args) {
    sai_status_t status =
        impl()._getAttr(attr.saiAttr(), std::forward<Args>(args)...);
    /*
     * If this is a list attribute and we have not allocated enough
     * memory for the data coming from SAI, the Adapter will return
     * SAI_STATUS_BUFFER_OVERFLOW and fill in `count` in the list object.
     * We can take advantage of that to allocate a proper buffer and
     * try the get again.
     */
    if (status == SAI_STATUS_BUFFER_OVERFLOW) {
      attr.realloc();
      status = impl()._getAttr(attr.saiAttr(), std::forward<Args>(args)...);
    }
    saiApiCheckError(
        status, ApiParameters::ApiType, "Failed to get sai attribute");
    return attr.value();
  }

  template <typename AttrT, typename... Args>
  sai_status_t setAttribute(const AttrT& attr, Args&&... args) {
    return impl()._setAttr(attr.saiAttr(), std::forward<Args>(args)...);
  }

  template <typename AttrT, typename... Args>
  typename std::remove_reference<AttrT>::type::ValueType getMemberAttribute(
      AttrT&& attr,
      Args&&... args) {
    sai_status_t status =
        impl()._getMemberAttr(attr.saiAttr(), std::forward<Args>(args)...);
    saiApiCheckError(
        status, ApiParameters::ApiType, "Failed to get sai member attribute");
    return attr.value();
  }

  template <typename AttrT, typename... Args>
  void setMemberAttribute(const AttrT& attr, Args&&... args) {
    sai_status_t status =
        impl()._setMemberAttr(attr.saiAttr(), std::forward<Args>(args)...);
    saiApiCheckError(
        status, ApiParameters::ApiType, "Failed to set sai attribute");
  }

  template <typename T = ApiParameters, typename... Args>
  typename std::enable_if<apiHasMembers<T>::value, sai_object_id_t>::type
  createMember(
      const typename T::MemberAttributes::CreateAttributes& createAttrs,
      Args&&... args) {
    static_assert(
        std::is_same<T, ApiParameters>::value,
        "MemberAttributes must come from correct ApiParameters");
    sai_object_id_t id;
    std::vector<sai_attribute_t> saiAttributeTs = createAttrs.saiAttrs();
    sai_status_t status = impl()._createMember(
        &id,
        saiAttributeTs.data(),
        saiAttributeTs.size(),
        std::forward<Args>(args)...);
    saiApiCheckError(status, T::ApiType, "Failed to create sai entity member");
    return id;
  }

  void removeMember(sai_object_id_t id) {
    sai_status_t status = impl()._removeMember(id);
    saiApiCheckError(
        status, ApiParameters::ApiType, "Failed to remove sai member entity");
  }

  /*
   * NOTE:
   * The following APIs constitute a re-write of the above APIs and are renamed
   * with a '2' at the end to indicate that. Once the callsites are all ported
   * to the new APIs, the old APIs and the '2' can be deleted.
   *
   * TODO(borisb): clean this up:
   * delete ApiParameters class from CRTP base class
   * delete old APIs
   * delete SaiAttributeDataTypes
   * rename these to no longer have the '2'
   */

  /*
   * We can do getAttribute on top of more complicated types than just
   * attributes. For example, if we overload on tuples and optionals, we
   * can recursively call getAttribute on their internals to do more
   * interesting gets. This is particularly useful when we want to load
   * complicated aggregations of SaiAttributes for something like warm boot.
   */

  // Default "real attr". This is the base case of the recursion
  template <typename AdapterKeyT, typename AttrT>
  typename std::remove_reference<AttrT>::type::ValueType getAttribute2(
      const AdapterKeyT& key,
      AttrT&& attr) {
    static_assert(
        IsSaiAttribute<typename std::remove_reference<AttrT>::type>::value,
        "getAttribute must be called on a SaiAttribute or supported "
        "collection of SaiAttributes");
    sai_status_t status;
    status = impl()._getAttribute(key, attr.saiAttr());
    /*
     * If this is a list attribute and we have not allocated enough
     * memory for the data coming from SAI, the Adapter will return
     * SAI_STATUS_BUFFER_OVERFLOW and fill in `count` in the list object.
     * We can take advantage of that to allocate a proper buffer and
     * try the get again.
     */
    if (status == SAI_STATUS_BUFFER_OVERFLOW) {
      attr.realloc();
      status = impl()._getAttribute(key, attr.saiAttr());
    }
    saiApiCheckError(
        status, ApiParameters::ApiType, "Failed to get sai attribute");
    return attr.value();
  }

  // std::tuple of attributes
  template <typename AdapterKeyT, typename... AttrTs>
  auto getAttribute2(const AdapterKeyT& key, std::tuple<AttrTs...>& attrTuple) {
    // TODO: assert on All<IsSaiAttribute>
    auto recurse = [&key, this](auto&& attr) {
      return getAttribute2(key, attr);
    };
    return tupleMap(recurse, attrTuple);
  }

  // std::optional of attribute
  template <typename AdapterKeyT, typename AttrT>
  auto getAttribute2(
      const AdapterKeyT& key,
      std::optional<AttrT>& attrOptional) {
    AttrT attr = attrOptional.value_or(AttrT{});
    return getAttribute2(key, attr);
  }

  // Currently, create2 is not clever enough to have totally deducible
  // template parameters. It can be done, but I think it would reduce
  // the value of the CreateAttributes pattern. That is something that
  // may change in the future.
  //
  // It is also split up into two implementations:
  // one for objects whose AdapterKey is a SAI object id, which needs to
  // return the adapter key, and another for those objects whose AdapterKey
  // is an entry struct, which must take an AdapterKey but don't return one.
  // The distinction is drawn with traits from Traits.h and SFINAE

  // sai_object_id_t case
  template <typename SaiObjectTraits>
  std::enable_if_t<
      AdapterKeyIsObjectId<SaiObjectTraits>::value,
      typename SaiObjectTraits::AdapterKey>
  create2(
      const typename SaiObjectTraits::CreateAttributes& createAttributes,
      sai_object_id_t switch_id) {
    sai_object_id_t id;
    std::vector<sai_attribute_t> saiAttributeTs = saiAttrs(createAttributes);
    sai_status_t status = impl().template _create<SaiObjectTraits>(
        &id, switch_id, saiAttributeTs.size(), saiAttributeTs.data());
    saiApiCheckError(
        status, ApiParameters::ApiType, "Failed to create sai entity");
    return typename SaiObjectTraits::AdapterKey{id};
  }

  // entry struct case
  template <typename SaiObjectTraits>
  std::enable_if_t<AdapterKeyIsEntryStruct<SaiObjectTraits>::value, void>
  create2(
      const typename SaiObjectTraits::AdapterKey& entry,
      const typename SaiObjectTraits::CreateAttributes& createAttributes) {
    std::vector<sai_attribute_t> saiAttributeTs = saiAttrs(createAttributes);
    sai_status_t status = impl().template _create<SaiObjectTraits>(
        entry, saiAttributeTs.size(), saiAttributeTs.data());
    saiApiCheckError(
        status, ApiParameters::ApiType, "Failed to create sai entity");
  }

  template <typename AdapterKeyT>
  void remove2(const AdapterKeyT& key) {
    sai_status_t status = impl()._remove(key);
    saiApiCheckError(
        status, ApiParameters::ApiType, "Failed to remove sai object");
  }

  template <typename AdapterKeyT, typename AttrT>
  sai_status_t setAttribute2(const AdapterKeyT& key, const AttrT& attr) {
    return impl()._setAttribute(key, attr.saiAttr());
  }

 private:
  // boost visitor that takes a SaiAttribute and returns a pointer to the
  // underlying sai_attribute_t.
  class getSaiAttributeVisitor
      : public boost::static_visitor<const sai_attribute_t*> {
   public:
    template <typename AttrT>
    const sai_attribute_t* operator()(const AttrT& attr) const {
      return attr.saiAttr();
    }
    const sai_attribute_t* operator()(const boost::blank& blank) const {
      return nullptr;
    }
  };

  // Given a vector of SaiAttribute boost variant wrapper objects
  // (ApiParameters::AttributeType or ApiParameters::MemberAttributeType),
  // return a vector of the underlying sai_attribute_t structs. Helper function
  // for various methods of SaiApi that take multiple attributes, particularly
  // create()
  // TODO(borisb): In the future, we can create an AttributeList class that
  // provides this behavior more directly and stores the sai_attribute_t structs
  // inline to prevent unnecessary copying
  template <typename AttrT>
  std::vector<sai_attribute_t> getSaiAttributeTs(
      const std::vector<AttrT>& attributes) const {
    static_assert(
        std::is_same<AttrT, typename ApiParameters::AttributeType>::value ||
            std::is_same<AttrT, typename ApiParameters::MemberAttributeType>::
                value,
        "Type we get sai_attribute_t from must be a SaiAttribute in the API");
    std::vector<sai_attribute_t> saiAttributeTs;
    for (const auto& attribute : attributes) {
      saiAttributeTs.push_back(
          *(boost::apply_visitor(getSaiAttributeVisitor(), attribute)));
    }
    return saiAttributeTs;
  }

  ApiT& impl() {
    return static_cast<ApiT&>(*this);
  }
};

} // namespace fboss
} // namespace facebook
