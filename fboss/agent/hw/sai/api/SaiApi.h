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
#include "fboss/agent/hw/sai/api/SaiApiLock.h"
#include "fboss/agent/hw/sai/api/SaiAttribute.h"
#include "fboss/agent/hw/sai/api/SaiAttributeDataTypes.h"
#include "fboss/agent/hw/sai/api/Traits.h"
#include "fboss/lib/FunctionCallTimeReporter.h"
#include "fboss/lib/HwWriteBehavior.h"
#include "fboss/lib/TupleUtils.h"

#include <folly/Format.h>
#include <folly/logging/xlog.h>

#include <algorithm>
#include <exception>
#include <mutex>
#include <stdexcept>
#include <type_traits>
#include <vector>

#include <fmt/format.h>

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
      sai_object_id_t switch_id) const {
    static_assert(
        std::is_same_v<typename SaiObjectTraits::SaiApiT, ApiT>,
        "invalid traits for the api");
    typename SaiObjectTraits::AdapterKey key;
    std::vector<sai_attribute_t> saiAttributeTs = saiAttrs(createAttributes);
    if (UNLIKELY(failHwWrites() || skipHwWrites())) {
      // Fail hard on both skip and fail Hw write settings. For FAIL, its
      // obvious why we fail hard. For SKIP, we fail since the expectation here
      // is to return a key from adapter, which we can't manufacture out of thin
      // air
      XLOGF(
          FATAL,
          "Attempting create SAI obj with {}, while hw writes are blocked",
          createAttributes);
    }
    auto g{SaiApiLock::getInstance()->lock()};
    sai_status_t status;
    {
      TIME_CALL;
      status = impl()._create(
          &key, switch_id, saiAttributeTs.size(), saiAttributeTs.data());
    }
    saiApiCheckError(
        status,
        apiType(),
        fmt::format(
            "Failed to create sai entity {}: {}", key, createAttributes));
    XLOGF(DBG5, "created SAI object: {}: {}", key, createAttributes);
    return key;
  }

  // entry struct case
  template <typename SaiObjectTraits>
  std::enable_if_t<AdapterKeyIsEntryStruct<SaiObjectTraits>::value, void>
  create(
      const typename SaiObjectTraits::AdapterKey& entry,
      const typename SaiObjectTraits::CreateAttributes& createAttributes)
      const {
    static_assert(
        std::is_same_v<typename SaiObjectTraits::SaiApiT, ApiT>,
        "invalid traits for the api");
    if (UNLIKELY(skipHwWrites())) {
      return;
    }
    std::vector<sai_attribute_t> saiAttributeTs = saiAttrs(createAttributes);
    if (UNLIKELY(failHwWrites())) {
      XLOGF(
          FATAL,
          "Attempting create SAI obj with {}, while hw writes are blocked",
          createAttributes);
    }
    auto g{SaiApiLock::getInstance()->lock()};
    sai_status_t status;
    {
      TIME_CALL;
      status =
          impl()._create(entry, saiAttributeTs.size(), saiAttributeTs.data());
    }
    saiApiCheckError(
        status,
        apiType(),
        fmt::format(
            "Failed to create sai entity: {}: {}", entry, createAttributes));
    XLOGF(DBG5, "created SAI object: {}: {}", entry, createAttributes);
  }

  template <typename AdapterKeyT>
  void remove(const AdapterKeyT& key) const {
    if (UNLIKELY(skipHwWrites())) {
      return;
    }
    if (UNLIKELY(failHwWrites())) {
      XLOGF(
          FATAL,
          "Attempting to remove SAI obj {} while hw writes are blocked",
          key);
    }
    auto g{SaiApiLock::getInstance()->lock()};
    sai_status_t status;
    {
      TIME_CALL;
      status = impl()._remove(key);
    }
    saiApiCheckError(
        status,
        apiType(),
        fmt::format("Failed to remove sai object : {}", key));
    XLOGF(DBG5, "removed SAI object: {}", key);
  }

  /*
   * We can do getAttribute on top of more complicated types than just
   * attributes. For example, if we overload on tuples and optionals, we
   * can recursively call getAttribute on their internals to do more
   * interesting gets. This is particularly useful when we want to load
   * complicated aggregations of SaiAttributes for something like warm boot.
   */

  // Default "real attr". This is the base case of the recursion
  template <
      typename AdapterKeyT,
      typename AttrT,
      typename = std::enable_if_t<
          IsSaiAttribute<std::remove_reference_t<AttrT>>::value>>
  typename std::remove_reference_t<AttrT>::ValueType getAttribute(
      const AdapterKeyT& key,
      AttrT&& attr) const {
    static_assert(
        IsSaiAttribute<typename std::remove_reference<AttrT>::type>::value,
        "getAttribute must be called on a SaiAttribute or supported "
        "collection of SaiAttributes");
    auto g{SaiApiLock::getInstance()->lock()};
    sai_status_t status;
    {
      TIME_CALL;
      status = impl()._getAttribute(key, attr.saiAttr());
    }
    /*
     * If this is a list attribute and we have not allocated enough
     * memory for the data coming from SAI, the Adapter will return
     * SAI_STATUS_BUFFER_OVERFLOW and fill in `count` in the list object.
     * We can take advantage of that to allocate a proper buffer and
     * try the get again.
     */
    if (status == SAI_STATUS_BUFFER_OVERFLOW) {
      attr.realloc();
      {
        TIME_CALL;
        status = impl()._getAttribute(key, attr.saiAttr());
      }
    }
    if constexpr (!std::remove_reference_t<AttrT>::HasDefaultGetter) {
      saiApiCheckError(
          status,
          apiType(),
          fmt::format("Failed to get sai attribute: {}: {}", key, attr));
    } else if (status != SAI_STATUS_SUCCESS) {
      throw SaiApiError(
          status,
          apiType(),
          fmt::format("Failed to get sai attribute: {}: {}", key, attr));
    }
    XLOGF(DBG5, "got SAI attribute: {}: {}", key, attr);
    return attr.value();
  }

  // std::tuple of attributes
  template <
      typename AdapterKeyT,
      typename TupleT,
      typename =
          std::enable_if_t<IsTuple<std::remove_reference_t<TupleT>>::value>>
  const std::remove_reference_t<TupleT> getAttribute(
      const AdapterKeyT& key,
      TupleT&& attrTuple) const {
    // TODO: assert on All<IsSaiAttribute>
    auto recurse = [&key, this](auto&& attr) {
      return getAttribute(key, std::forward<decltype(attr)>(attr));
    };
    return tupleMap(recurse, std::forward<TupleT>(attrTuple));
  }

  // std::optional of attribute
  template <
      typename AdapterKeyT,
      typename AttrT,
      typename = std::enable_if_t<
          IsSaiAttribute<std::remove_reference_t<AttrT>>::value>>
  auto getAttribute(const AdapterKeyT& key, std::optional<AttrT>& attrOptional)
      const {
    if constexpr (IsSaiExtensionAttribute<AttrT>::value) {
      auto id = typename AttrT::AttributeId()();
      if (!id.has_value()) {
        // if attribute is not supported, do not query it
        XLOGF(DBG5, "extension SAI attribute is not supported {}", key);
        return std::optional<typename AttrT::ValueType>(std::nullopt);
      }
    }
    AttrT attr = attrOptional.value_or(AttrT{});
    try {
      auto result = getAttribute(key, attr);
      if constexpr (IsVector<decltype(result)>::value) {
        if (result.empty()) {
          return std::optional<typename AttrT::ValueType>();
        }
      }
      return std::optional<typename AttrT::ValueType>(result);
    } catch (const SaiApiError& e) {
      if constexpr (std::remove_reference_t<AttrT>::HasDefaultGetter) {
        /*
         * If default value is provided, allow fallback in case of certain
         * error conditions (primarily to support the case where adapter should,
         * but does not return a default value for unimplemented attributes).
         * The error codes here are not very precise and may need to be revised
         * based on experience.
         */
        static constexpr std::array<sai_status_t, 2> kFallbackToDefaultErrCode{
            SAI_STATUS_NOT_IMPLEMENTED, SAI_STATUS_NOT_SUPPORTED};
        auto status = e.getSaiStatus();
        auto fallbackToDefault =
            std::find(
                kFallbackToDefaultErrCode.begin(),
                kFallbackToDefaultErrCode.end(),
                status) != kFallbackToDefaultErrCode.end() ||
            SAI_STATUS_IS_ATTR_NOT_IMPLEMENTED(status) ||
            SAI_STATUS_IS_ATTR_NOT_SUPPORTED(status);
        if (fallbackToDefault) {
          return std::optional<typename AttrT::ValueType>{attr.defaultValue()};
        }
      }
      throw;
    }
  }

  template <typename AdapterKeyT, typename AttrT>
  void setAttributeUnlocked(const AdapterKeyT& key, const AttrT& attr) const {
    if (UNLIKELY(skipHwWrites())) {
      return;
    }
    if (UNLIKELY(failHwWrites())) {
      XLOGF(
          FATAL,
          "Attempting set SAI attribute of {} to {}, while hw writes are blocked",
          key,
          attr);
    }
    if constexpr (IsSaiExtensionAttribute<AttrT>::value) {
      auto id = typename AttrT::AttributeId()();
      if (!id.has_value()) {
        // if attribute is not supported, do not set it
        XLOGF(
            FATAL,
            "attempting to set unsupported extension SAI attribute {} for key {}",
            attr,
            key);
      }
    }
    sai_status_t status;
    {
      TIME_CALL;
      status = impl()._setAttribute(key, saiAttr(attr));
    }
    saiApiCheckError(
        status,
        apiType(),
        fmt::format("Failed to set attribute {} to {}", key, attr));
    XLOGF(DBG5, "set SAI attribute of {} to {}", key, attr);
  }
  template <typename AdapterKeyT, typename AttrT>
  void setAttribute(const AdapterKeyT& key, const AttrT& attr) const {
    auto g{SaiApiLock::getInstance()->lock()};
    setAttributeUnlocked(key, attr);
  }

  template <typename AdapterKeyT, typename AttrT>
  void bulkSetAttributesUnlocked(
      std::vector<AdapterKeyT>& adapterKeys,
      std::vector<AttrT>& attributes) const {
    if (UNLIKELY(skipHwWrites())) {
      return;
    }
    if (UNLIKELY(failHwWrites())) {
      XLOG(
          FATAL,
          "Attempting bulk set SAI attributes while hw writes are blocked");
    }
    if constexpr (IsSaiExtensionAttribute<AttrT>::value) {
      auto id = typename AttrT::AttributeId()();
      if (!id.has_value()) {
        // if attribute is not supported, do not set it
        XLOG(
            FATAL,
            "attempting to bulk set unsupported extension SAI attribute");
      }
    }
    std::vector<sai_attribute_t> attrs;
    for (const auto& attr : attributes) {
      attrs.emplace_back(*saiAttr(attr));
    }
    sai_status_t status;
    sai_status_t retStatus[adapterKeys.size()];
    {
      TIME_CALL;
      status = impl()._bulkSetAttribute(
          adapterKeys.data(), attrs.data(), retStatus, adapterKeys.size());
    }
    saiApiCheckError(
        status, apiType(), fmt::format("Failed to bulk set attribute"));
    for (auto idx = 0; idx < adapterKeys.size(); idx++) {
      saiApiCheckError(
          retStatus[idx],
          apiType(),
          fmt::format(
              "Failed to set attribute {} to {}",
              adapterKeys[idx],
              attributes[idx]));
      XLOGF(
          DBG5,
          "bulk set SAI attribute of {} to {}",
          adapterKeys[idx],
          attributes[idx]);
    }
  }

  template <typename AdapterKeyT, typename AttrT>
  void bulkSetAttributes(
      std::vector<AdapterKeyT>& adapterKeys,
      std::vector<AttrT>& attributes) const {
    auto g{SaiApiLock::getInstance()->lock()};
    return bulkSetAttributesUnlocked(adapterKeys, attributes);
  }

  template <typename SaiObjectTraits>
  std::vector<uint64_t> getStats(
      const typename SaiObjectTraits::AdapterKey& key,
      const std::vector<sai_stat_id_t>& counterIds,
      sai_stats_mode_t mode) const {
    static_assert(
        SaiObjectHasStats<SaiObjectTraits>::value,
        "getStats only supported for Sai objects with stats");
    auto g{SaiApiLock::getInstance()->lock()};
    return getStatsImpl<SaiObjectTraits>(
        key, counterIds.data(), counterIds.size(), mode);
  }
  template <typename SaiObjectTraits>
  std::vector<uint64_t> getStats(
      const typename SaiObjectTraits::AdapterKey& key,
      sai_stats_mode_t mode) const {
    static_assert(
        SaiObjectHasStats<SaiObjectTraits>::value,
        "getStats only supported for Sai objects with stats");
    auto g{SaiApiLock::getInstance()->lock()};
    XLOGF(DBG6, "got SAI stats for {}", key);
    return mode == SAI_STATS_MODE_READ
        ? getStatsImpl<SaiObjectTraits>(
              key,
              SaiObjectTraits::CounterIdsToRead.data(),
              SaiObjectTraits::CounterIdsToRead.size(),
              mode)
        : getStatsImpl<SaiObjectTraits>(
              key,
              SaiObjectTraits::CounterIdsToReadAndClear.data(),
              SaiObjectTraits::CounterIdsToReadAndClear.size(),
              mode);
  }

  template <typename SaiObjectTraits>
  void clearStats(
      const typename SaiObjectTraits::AdapterKey& key,
      const std::vector<sai_stat_id_t>& counterIds) const {
    static_assert(
        SaiObjectHasStats<SaiObjectTraits>::value,
        "clearStats only supported for Sai objects with stats");
    auto g{SaiApiLock::getInstance()->lock()};
    clearStatsImpl<SaiObjectTraits>(key, counterIds.data(), counterIds.size());
  }
  template <typename SaiObjectTraits>
  void clearStats(const typename SaiObjectTraits::AdapterKey& key) const {
    static_assert(
        SaiObjectHasStats<SaiObjectTraits>::value,
        "clearStats only supported for Sai objects with stats");
    auto g{SaiApiLock::getInstance()->lock()};
    clearStatsImpl<SaiObjectTraits>(
        key,
        SaiObjectTraits::CounterIdsToRead.data(),
        SaiObjectTraits::CounterIdsToRead.size());
    clearStatsImpl<SaiObjectTraits>(
        key,
        SaiObjectTraits::CounterIdsToReadAndClear.data(),
        SaiObjectTraits::CounterIdsToReadAndClear.size());
  }
  sai_api_t apiType() const {
    return ApiT::ApiType;
  }

 private:
  bool failHwWrites() const {
    return getHwWriteBehavior() == HwWriteBehavior::FAIL;
  }
  bool skipHwWrites() const {
    return getHwWriteBehavior() == HwWriteBehavior::SKIP;
  }
  template <typename SaiObjectTraits>
  std::vector<uint64_t> getStatsImpl(
      const typename SaiObjectTraits::AdapterKey& key,
      const sai_stat_id_t* counterIds,
      size_t numCounters,
      sai_stats_mode_t mode) const {
    std::vector<uint64_t> counters;
    if (numCounters) {
      counters.resize(numCounters);
      sai_status_t status;
      {
        TIME_CALL
        status = impl()._getStats(
            key, counters.size(), counterIds, mode, counters.data());
      }
      saiApiCheckError(
          status, apiType(), fmt::format("Failed to get stats {}", key));
      saiApiCheckError(status, apiType(), "Failed to get stats");
    }
    return counters;
  }
  template <typename SaiObjectTraits>
  void clearStatsImpl(
      const typename SaiObjectTraits::AdapterKey& key,
      const sai_stat_id_t* counterIds,
      size_t numCounters) const {
    if (UNLIKELY(skipHwWrites())) {
      return;
    }
    if (numCounters) {
      if (UNLIKELY(failHwWrites())) {
        XLOGF(
            FATAL,
            "Attempting clear stats {} , while hw writes are blocked",
            saiApiTypeToString(apiType()));
      }
      sai_status_t status;
      {
        TIME_CALL
        status = impl()._clearStats(key, numCounters, counterIds);
      }
      saiApiCheckError(status, apiType(), "Failed to clear stats");
    }
  }
  ApiT& impl() {
    return static_cast<ApiT&>(*this);
  }
  const ApiT& impl() const {
    return static_cast<const ApiT&>(*this);
  }
};

} // namespace facebook::fboss
