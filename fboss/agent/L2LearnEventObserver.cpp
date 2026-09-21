/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/L2LearnEventObserver.h"
#include <folly/logging/xlog.h>
#include "fboss/agent/FbossError.h"

class L2Entry;
class L2EntryUpdateType;

namespace facebook::fboss {

void L2LearnEventObservers::l2LearnEventReceived(
    const L2Entry& l2entry,
    const L2EntryUpdateType& l2EntryUpdateType) {
  // notify about the l2 learn event received to the interested observers
  auto l2LearnEventObserversMap = l2LearnEventObservers_.rlock();
  for (auto& l2LearnEventObserver : *l2LearnEventObserversMap) {
    try {
      auto observer = l2LearnEventObserver.second;
      observer->handleL2LearnEvent(l2entry, l2EntryUpdateType);
    } catch (const std::exception& ex) {
      XLOG(FATAL) << "Error: " << folly::exceptionStr(ex)
                  << "in l2 learn event notification for : "
                  << l2LearnEventObserver.second;
    }
  }
}

void L2LearnEventObservers::registerL2LearnEventObserver(
    L2LearnEventObserverIf* observer,
    const std::string& name) {
  auto l2LearnEventObservers = l2LearnEventObservers_.wlock();
  if (l2LearnEventObservers->find(name) != l2LearnEventObservers->end()) {
    throw FbossError("Observer was already added: ", name);
  }
  XLOG(DBG2) << "Register: " << name << " as l2 learn event observer";
  l2LearnEventObservers->emplace(name, observer);
}

void L2LearnEventObservers::unregisterL2LearnEventObserver(
    L2LearnEventObserverIf* /*unused */,
    const std::string& name) {
  auto l2LearnEventObservers = l2LearnEventObservers_.wlock();
  if (!l2LearnEventObservers->erase(name)) {
    throw FbossError("Observer erase failed for:", name);
  }
  XLOG(DBG2) << "Unergister: " << name << " as l2 learn event observer";
}

} // namespace facebook::fboss
