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
struct ObjectPublisherAttributes {
  static_assert(
      IsObjectPublisher<ObjectTraits>::value,
      "Object is not publisher");

  using PublisherAttributes = typename ObjectTraits::PublisherAttributes;
  using AdapterHostKey = typename ObjectTraits::AdapterHostKey;
  using CreateAttributes = typename ObjectTraits::CreateAttributes;

  static constexpr auto isPublisherAttributesAdapterHostKey = std::is_same_v<
      typename ObjectTraits::PublisherAttributes,
      typename ObjectTraits::AdapterHostKey>;

  static constexpr auto isPublisherAttributesCreateAttributes = std::is_same_v<
      typename ObjectTraits::PublisherAttributes,
      typename ObjectTraits::CreateAttributes>;

  static PublisherAttributes get(
      const AdapterHostKey& adapterHostKey,
      const CreateAttributes& createAttributes) {
    if constexpr (isPublisherAttributesAdapterHostKey) {
      return adapterHostKey;
    } else if constexpr (isPublisherAttributesCreateAttributes) {
      return createAttributes;
    } else {
      static_assert(
          !isPublisherAttributesCreateAttributes &&
              !isPublisherAttributesAdapterHostKey,
          "published attribute is neither adapter host key nor create attributes");
    }
  }
};
} // namespace fboss
} // namespace facebook
