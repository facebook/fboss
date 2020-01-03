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

#include <memory>

#include <folly/FBString.h>
#include <folly/IntrusiveList.h>

namespace facebook::fboss {

class SwitchState;

/*
 * StateUpdate objects are used to make changes to the SwitchState.
 *
 * All updates are applied in a single thread.  StateUpdate objects allow other
 * threads to instruct the update thread how to apply a particular update.
 *
 * Note that the update thread may choose to batch updates in some cases--if it
 * has multiple updates to apply it may run them all at once and only send a
 * single update notification to the HwSwitch and other update subscribers.
 * Therefore the applyUpdate() may be called with an unpublished SwitchState in
 * some cases.
 */
class StateUpdate {
 public:
  explicit StateUpdate(folly::StringPiece name, bool allowCoalesce = true)
      : name_(name.str()), allowCoalesce_(allowCoalesce) {}
  virtual ~StateUpdate() {}

  const std::string& getName() const {
    return name_;
  }

  bool allowsCoalescing() const {
    return allowCoalesce_;
  }

  /*
   * Apply the update, and return a new SwitchState.
   *
   * This function should return null if the update does not result in any
   * changes to the state.  (This may occur in cases where the update would
   * have caused changes when it was first scheduled, but no longer results in
   * changes by the time it is actually applied.)
   */
  virtual std::shared_ptr<SwitchState> applyUpdate(
      const std::shared_ptr<SwitchState>& origState) = 0;

  /*
   * The onError() function will be called in the update thread if an error
   * occurs applying the state update to the SwSwitch state.
   *
   * In case the callback wishes to save the current exception as a
   * std::exception_ptr, the exception will also be available via
   * std::current_exception() when onError() is called.
   */
  virtual void onError(const std::exception& ex) noexcept = 0;

  /*
   * The onSuccess() function will be called in the update thread
   * after the state update has been successfully applied.
   *
   * It is called when it is applied successfully to the software switch state
   * (and not the hardware necessarily). This function indicates that SwSwitch
   * has taken the responsibility of applying this StateUpdate to the hardware,
   * and it will eventually apply it to the hardware. If there are errors, then
   * it will try and make sure it is eventually applied.
   */
  virtual void onSuccess() {}

 private:
  // Forbidden copy constructor and assignment operator
  StateUpdate(StateUpdate const&) = delete;
  StateUpdate& operator=(StateUpdate const&) = delete;

  std::string name_;
  bool allowCoalesce_;

  // An intrusive list hook for maintaining the list of pending updates.
  folly::IntrusiveListHook listHook_;
  // The SwSwitch code needs access to our listHook_ member so it can maintain
  // the update list.
  friend class SwSwitch;
};

} // namespace facebook::fboss
