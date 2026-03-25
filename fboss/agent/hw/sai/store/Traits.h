// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once

namespace facebook::fboss {

/* if an object is not publisher, i.e. no other object depends on this object */
template <typename>
struct IsObjectPublisher : std::false_type {};

namespace detail {
// Primary template: Attr differs from AdapterHostKey means custom publisher key
template <typename ObjectTraits, typename Attr>
struct PublisherKeyInternal {
  using type = Attr;
  using custom_type = std::conditional_t<
      std::is_same_v<Attr, typename ObjectTraits::AdapterHostKey>,
      std::monostate,
      Attr>;
};

// Specialization for non-publishers (Attr = void)
template <typename ObjectTraits>
struct PublisherKeyInternal<ObjectTraits, void> {
  using type = void;
  using custom_type = std::monostate;
};
} // namespace detail

template <typename T, typename = void>
struct PublisherKey : detail::PublisherKeyInternal<T, void> {};

template <typename ObjectTraits>
struct AdapterHostKeyWarmbootRecoverable : std::true_type {};

} // namespace facebook::fboss
