// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once

#include <boost/circular_buffer.hpp>
#include <folly/Synchronized.h>
#include <chrono>
#include <limits>
#include <numeric>

namespace facebook::fboss {
/*
 * The purpose of this class is to provide a small footprint
 * structure that can record the max and min values over a
 * timed interval. This class uses a bucketing method, where
 * the structure is a buffer of buckets, where each bucket holds
 * the max and min values for an interval in the bucket. The structure
 * allows for configuration of the length of time to record over,
 * and the granularity of the data recorded. This structure's functions
 * are all thread safe, and are intended to be used for data logging.
 */
template <class ValueType>
class TimeSeriesWithMinMax {
 public:
  /*
   * Unit definitions for readable interface.
   */
  using Time = std::chrono::time_point<std::chrono::system_clock>;
  using Duration = std::chrono::seconds;

  /*
   * Instantiate a time series.
   * interval : Length of time to record max over.
   * bucketInterval : The granularity of the data.
   * The class relies on the buffer's buckets being sorted
   * by time covered.
   */
  explicit TimeSeriesWithMinMax(
      Duration interval = Duration(60),
      Duration bucketInterval = Duration(1))
      : interval_(interval), bucketInterval_(bucketInterval) {
    assert(interval.count() >= bucketInterval.count());
    assert(bucketInterval.count() > 0);
    assert(interval.count() > 0);
    buf_ = boost::circular_buffer<Bucket>(
        interval.count() / bucketInterval_.count());
  }

  /*
   * Add a value into the buffer, at the current time.
   */
  void addValue(const ValueType& value);

  /*
   * Add a value into the bucket at a specified time
   */
  void addValue(const ValueType& value, Time t);

  /*
   * Get the current maximum value of the buffer.
   */
  ValueType getMax();

  /*
   * Get the maximum value over an interval
   */
  ValueType getMax(Time start, Time end);

  /*
   * Get the current minimum value of the buffer.
   */
  ValueType getMin();

  /*
   * Get the minimum value over an interval
   */
  ValueType getMin(Time start, Time end);

 private:
  /*
   * Internal bucket class to keep granularity of data.
   */
  class Bucket {
   public:
    /*
     * Construct a Bucket object.
     */
    Bucket(Time now, Duration width);

    /*
     * Add a value into a bucket.
     */
    void addValue(const ValueType& value);

    /*
     * Field querying methods.
     */
    bool isTimeInBucket(Time time);

    Time getInstantiatedTime() const;

    ValueType getMax() const {
      return max_;
    }

    ValueType getMin() const {
      return min_;
    }

   private:
    ValueType max_ = std::numeric_limits<ValueType>::lowest();
    ValueType min_ = std::numeric_limits<ValueType>::max();
    Time startTime_;
    Duration width_;
  };

  /*
   * Internal method to restrict buffer length, and remove buckets
   * from outside of the time window.
   */
  void maintainBuffer(Time now);

  /*
   * Local copies of constructor arguments.
   */
  Duration interval_;
  Duration bucketInterval_;

  /*
   * Vector structure to hold buckets of data. Using folly
   * Synchronized provides the thread-safety needed.
   */
  folly::Synchronized<boost::circular_buffer<Bucket>> buf_;
};

} // namespace facebook::fboss
#include "TimeSeriesWithMinMax-inl.h"
