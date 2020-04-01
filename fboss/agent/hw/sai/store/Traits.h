// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once

namespace facebook {
namespace fboss {

/* if an object is not publisher, i.e. no other object depends on this object */
template <typename, typename = std::void_t<>>
struct IsObjectPublisher : std::false_type {};

/* if an object is a publisher, i.e. some object depends on this object,
 * PublisherAttributes define a tuple of important attributes that govern life
 * or characteristic of that other object */
template <typename ObjectTraits>
struct IsObjectPublisher<
    ObjectTraits,
    std::void_t<typename ObjectTraits::PublisherAttributes>> : std::true_type {
};

template <typename ObjectTraits>
inline constexpr bool IsPublisherAttributesAdapterHostKey = std::is_same_v<
    typename ObjectTraits::PublisherAttributes,
    typename ObjectTraits::AdapterHostKey>;

template <typename ObjectTraits>
inline constexpr bool IsPublisherAttributesCreateAttributes = std::is_same_v<
    typename ObjectTraits::PublisherAttributes,
    typename ObjectTraits::CreateAttributes>;

namespace detail {
template <typename ObjectTraits, typename Attr>
struct PublisherAttributesInternal {
  using type = Attr;

  static auto get(
      typename ObjectTraits::AdapterHostKey adapterHostKey,
      typename ObjectTraits::CreateAttributes createAttributes) {
    if constexpr (IsPublisherAttributesAdapterHostKey<ObjectTraits>) {
      return adapterHostKey;
    } else if constexpr (IsPublisherAttributesCreateAttributes<ObjectTraits>) {
      return createAttributes;
    } else {
      // TODO(pshaikh): lets do something here
      static_assert(
          IsPublisherAttributesAdapterHostKey<ObjectTraits> ||
              IsPublisherAttributesCreateAttributes<ObjectTraits>,
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
        IsPublisherAttributesAdapterHostKey<ObjectTraits> &&
        !IsPublisherAttributesCreateAttributes<ObjectTraits>>>
    : detail::PublisherAttributesInternal<
          ObjectTraits,
          typename ObjectTraits::AdapterHostKey> {};

template <typename ObjectTraits>
struct PublisherAttributes<
    ObjectTraits,
    std::enable_if_t<
        !IsPublisherAttributesAdapterHostKey<ObjectTraits> &&
        IsPublisherAttributesCreateAttributes<ObjectTraits>>>
    : detail::PublisherAttributesInternal<
          ObjectTraits,
          typename ObjectTraits::CreateAttributes> {};

template <typename ObjectTraits>
struct PublisherAttributes<
    ObjectTraits,
    std::enable_if_t<
        IsPublisherAttributesAdapterHostKey<ObjectTraits> &&
        IsPublisherAttributesCreateAttributes<ObjectTraits>>>
    : detail::PublisherAttributesInternal<
          ObjectTraits,
          typename ObjectTraits::CreateAttributes> {};
} // namespace fboss
} // namespace facebook
