// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include "folly/String.h"

namespace facebook::fboss {

enum AggregateOpEnum { SUM, COUNT, MIN, MAX, AVG };

/* Note that we are returning double from all these accumulate methods.
This is fine for now because the result of all supported aggregation
operations can be expressed as a double. Later on, if we want to support
something like a string concatenation aggregate, we would have to change the
return type to string and do appropriate casts. Since we don't see the need for
that usecase right now, we have traded-off some generalization for performance
beenfits. */

template <typename ExpectedType>
class SumAgg {
 public:
  ExpectedType newAccumValue;
  double accumulate(
      const ExpectedType& newValue,
      const ExpectedType& oldAccumValue) {
    newAccumValue = oldAccumValue + newValue;
    if constexpr (std::is_same<decltype(newAccumValue), double>::value) {
      return newAccumValue;
    } else {
      double convertedNewAccumValue;
      convertedNewAccumValue = folly::tryTo<double>(newAccumValue).value();
      return convertedNewAccumValue;
    }
  }
};

template <typename ExpectedType>
class MinAgg {
 public:
  ExpectedType newAccumValue;
  double accumulate(
      const ExpectedType& newValue,
      const ExpectedType& oldAccumValue) {
    newAccumValue = std::min(oldAccumValue, newValue);
    if constexpr (std::is_same<decltype(newAccumValue), double>::value) {
      return newAccumValue;
    } else {
      double convertedNewAccumValue;
      convertedNewAccumValue = folly::tryTo<double>(newAccumValue).value();
      return convertedNewAccumValue;
    }
  }
};

template <typename ExpectedType>
class MaxAgg {
 public:
  ExpectedType newAccumValue;
  double accumulate(
      const ExpectedType& newValue,
      const ExpectedType& oldAccumValue) {
    newAccumValue = std::max(oldAccumValue, newValue);
    if constexpr (std::is_same<decltype(newAccumValue), double>::value) {
      return newAccumValue;
    } else {
      double convertedNewAccumValue;
      convertedNewAccumValue = folly::tryTo<double>(newAccumValue).value();
      return convertedNewAccumValue;
    }
  }
};

template <typename ExpectedType>
class CountAgg {
 public:
  double newAccumValue;
  double accumulate(const ExpectedType& /*newValue*/, double& oldAccumValue) {
    newAccumValue = oldAccumValue + 1;
    return newAccumValue;
  }
};

template <typename ExpectedType>
class AvgAgg {
 public:
  ExpectedType accumulatedSum, accumulatedCount;

  // TODO(surabhi236): This will eventually be removed. We will perform average
  // aggregation using sum and count instead of doing a rolling aggregate.
  double accumulate(
      const ExpectedType& newValue,
      const ExpectedType& oldAccumValue) {
    return 0;
  }
};

} // namespace facebook::fboss
