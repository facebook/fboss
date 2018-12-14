#include "RouteApi.h"

#include <boost/functional/hash.hpp>

#include <functional>

namespace std {
size_t hash<facebook::fboss::RouteTypes::RouteEntry>::operator()(
    const facebook::fboss::RouteTypes::RouteEntry& r) const {
  size_t seed = 0;
  boost::hash_combine(seed, r.switchId());
  boost::hash_combine(seed, r.virtualRouterId());
  boost::hash_combine(seed, std::hash<folly::CIDRNetwork>()(r.destination()));
  return seed;
}
} // namespace std
