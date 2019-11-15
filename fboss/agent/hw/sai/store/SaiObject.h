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

#include "fboss/agent/hw/sai/api/PortApi.h"
#include "fboss/agent/hw/sai/api/SaiApiTable.h"
#include "fboss/agent/hw/sai/api/Traits.h"
#include "fboss/lib/TupleUtils.h"

#include <variant>

namespace facebook::fboss {

namespace detail {

/*
 * For now, adapter host key extraction is a little clumsy.
 *
 * I thought it would be well matched by SFINAE on the Traits
 * object (particularly the relationship between AdapterHostKey and
 * CreateAttributes). However, the couple exceptions (default objects
 * using monostate and NextHopGroup being full on wild) resulted
 * in pretty complicated feeling logic being executed by "template
 * override selection" (if that is even a valid name for this...)
 * but really the logic in pseudo-code is:
 *
 * if AdapterHostKey is a single attribute:
 *   return that attribute
 * else if AdapterHostKey is a subset of attributes:
 *   return those attributes
 * else if AdapterHostKey is an entry type:
 *   return the entry type (AdapterKey)
 * else if the object is a single-instance default:
 *   return the bottom type (std::monostate)
 * else if the object is NextHopGroup:
 *   run the complicated, specific computation for NextHopGroup AdapterHostKey
 */

// If the adapter host key is one of the CreateAttributes of the object,
// simply extract that attribute from the tuple of CreateAttributes.
template <
    typename SaiObjectTraits,
    typename T = typename SaiObjectTraits::AdapterHostKey>
typename std::enable_if_t<
    IsElementOfTuple<T, typename SaiObjectTraits::CreateAttributes>::value,
    T>
adapterHostKey(
    const typename SaiObjectTraits::AdapterKey& adapterKey,
    const typename SaiObjectTraits::CreateAttributes& attributes) {
  static_assert(
      IsTupleOfSaiAttributes<typename SaiObjectTraits::CreateAttributes>::value,
      "attributes must be an std::tuple of SaiAttributes");
  static_assert(
      std::is_same_v<T, typename SaiObjectTraits::AdapterHostKey>,
      "placeholder template parameter must be adapter host key type");
  return std::get<typename SaiObjectTraits::AdapterHostKey>(attributes);
}

// If the adapter host key is a tuple subset of the CreateAttributes of the
// object (e.g., key=std:tuple<A,C>, create=std::tuple<A,B,C,D>), then
// project the full set of CreateAttributes onto the subset.
template <
    typename SaiObjectTraits,
    typename T = typename SaiObjectTraits::AdapterHostKey>
typename std::enable_if_t<
    IsSubsetOfTuple<T, typename SaiObjectTraits::CreateAttributes>::value,
    T>
adapterHostKey(
    const typename SaiObjectTraits::AdapterKey& adapterKey,
    const typename SaiObjectTraits::CreateAttributes& attributes) {
  static_assert(
      IsTupleOfSaiAttributes<typename SaiObjectTraits::CreateAttributes>::value,
      "attributes must be an std::tuple of SaiAttributes");
  static_assert(
      std::is_same_v<T, typename SaiObjectTraits::AdapterHostKey>,
      "placeholder template parameter must be adapter host key type");
  return tupleProjection<
      typename SaiObjectTraits::CreateAttributes,
      typename SaiObjectTraits::AdapterHostKey>(attributes);
}

// If the adapter host key is an entry type (e.g. RouteEntry, FdbEntry), simply
// return the adapter key, which must also be the entry type.
template <
    typename SaiObjectTraits,
    typename T = typename SaiObjectTraits::AdapterHostKey>
typename std::enable_if_t<IsSaiEntryStruct<T>::value, T> adapterHostKey(
    const typename SaiObjectTraits::AdapterKey& adapterKey,
    const typename SaiObjectTraits::CreateAttributes& attributes) {
  static_assert(
      std::is_same_v<T, typename SaiObjectTraits::AdapterHostKey>,
      "placeholder template parameter must be adapter host key type");
  return adapterKey;
}

// If the object always has only a single value, the key will be std::monostate
template <
    typename SaiObjectTraits,
    typename T = typename SaiObjectTraits::AdapterHostKey>
typename std::enable_if_t<std::is_same_v<T, std::monostate>, T> adapterHostKey(
    const typename SaiObjectTraits::AdapterKey& adapterKey,
    const typename SaiObjectTraits::CreateAttributes& attributes) {
  static_assert(
      std::is_same_v<T, typename SaiObjectTraits::AdapterHostKey>,
      "placeholder template parameter must be adapter host key type");
  return std::monostate{};
}

// Exactly a special case for NextHopGroup
template <
    typename SaiObjectTraits,
    typename T = typename SaiObjectTraits::AdapterHostKey>
typename std::
    enable_if_t<std::is_same_v<SaiObjectTraits, SaiNextHopGroupTraits>, T>
    adapterHostKey(
        const typename SaiObjectTraits::AdapterKey& adapterKey,
        const typename SaiObjectTraits::CreateAttributes& attributes) {
  typename SaiObjectTraits::AdapterHostKey ret;
  auto apiTable = SaiApiTable::getInstance();
  auto memberIds = apiTable->nextHopGroupApi().getAttribute(
      adapterKey, SaiNextHopGroupTraits::Attributes::NextHopMemberList{});
  for (const auto memberId : memberIds) {
    NextHopSaiId nextHopSaiId{apiTable->nextHopGroupApi().getAttribute(
        NextHopGroupMemberSaiId(memberId),
        SaiNextHopGroupMemberTraits::Attributes::NextHopId{})};
    SaiNextHopTraits::AdapterHostKey nhk{};
    apiTable->nextHopApi().getAttribute(nextHopSaiId, nhk);
    ret.insert(nhk);
  }
  return ret;
}

} // namespace detail

/*
 * SaiObject is a generic object which manages an object in the SAI adapter.
 *
 * It is an RAII type, which can be empty (the live_ field denotes this) if
 * moved from. If it is live, destroying the SaiObject removes the
 * corresponding object from SAI.
 *
 * A SaiObject can be constructed in three ways:
 * 1. By loading it from the SAI adapter using the AdapterKey. This can be
 *    thought of as the SaiObject taking control of an existing object in SAI.
 * 2. By creating a new object in the SAI adapter using the AdapterHostKey and
 *    CreateAttributes
 * 3. Moving from another SaiObject. If the moved-from SaiObject was live,
 *    after the move, it is no longer live, so that at any point, only one
 *    SaiObject manages a given SAI object. (N.B., there is no general hard
 *    guarantee for this property -- a user could load the same SaiObject more
 *    than once).
 * In all three cases, (excepting the unlikely event of moving from a non-live
 * SaiObject), the newly constructed SaiObject is live and stores the
 * appropriate values of AdapterHostKey, AdapterKey, and CreateAttributes.
 *
 * Finally, SaiObject supports setting new values for the attributes. Given
 * a CreateAttributes, setAttributes will iterate over each attribute and
 * check the existing value against the new value. If they are unequal, we
 * set the new value. A particularly interesting case is an optional
 * attribute that goes from set to unset. In that case, we need to set the
 * attribute value back to a default value (which SAI always defines for any
 * optional attribute). That behavior is not yet correctly implemented in
 * SaiObject. TODO(borisb): remove this last note once we handle unsetting
 * optional attributes.
 */
template <typename SaiObjectTraits>
class SaiObject {
 public:
  // Load from adapter key
  explicit SaiObject(const typename SaiObjectTraits::AdapterKey& adapterKey)
      : adapterKey_(adapterKey) {
    auto& api =
        SaiApiTable::getInstance()->getApi<typename SaiObjectTraits::SaiApiT>();
    // N.B., fills out attributes_ as a side effect
    // XXX TODO: side-effect mode does NOT work with optionals
    attributes_ = api.getAttribute(adapterKey_, attributes_);
    live_ = true;
    adapterHostKey_ =
        detail::adapterHostKey<SaiObjectTraits>(adapterKey_, attributes_);
  }

