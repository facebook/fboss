// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once

namespace facebook {
namespace fboss {

/* if an object is not publisher, i.e. no other object depends on this object */
template <typename>
struct IsObjectPublisher : std::false_type {};

template <typename ObjectTraits>
struct IsPublisherAttributesAdapterHostKey : std::false_type {};

template <typename ObjectTraits>
struct IsPublisherAttributesCreateAttributes : std::false_type {};

namespace detail {
template <typename ObjectTraits, typename Attr>
struct PublisherAttributesInternal {
  using type = Attr;

  static auto get(
      typename ObjectTraits::AdapterHostKey adapterHostKey,
      typename ObjectTraits::CreateAttributes createAttributes) {
    if constexpr (IsPublisherAttributesAdapterHostKey<ObjectTraits>::value) {
      return adapterHostKey;
    } else if constexpr (IsPublisherAttributesCreateAttributes<
                             ObjectTraits>::value) {
      return createAttributes;
    } else {
      // TODO(pshaikh): lets do something here
      static_assert(
          IsPublisherAttributesAdapterHostKey<ObjectTraits>::value ||
              IsPublisherAttributesCreateAttributes<ObjectTraits>::value,
          "Custom PublisherAttributes are not supported");
    }
  }
};
} // namespace detail

template <typename, typename = void>
struct PublisherAttributes;

template <typename ObjectTraits>
struct PublisherAttributes<
    ObjectTraits,
    std::enable_if_t<
        IsPublisherAttributesAdapterHostKey<ObjectTraits>::value &&
        !IsPublisherAttributesCreateAttributes<ObjectTraits>::value>>
    : detail::PublisherAttributesInternal<
          ObjectTraits,
          typename ObjectTraits::AdapterHostKey> {};

template <typename ObjectTraits>
struct PublisherAttributes<
    ObjectTraits,
    std::enable_if_t<
        !IsPublisherAttributesAdapterHostKey<ObjectTraits>::value &&
        IsPublisherAttributesCreateAttributes<ObjectTraits>::value>>
    : detail::PublisherAttributesInternal<
          ObjectTraits,
          typename ObjectTraits::CreateAttributes> {};

template <typename ObjectTraits>
struct PublisherAttributes<
    ObjectTraits,
    std::enable_if_t<
        IsPublisherAttributesAdapterHostKey<ObjectTraits>::value &&
        IsPublisherAttributesCreateAttributes<ObjectTraits>::value>>
    : detail::PublisherAttributesInternal<
          ObjectTraits,
          typename ObjectTraits::CreateAttributes> {};
} // namespace fboss
} // namespace facebook
