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

/*
 * apiHasMembers<T>::value is true if T is an ApiTypes which
 * has nested "members". Examples of SaiApis whose types satisfy
 * this are VlanApi and NextHopGroupApi
 */
template <typename ApiTypes>
struct apiHasMembers : public std::false_type {};

} // namespace fboss
} // namespace facebook
