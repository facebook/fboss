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
#include <type_traits>

extern "C" {
#include <sai.h>
}

namespace facebook {
namespace fboss {

/*
 * apiUsesObjectId<T>::value is true if T is an ApiTypes whose
 * create/remove/getAttribute/setAttribute use sai_object_id_t
 * for lookup. create will return a sai_object_id_t
 * Examples of SaiApis whose types satisfy this include PortApi, SwitchApi,
 * BridgeApi.
 * apiUsesEntry<T>::value will be false.
 */
template <typename ApiTypes>
struct apiUsesObjectId : public std::true_type {};

/*
 * apiUsesEntry<T>::value is true if T is an ApiTypes whose
 * create/remove/getAttribute/setAttribute use an entry
 * struct for lookup.
 * Examples of SaiApis whose types satisfy this include RouteApi
 * apiUsesObjectId<T>::value will be false
 */
template <typename ApiTypes>
struct apiUsesEntry : public std::false_type {};
class NeighborApiParameters;
class RouteApiParameters;
class FdbApiParameters;
template <>
struct apiUsesObjectId<NeighborApiParameters> : public std::false_type {};
template <>
struct apiUsesObjectId<RouteApiParameters> : public std::false_type {};
template <>
struct apiUsesObjectId<FdbApiParameters> : public std::false_type {};
template <>
struct apiUsesEntry<NeighborApiParameters> : public std::true_type {};
template <>
struct apiUsesEntry<RouteApiParameters> : public std::true_type {};
template <>
struct apiUsesEntry<FdbApiParameters> : public std::true_type {};

/*
 * apiHasMembers<T>::value is true if T is an ApiTypes which
 * has nested "members". Examples of SaiApis whose types satisfy
 * this are VlanApi and NextHopGroupApi
 */
template <typename ApiTypes>
struct apiHasMembers : public std::false_type {};

class BridgeApiParameters;
class VlanApiParameters;
class LagTypes;
class NextHopGroupApiParameters;
class HostifApiParameters;
template <>
struct apiHasMembers<BridgeApiParameters> : public std::true_type {};
template <>
struct apiHasMembers<VlanApiParameters> : public std::true_type {};
template <>
struct apiHasMembers<LagTypes> : public std::true_type {};
template <>
struct apiHasMembers<NextHopGroupApiParameters> : public std::true_type {};
template <>
struct apiHasMembers<HostifApiParameters> : public std::true_type {};

/*
 * Helper metafunctions for C++ wrappers of non-primitive SAI types.
 *
 * e.g., folly::MacAddress for sai_mac_t, folly::IPAddress for
 * sai_ip_address_t, std::vector<uint64_t> for sai_object_list_t
 */
template <typename T>
struct WrappedSaiType {
  using value = T;
};

template <>
struct WrappedSaiType<folly::MacAddress> {
  using value = sai_mac_t;
};

template <>
struct WrappedSaiType<folly::IPAddressV4> {
  using value = sai_ip4_t;
};

template <>
struct WrappedSaiType<folly::IPAddressV6> {
  using value = sai_ip6_t;
};

template <>
struct WrappedSaiType<folly::IPAddress> {
  using value = sai_ip_address_t;
};

template <>
struct WrappedSaiType<folly::CIDRNetwork> {
  using value = sai_ip_prefix_t;
};

template <>
struct WrappedSaiType<std::vector<sai_object_id_t>> {
  using value = sai_object_list_t;
};

template <>
struct WrappedSaiType<std::vector<sai_int8_t>> {
  using value = sai_s8_list_t;
};

template <>
struct WrappedSaiType<std::vector<sai_uint8_t>> {
  using value = sai_u8_list_t;
};

template <>
struct WrappedSaiType<std::vector<sai_int16_t>> {
  using value = sai_s16_list_t;
};

template <>
struct WrappedSaiType<std::vector<sai_uint16_t>> {
  using value = sai_u16_list_t;
};

template <>
struct WrappedSaiType<std::vector<sai_int32_t>> {
  using value = sai_s32_list_t;
};

template <>
struct WrappedSaiType<std::vector<sai_uint32_t>> {
  using value = sai_u32_list_t;
};

template <typename T>
struct IsSaiTypeWrapper
    : std::negation<std::is_same<typename WrappedSaiType<T>::value, T>> {};

/*
 * Helper metafunctions for resolving two types in the SAI
 * sai_attribute_value_t union being aliases. This results in SaiAttribute
 * not being able to select the correct union member from sai_attribute_t
 * from the SAI type alone. i.e., how could it choose between attr.value.oid
 * and attr.value.u64 based on just the fact that it is extracting a 64 bit
 * unsigned integer?
 *
 * e.g., sai_object_id_t vs uint64_t and sai_ip4_t vs uint32_t;
 */
struct SaiObjectIdT {};

template <typename T>
struct DuplicateTypeFixer {
  using value = T;
};

template <>
struct DuplicateTypeFixer<SaiObjectIdT> {
  using value = sai_object_id_t;
};

template <>
struct DuplicateTypeFixer<folly::IPAddressV4> {
  using value = sai_ip4_t;
};

template <typename T>
struct IsDuplicateSaiType
    : std::negation<std::is_same<typename DuplicateTypeFixer<T>::value, T>> {};

template <typename T>
struct IsSaiAttribute : public std::false_type {};

template <typename AttrT>
struct IsSaiAttribute<std::optional<AttrT>> : public IsSaiAttribute<AttrT> {};

template <typename T>
struct IsSaiEntryStruct : public std::false_type {};

template <typename SaiObjectTraits>
struct AdapterKeyIsEntryStruct
    : public IsSaiEntryStruct<typename SaiObjectTraits::AdapterHostKey> {};

template <typename SaiObjectTraits>
struct AdapterKeyIsObjectId
    : std::negation<AdapterKeyIsEntryStruct<SaiObjectTraits>> {};

template <typename T>
struct IsTupleOfSaiAttributes : public std::false_type {};

template <typename... AttrTs>
struct IsTupleOfSaiAttributes<std::tuple<AttrTs...>>
    : public std::conjunction<IsSaiAttribute<AttrTs>...> {};

} // namespace fboss
} // namespace facebook
