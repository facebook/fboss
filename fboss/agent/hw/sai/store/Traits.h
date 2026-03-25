// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once

namespace facebook::fboss {

/* if an object is not publisher, i.e. no other object depends on this object */
template <typename>
struct IsObjectPublisher : std::false_type {};

template <typename T, typename = void>
struct PublisherKey {
  using type = void;
  using custom_type = std::monostate;
};

template <typename ObjectTraits>
struct AdapterHostKeyWarmbootRecoverable : std::true_type {};

} // namespace facebook::fboss
