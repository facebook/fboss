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

#include "fboss/agent/hw/sai/api/LoggingUtil.h"
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

namespace facebook::fboss {

template <typename ApiT>
class SaiApi {
 public:
  virtual ~SaiApi() = default;
  SaiApi() = default;
  SaiApi(const SaiApi& other) = delete;
  SaiApi& operator=(const SaiApi& other) = delete;

  /*
   * NOTE:
   * The following APIs constitute a re-write of the above APIs and are renamed
   * with a '2' at the end to indicate that. Once the callsites are all ported
   * to the new APIs, the old APIs and the '2' can be deleted.
   *
   * TODO(borisb): clean this up:
   * delete ApiT class from CRTP base class
   * delete old APIs
   * delete SaiAttributeDataTypes
   * rename these to no longer have the '2'
   */

  // Currently, create is not clever enough to have totally deducible
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
  create(
      const typename SaiObjectTraits::CreateAttributes& createAttributes,
      sai_object_id_t switch_id) {
    static_assert(
        std::is_same_v<typename SaiObjectTraits::SaiApiT, ApiT>,
        "invalid traits for the api");
    typename SaiObjectTraits::AdapterKey key;
    std::vector<sai_attribute_t> saiAttributeTs = saiAttrs(createAttributes);
    sai_status_t status = impl()._create(
        &key, switch_id, saiAttributeTs.size(), saiAttributeTs.data());
    saiApiCheckError(status, ApiT::ApiType, "Failed to create sai entity");
    return key;
  }

  // entry struct case
  template <typename SaiObjectTraits>
  std::enable_if_t<AdapterKeyIsEntryStruct<SaiObjectTraits>::value, void>
  create(
      const typename SaiObjectTraits::AdapterKey& entry,
      const typename SaiObjectTraits::CreateAttributes& createAttributes) {
    static_assert(
        std::is_same_v<typename SaiObjectTraits::SaiApiT, ApiT>,
        "invalid traits for the api");
    std::vector<sai_attribute_t> saiAttributeTs = saiAttrs(createAttributes);
    sai_status_t status =
        impl()._create(entry, saiAttributeTs.size(), saiAttributeTs.data());
    saiApiCheckError(status, ApiT::ApiType, "Failed to create sai entity");
    XLOG(DBG5) << "created sai object [" << saiApiTypeToString(ApiT::ApiType)
               << "]:" << folly::logging::objectToString(entry);
  }

  template <typename AdapterKeyT>
  void remove(const AdapterKeyT& key) {
    sai_status_t status = impl()._remove(key);
    saiApiCheckError(status, ApiT::ApiType, "Failed to remove sai object");
    XLOG(DBG5) << "removed sai object [" << saiApiTypeToString(ApiT::ApiType)
               << "]:" << folly::logging::objectToString(key);
  }

  /*
   * We can do getAttribute on top of more complicated types than just
   * attributes. For example, if we overload on tuples and optionals, we
   * can recursively call getAttribute on their internals to do more
   * interesting gets. This is particularly useful when we want to load
   * complicated aggregations of SaiAttributes for something like warm boot.
   */

  // Default "real attr". This is the base case of the recursion
  template <typename AdapterKeyT, typename AttrT>
  typename std::remove_reference<AttrT>::type::ValueType getAttribute(
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
    saiApiCheckError(status, ApiT::ApiType, "Failed to get sai attribute");
    return attr.value();
  }

  // std::tuple of attributes
  template <typename AdapterKeyT, typename... AttrTs>
  auto getAttribute(const AdapterKeyT& key, std::tuple<AttrTs...>& attrTuple) {
    // TODO: assert on All<IsSaiAttribute>
    auto recurse = [&key, this](auto&& attr) {
      return getAttribute(key, attr);
    };
    return tupleMap(recurse, attrTuple);
  }

  // std::optional of attribute
  template <typename AdapterKeyT, typename AttrT>
  auto getAttribute(
      const AdapterKeyT& key,
      std::optional<AttrT>& attrOptional) {
    AttrT attr = attrOptional.value_or(AttrT{});
    auto res =
        std::optional<typename AttrT::ValueType>{getAttribute(key, attr)};
    return res;
  }

  template <typename AdapterKeyT, typename AttrT>
  sai_status_t setAttribute(const AdapterKeyT& key, const AttrT& attr) {
    return impl()._setAttribute(key, saiAttr(attr));
  }

 private:
  ApiT& impl() {
    return static_cast<ApiT&>(*this);
  }
};

} // namespace facebook::fboss
