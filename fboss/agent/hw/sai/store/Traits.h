// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once

namespace facebook::fboss {

/* if an object is not publisher, i.e. no other object depends on this object */
template <typename>
struct IsObjectPublisher : std::false_type {};

template <typename ObjectTraits>
struct IsPublisherKeyAdapterHostKey : std::false_type {};

template <typename ObjectTraits>
struct IsPublisherKeyCreateAttributes : std::false_type {};

template <typename ObjectTraits, typename = void>
struct IsPublisherKeyCustomType : std::false_type {};

namespace detail {
template <typename ObjectTraits, typename Attr>
struct PublisherKeyInternal {
  using type = Attr;
  using custom_type = std::conditional_t<
      IsPublisherKeyCustomType<ObjectTraits>::value,
      Attr,
      std::monostate>;
};
} // namespace detail

template <typename T, typename = void>
struct PublisherKey : detail::PublisherKeyInternal<T, void> {};

template <typename ObjectTraits>
struct PublisherKey<
    ObjectTraits,
    std::enable_if_t<
        IsPublisherKeyAdapterHostKey<ObjectTraits>::value &&
        !IsPublisherKeyCreateAttributes<ObjectTraits>::value &&
        !IsPublisherKeyCustomType<ObjectTraits>::value>>
    : detail::PublisherKeyInternal<
          ObjectTraits,
          typename ObjectTraits::AdapterHostKey> {};

template <typename ObjectTraits>
struct PublisherKey<
    ObjectTraits,
    std::enable_if_t<
        !IsPublisherKeyAdapterHostKey<ObjectTraits>::value &&
        IsPublisherKeyCreateAttributes<ObjectTraits>::value &&
        !IsPublisherKeyCustomType<ObjectTraits>::value>>
    : detail::PublisherKeyInternal<
          ObjectTraits,
          typename ObjectTraits::CreateAttributes> {};

template <typename ObjectTraits>
struct PublisherKey<
    ObjectTraits,
    std::enable_if_t<
        IsPublisherKeyAdapterHostKey<ObjectTraits>::value &&
        std::is_same_v<
            ObjectTraits::CreateAttributes,
            ObjectTraits::AdapterHostKey>>>
    : detail::PublisherKeyInternal<
          ObjectTraits,
          typename ObjectTraits::CreateAttributes> {};

template <typename ObjectTraits>
struct AdapterHostKeyWarmbootRecoverable : std::true_type {};

} // namespace facebook::fboss
