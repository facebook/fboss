/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#pragma once

#include <boost/core/noncopyable.hpp>

#include <folly/SpinLock.h>
#include <folly/Synchronized.h>
#include <map>
#include <string>

namespace facebook::fboss {

class L2Entry;
enum class L2EntryUpdateType;

class L2LearnEventObserverIf : public boost::noncopyable {
 public:
  virtual ~L2LearnEventObserverIf() {}
  void handleL2LearnEvent(
      const L2Entry& l2entry,
      const L2EntryUpdateType& l2EntryUpdateType) noexcept {
    l2LearningUpdateReceived(l2entry, l2EntryUpdateType);
  }

 private:
  virtual void l2LearningUpdateReceived(
      const L2Entry& l2entry,
      const L2EntryUpdateType& l2EntryUpdateType) noexcept = 0;
};

class L2LearnEventObservers : public boost::noncopyable {
 public:
  // EventObserver() noexcept {}
  void registerL2LearnEventObserver(
      L2LearnEventObserverIf* observer,
      const std::string& name);
  void unregisterL2LearnEventObserver(
      L2LearnEventObserverIf* observer,
      const std::string& name);

  void l2LearnEventReceived(
      const L2Entry& l2entry,
      const L2EntryUpdateType& l2EntryUpdateType);

 private:
  folly::Synchronized<std::map<std::string, L2LearnEventObserverIf*>>
      l2LearnEventObservers_;
};

} // namespace facebook::fboss
