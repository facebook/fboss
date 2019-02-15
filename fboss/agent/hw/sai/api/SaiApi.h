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

#include "SaiAttribute.h"
#include "fboss/agent/hw/sai/api/SaiApiError.h"
#include "fboss/agent/hw/sai/api/Traits.h"

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

  /*
   * Because there are objects with attributes which MUST be set on create,
   * we must be able to take a list of attributes for create. This means
   * that we have to wrap individual attributes in a type that can live
   * in a container. We use boost::variant for this. The visitor copies
   * out the POD sai_attribute_t from the SaiAttribute into the array
   * of sai_attribute_t pointers passed to SAI. To avoid this copying, if
   * needed, it is probably best to use the underlying SAI API directly.
   */
  template <typename T = ApiParameters, typename... Args>
  typename std::enable_if<apiUsesObjectId<T>::value, sai_object_id_t>::type
  create(
      const std::vector<typename T::AttributeType>& attributes,
      Args&&... args) {
    static_assert(
        std::is_same<T, ApiParameters>::value,
        "AttributeType must come from correct ApiParameters");
    sai_object_id_t id;
    std::vector<sai_attribute_t> saiAttributeTs = getSaiAttributeTs(attributes);
    sai_status_t status = impl()._create(
        &id,
        saiAttributeTs.data(),
        saiAttributeTs.size(),
        std::forward<Args>(args)...);
    saiCheckError(status, "Failed to create sai entity");
    return id;
  }

  template <typename T = ApiParameters, typename... Args>
  typename std::enable_if<apiUsesEntry<T>::value, void>::type create(
      const typename T::EntryType& entry,
      const std::vector<typename T::AttributeType>& attributes,
      Args&&... args) {
    static_assert(
        std::is_same<T, ApiParameters>::value,
        "AttributeType or EntryTypes must come from correct ApiParameters");
    std::vector<sai_attribute_t> saiAttributeTs = getSaiAttributeTs(attributes);
    sai_status_t status = impl()._create(
        entry,
        saiAttributeTs.data(),
        saiAttributeTs.size(),
        std::forward<Args>(args)...);
    saiCheckError(status, "Failed to create sai entity");
  }

  // Note: we need to parameterize by T = ApiParameters, then immediately
  // statically check that T = ApiParameters in order to enable type
  // deduction to allow the use of SFINAE to select this method rather
  // than the version that expects an EntryType and has no return value
  template <typename T = ApiParameters, typename... Args>
  sai_object_id_t create2(
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
    saiCheckError(status, "Failed to create2 sai entity");
    return id;
  }

  template <typename T>
  void remove(const T& t) {
    sai_status_t status = impl()._remove(t);
    saiCheckError(status, "Failed to remove sai entity");
  }

  template <typename AttrT, typename... Args>
  typename std::remove_reference<AttrT>::type::ValueType getAttribute(
      AttrT&& attr,
      Args&&... args) {
    sai_status_t status =
        impl()._getAttr(attr.saiAttr(), std::forward<Args>(args)...);
    saiCheckError(status, "Failed to get sai attribute");
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
    saiCheckError(status, "Failed to get sai member attribute");
    return attr.value();
  }

  template <typename AttrT, typename... Args>
  void setMemberAttribute(const AttrT& attr, Args&&... args) {
    sai_status_t status =
        impl()._setMemberAttr(attr.saiAttr(), std::forward<Args>(args)...);
    saiCheckError(status, "Failed to set sai attribute");
  }

  template <typename T = ApiParameters, typename... Args>
  typename std::enable_if<apiHasMembers<T>::value, sai_object_id_t>::type
  createMember(
      const std::vector<typename T::MemberAttributeType>& attributes,
      Args&&... args) {
    std::vector<sai_attribute_t> saiAttributeTs = getSaiAttributeTs(attributes);
    sai_object_id_t id;
    sai_status_t status = impl()._createMember(
        &id,
        saiAttributeTs.data(),
        saiAttributeTs.size(),
        std::forward<Args>(args)...);
    saiCheckError(status, "Failed to create sai member entity");
    return id;
  }

  template <typename T = ApiParameters, typename... Args>
  typename std::enable_if<apiHasMembers<T>::value, sai_object_id_t>::type
  createMember2(
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
    saiCheckError(status, "Failed to create2 sai entity member");
    return id;
  }

  void removeMember(sai_object_id_t id) {
    sai_status_t status = impl()._removeMember(id);
    saiCheckError(status, "Failed to remove sai member entity");
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
