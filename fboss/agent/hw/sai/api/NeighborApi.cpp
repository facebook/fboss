#include "NeighborApi.h"

#include <boost/functional/hash.hpp>

#include <functional>

namespace std {
size_t hash<facebook::fboss::NeighborTypes::NeighborEntry>::operator()(
    const facebook::fboss::NeighborTypes::NeighborEntry& n) const {
  size_t seed = 0;
  boost::hash_combine(seed, n.switchId());
  boost::hash_combine(seed, n.routerInterfaceId());
  boost::hash_combine(seed, std::hash<folly::IPAddress>()(n.ip()));
  return seed;
}
} // namespace std