  // Create a new one from adapter host key and attributes
  SaiObject(
      const typename SaiObjectTraits::AdapterHostKey& adapterHostKey,
      const typename SaiObjectTraits::CreateAttributes& attributes,
      sai_object_id_t switchId)
      : adapterHostKey_(adapterHostKey), attributes_(attributes) {
    adapterKey_ = createHelper(adapterHostKey, attributes, switchId);
    live_ = true;
  }

  // Forbid copy construction and copy assignment
  SaiObject(const SaiObject& other) = delete;
  SaiObject& operator=(const SaiObject& other) = delete;

  // Implemented in terms of move assignment
  SaiObject(SaiObject&& other) {
    *this = std::move(other);
  }

  SaiObject& operator=(SaiObject&& other) {
    if (LIKELY(other.live_)) {
      // TODO(borisb): move members instead of copy?!
      adapterKey_ = other.adapterKey();
      adapterHostKey_ = other.adapterHostKey();
      attributes_ = other.attributes();
      live_ = true;
      other.live_ = false;
    } else {
      live_ = false;
    }
    return *this;
  }

  ~SaiObject() {
    if (live_) {
      remove();
    }
    live_ = false;
  }

  void setAttributes(
      const typename SaiObjectTraits::CreateAttributes& newAttributes) {
    if (UNLIKELY(!live_)) {
      XLOG(FATAL) << "Attempted to setAttributes on non-live SaiObject";
    }
    auto checkAndSetAttribute = [this](const auto& newAttr) {
      const auto& oldAttr =
          std::get<std::decay_t<decltype(newAttr)>>(attributes_);
      if (oldAttr != newAttr) {
        setNewAttributeHelper(newAttr);
      }
    };
    tupleForEach(checkAndSetAttribute, newAttributes);
    attributes_ = newAttributes;
  }

