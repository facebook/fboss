// Copyright 2004-present Facebook. All Rights Reserved.
#include "TimedMaxBuffer.h"

#include <algorithm>

namespace facebook {
namespace fboss {

/*
 * Add value into bucket. This will place the value in the most
 * recent bucket if it is within the bucket's time frame, else
 * it will create a new bucket at the end of the vector, and place
 * the value inside it.
 */
template <class ValueType>
void TimedMaxBuffer<ValueType>::addValue(const ValueType& value) {
  auto t = std::chrono::system_clock::now();
  auto locked_buf = buf_.wlock();
  auto last_bucket = locked_buf->size() == 0
      ? nullptr
      : &locked_buf->at(locked_buf->size() - 1);
  if (last_bucket != nullptr && last_bucket->isTimeInBucket(t)) {
    last_bucket->addValue(value);
  } else {
    Bucket b(t, bucketInterval_);
    b.addValue(value);
    locked_buf->push_back(std::move(b));
  }
}

/*
 * If the time is specified for the addition, then more
 * caution is required to maintain the integrity of the
 * structure.
 */
template <class ValueType>
void TimedMaxBuffer<ValueType>::addValue(
    const ValueType& value,
    typename TimedMaxBuffer<ValueType>::Time t) {
  auto locked_buf = buf_.wlock();
  /*
   * If the time is out of the buffer, return.
   */
  if (t < std::chrono::system_clock::now() - interval_) {
    return;
  }

  /*
   * If the buffer is empty, add the first bucket to the
   * end of the buffer.
   */
  if (locked_buf->size() == 0) {
    Bucket b(t, bucketInterval_);
    b.addValue(value);
    locked_buf->push_back(std::move(b));
    return;
  }

  /*
   * Handle the case of the time being less than the earliest
   * time in the buffer.
   */
  if (t < locked_buf->at(0).getInstantiatedTime()) {
    Bucket b(t, bucketInterval_);
    b.addValue(value);
    locked_buf->insert(locked_buf->begin(), std::move(b));
    return;
  }

  /*
   * Handle the case of the time being greater than the newest
   * time in the buffer.
   */
  if (t > locked_buf->at(locked_buf->size() - 1).getInstantiatedTime() +
          bucketInterval_) {
    Bucket b(t, bucketInterval_);
    b.addValue(value);
    locked_buf->push_back(std::move(b));
    return;
  }

  /*
   * Handle the case when we have to add a bucket in the middle
   * of the buffer.
   */
  for (int i = 0; i < locked_buf->size(); i++) {
    Bucket& b = locked_buf->at(i);
    if (b.isTimeInBucket(t)) {
      b.addValue(value);
      return;
    }
    if (i + 1 < locked_buf->size()) {
      if (t < locked_buf->at(i + 1).getInstantiatedTime() &&
          t > b.getInstantiatedTime()) {
        Bucket b2(t, bucketInterval_);
        b2.addValue(value);
        locked_buf->insert(locked_buf->begin() + i, std::move(b2));
        return;
      }
    }
  }
}

/*
 * Maintains the buffer. Enforces that all buckets within the
 * buffer are within the appropriate time frame, and that the
 * size of the buffer is not longer than expected.
 */
template <class T>
void TimedMaxBuffer<T>::maintainBuffer(Time now) {
  auto locked_buf = buf_.wlock();
  // remove buckets that are out of date
  locked_buf->erase(
      std::remove_if(
          locked_buf->begin(),
          locked_buf->end(),
          [&](Bucket b) {
            return std::chrono::system_clock::now() - interval_ >=
                b.getInstantiatedTime();
          }),
      locked_buf->end());
}

/*
 * Return the maximum value in the buffer. Use the STL accumulate
 * function for readable code.
 */
template <class ValueType>
ValueType TimedMaxBuffer<ValueType>::getMax() {
  maintainBuffer(std::chrono::system_clock::now());
  auto locked_buf = buf_.rlock();
  if (locked_buf->size() == 0) {
    throw std::runtime_error("Empty Buffer!");
  }
  return std::accumulate(
      locked_buf->begin(),
      locked_buf->end(),
      std::numeric_limits<ValueType>::lowest(),
      [](ValueType b1, ValueType b2) { return std::max(b1, b2); });
}

template <class ValueType>
ValueType TimedMaxBuffer<ValueType>::getMax(
    typename TimedMaxBuffer<ValueType>::Time start,
    typename TimedMaxBuffer<ValueType>::Time end) {
  maintainBuffer(std::chrono::system_clock::now());
  auto locked_buf = buf_.rlock();

  if (locked_buf->size() == 0) {
    throw std::runtime_error("Empty Buffer!");
  }

  ValueType maxVal = std::numeric_limits<ValueType>::lowest();
  bool isValid = false;
  for (int i = 0; i < locked_buf->size(); i++) {
    auto b = &locked_buf->at(i);
    auto t = b->getInstantiatedTime();
    if (t >= start && t < end) {
      isValid = true;
      maxVal = std::max((ValueType)*b, maxVal);
    }
  }
  if (!isValid) {
    throw std::runtime_error("Bad range specified");
  }
  return maxVal;
}

/*
 * Create a Bucket object. This will create a bucket granular
 * to the bucket interval.
 */
template <class ValueType>
TimedMaxBuffer<ValueType>::Bucket::Bucket(Time now, Duration width)
    : width_(width) {
  auto epoch = std::chrono::duration_cast<Duration>(now.time_since_epoch());
  auto leftover = epoch % width;
  startTime_ = now - leftover;
}

/*
 * This function does not check that the value is
 * within the time expected for the buffer. For correct
 * usage, the user must check this for themselves.
 */
template <class ValueType>
void TimedMaxBuffer<ValueType>::Bucket::addValue(const ValueType& value) {
  max_ = std::max(value, max_);
}

template <class ValueType>
bool TimedMaxBuffer<ValueType>::Bucket::isTimeInBucket(Time time) {
  return time >= startTime_ && time <= startTime_ + width_;
}

template <class ValueType>
typename TimedMaxBuffer<ValueType>::Time
TimedMaxBuffer<ValueType>::Bucket::getInstantiatedTime() const{
  return startTime_;
}
}
}
