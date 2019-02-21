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
class NeighborTypes;
class RouteTypes;
template <>
struct apiUsesObjectId<NeighborTypes> : public std::false_type {};
template <>
struct apiUsesObjectId<RouteTypes> : public std::false_type {};
template <>
struct apiUsesEntry<NeighborTypes> : public std::true_type {};
template <>
struct apiUsesEntry<RouteTypes> : public std::true_type {};

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
template <>
struct apiHasMembers<BridgeApiParameters> : public std::true_type {};
template <>
struct apiHasMembers<VlanApiParameters> : public std::true_type {};
template <>
struct apiHasMembers<LagTypes> : public std::true_type {};

/*
 * isDuplicateValueType<T>::value is true if T is a placeholder
 * type for a duplicate sai_attribute_t union value type. An example
 * is sai_object_id_t which is an alias of uint64_t, so the oid and u64
 * template specializations conflict.
 */
class SaiObjectIdT {};
template <typename T>
struct isDuplicateValueType : public std::false_type {};
template <>
struct isDuplicateValueType<SaiObjectIdT> : public std::true_type {};

} // namespace fboss
} // namespace facebook