  void release() {
    live_ = false;
  }

  const typename SaiObjectTraits::AdapterKey& adapterKey() const {
    if (UNLIKELY(!live_)) {
      XLOG(FATAL) << "Attempted to get Adapter Key on non-live SaiObject";
    }
    return adapterKey_;
  }

  const typename SaiObjectTraits::AdapterHostKey& adapterHostKey() const {
    if (UNLIKELY(!live_)) {
      XLOG(FATAL) << "Attempted to get Adapter Host Key of non-live SaiObject";
    }
    return adapterHostKey_;
  }

  const typename SaiObjectTraits::CreateAttributes& attributes() const {
    if (UNLIKELY(!live_)) {
      XLOG(FATAL) << "Attempted to get attributes of non-live SaiObject";
    }
    return attributes_;
  }

 protected:
  void remove() {
    if constexpr (not IsSaiObjectOwnedByAdapter<SaiObjectTraits>::value) {
      auto& api = SaiApiTable::getInstance()
                      ->getApi<typename SaiObjectTraits::SaiApiT>();
      api.remove(adapterKey_);
    }
  }
  template <typename AttrT>
  void setNewAttributeHelper(const AttrT& newAttr) {
    auto& api =
        SaiApiTable::getInstance()->getApi<typename SaiObjectTraits::SaiApiT>();
    api.setAttribute(adapterKey(), newAttr);
  }
  template <typename AttrT>
  void setNewAttributeHelper(const std::optional<AttrT>& newAttrOpt) {
    if (newAttrOpt) {
      setNewAttributeHelper(newAttrOpt.value());
    } else {
      // TODO(borisb): handle un-setting optional attributes to default
      // properly
    }
  }

  template <typename T = SaiObjectTraits>
  std::enable_if_t<AdapterKeyIsObjectId<T>::value, typename T::AdapterKey>
  createHelper(
      const typename T::AdapterHostKey& k,
      const typename T::CreateAttributes& attributes,
      sai_object_id_t switchId) {
    auto& api =
        SaiApiTable::getInstance()->getApi<typename SaiObjectTraits::SaiApiT>();
    return api.template create<T>(attributes, switchId);
  }

  template <typename T = SaiObjectTraits>
  std::enable_if_t<AdapterKeyIsEntryStruct<T>::value, typename T::AdapterKey>
  createHelper(
      const typename T::AdapterHostKey& k,
      const typename T::CreateAttributes& attributes,
      sai_object_id_t switchId) {
    // Entry structs have AdapterKey = AdapterHostKey
    static_assert(
        std::is_same_v<typename T::AdapterHostKey, typename T::AdapterKey>,
        "SAI objects which use an entry struct must have "
        "AdapterKey == AdapterHostKey == entry struct");
    auto& api =
        SaiApiTable::getInstance()->getApi<typename SaiObjectTraits::SaiApiT>();
    api.template create<T>(k, attributes);
    return k;
  }

 private:
  bool live_{false};
  typename SaiObjectTraits::AdapterKey adapterKey_;
  typename SaiObjectTraits::AdapterHostKey adapterHostKey_;
  typename SaiObjectTraits::CreateAttributes attributes_;
};

/*
 * A couple helpful macros to make using the attributes tuple in SaiObjects
 * much more palatable.
 * compare:
 *
 * auto speed = GET_ATTR(Port, Speed, object.attributes());
 * to
 * auto speed =
 *   std::get<SaiPortTraits::Attributes::Speed>(object.attributes()).value();
 *
 * or even worse:
 *
 * auto mac = GET_OPT_ATTR(RouterInterface, SrcMac, object.attributes());
 * to
 * auto mac =
 *   std::get<std::optional<SaiRouterInterfaceTraits::Attributes::SrcMac>>(
 *     object.attributes()).value().value();
 */
#define GET_ATTR(obj, attr, tuple) \
  std::get<Sai##obj##Traits::Attributes::attr>(tuple).value()

#define GET_OPT_ATTR(obj, attr, tuple)                               \
  std::get<std::optional<Sai##obj##Traits::Attributes::attr>>(tuple) \
      .value()                                                       \
      .value()

} // namespace facebook::fboss
