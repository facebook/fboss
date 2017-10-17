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

#include <folly/io/async/AsyncTimeout.h>

#include <boost/container/flat_map.hpp>

#include "fboss/agent/LacpTypes.h"
#include "fboss/agent/state/AggregatePort.h"
#include "fboss/agent/types.h"

#include <iosfwd>

namespace facebook {
namespace fboss {

class LacpController;
class AggregatePort;
class SwSwitch;
class SwitchState;

/*
 * See IEEE 802.3AD-2000 43.4.3 for an overview of each state machine
 */

class ReceiveMachine : private folly::AsyncTimeout {
 public:
  explicit ReceiveMachine(LacpController& controller, folly::EventBase* evb);
  ~ReceiveMachine() override;

  // thread-safe
  void start();
  void stop();

  // External events
  void rx(LACPDU lacpdu);
  void portUp();
  void portDown();

  // Used indirecty by the Transmission Machine to construct a LACPDU
  ParticipantInfo partnerInfo() const;

 private:
  enum class ReceiveState {
    INITIALIZED,
    CURRENT,
    EXPIRED,
    DEFAULTED,
    DISABLED
  };
  friend std::ostream& operator<<(
      std::ostream& out,
      ReceiveMachine::ReceiveState s);
  friend void toAppend(ReceiveMachine::ReceiveState state, std::string* result);

  // States
  void initialize();
  void expired();
  void defaulted();
  void disabled();
  void current(LACPDU lacpdu);

  // Timer-related
  void timeoutExpired() noexcept override;
  void startNextEpoch(std::chrono::seconds duration);
  void endThisEpoch();

  // Helpers
  void recordDefault();
  void recordPDU(LACPDU& lacpdu);
  void updateSelected(LACPDU& lacpdu);
  bool updateNTT(LACPDU& lacpdu);
  std::chrono::seconds epochDuration();
  void updateState(ReceiveState nextState);

  static const std::chrono::seconds SLOW_EPOCH_DURATION;
  static const std::chrono::seconds FAST_EPOCH_DURATION;

  ReceiveState state_{ReceiveState::INITIALIZED};
  ReceiveState prevState_{ReceiveState::INITIALIZED};
  ParticipantInfo partnerInfo_{LacpState::NONE}; // operational

  LacpController& controller_;
};
void toAppend(ReceiveMachine::ReceiveState state, std::string* result);
std::ostream& operator<<(std::ostream& out, ReceiveMachine::ReceiveState s);

class PeriodicTransmissionMachine : private folly::AsyncTimeout {
 public:
  explicit PeriodicTransmissionMachine(
      LacpController& controller,
      folly::EventBase* evb);
  ~PeriodicTransmissionMachine() override;

  void portUp();
  void portDown();

  // thread-safe
  void start();
  void stop();

 private:
  enum class PeriodicState { NONE, SLOW, FAST, TX };
  friend void toAppend(
      PeriodicTransmissionMachine::PeriodicState state,
      std::string* result);

  void timeoutExpired() noexcept override;
  void beginNextPeriod();
  PeriodicState determineTransmissionRate();

  static const std::chrono::seconds SHORT_PERIOD;
  static const std::chrono::seconds LONG_PERIOD;

  PeriodicState state_{PeriodicState::NONE};
  LacpController& controller_;
};
void toAppend(
    PeriodicTransmissionMachine::PeriodicState state,
    std::string* result);

class TransmitMachine : private folly::AsyncTimeout {
 public:
  TransmitMachine(
      LacpController& controller,
      folly::EventBase* evb,
      SwSwitch* sw);
  ~TransmitMachine() override;

  void ntt(LACPDU lacpdu);

  void start();
  void stop();

 private:
  enum class PeriodicState { NONE, SLOW, FAST, TX };

  void timeoutExpired() noexcept override;
  void replenishTranmissionsLeft() noexcept;

  static const int MAX_TRANSMISSIONS_IN_SHORT_PERIOD;
  static const std::chrono::seconds TX_REPLENISH_RATE;

  int transmissionsLeft_{MAX_TRANSMISSIONS_IN_SHORT_PERIOD};
  LacpController& controller_;
  SwSwitch* sw_;
};

class MuxMachine : private folly::AsyncTimeout {
 public:
  MuxMachine(LacpController& controller, folly::EventBase* evb, SwSwitch* sw);
  ~MuxMachine() override;

  void start();
  void stop();

  void selected(AggregatePortID selection);
  void unselected();
  void matched();
  void notMatched();

 private:
  enum class MuxState {
    DETACHED,
    WAITING,
    ATTACHED,
    COLLECTING_DISTRIBUTING, // coupled control
  };
  friend void toAppend(MuxMachine::MuxState state, std::string* result);
  friend std::ostream& operator<<(std::ostream& out, MuxMachine::MuxState s);

  // states
  void detached();
  void waiting();
  void attached();
  void collectingDistributing();

  void enableCollectingDistributing() const;
  void disableCollectingDistributing() const;

  void timeoutExpired() noexcept override;

  void updateState(MuxState nextState);

  MuxState state_{MuxState::DETACHED};
  MuxState prevState_{MuxState::DETACHED};
  bool matched_{false};
  AggregatePortID selection_;
  LacpController& controller_;
  SwSwitch* sw_{nullptr};

  static const std::chrono::seconds AGGREGATE_WAIT_DURATION;
};
void toAppend(MuxMachine::MuxState state, std::string* result);
std::ostream& operator<<(std::ostream& out, MuxMachine::MuxState s);

class Selector {
 public:
  explicit Selector(LacpController& controller);

  void start();
  void stop();

  void select();
  AggregatePortID selection();

  using PortIDToLagID =
      boost::container::flat_map<PortID, LinkAggregationGroupID>;
  static PortIDToLagID& portToLag();

 private:
  bool isAvailable(AggregatePortID);

  LacpController& controller_;
};
} // namespace fboss
} // namespace facebook
