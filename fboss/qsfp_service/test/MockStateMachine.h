#pragma once

#include <boost/msm/back/state_machine.hpp>
#include <boost/msm/front/euml/euml.hpp>
#include <boost/msm/front/euml/state_grammar.hpp>
#include <folly/logging/xlog.h>
#include <thrift/lib/cpp/util/EnumUtils.h>
#include "fboss/qsfp_service/StateMachineController.h"

namespace facebook::fboss {

using boost::msm::front::euml::attributes_;
using boost::msm::front::euml::build_state;
using boost::msm::front::euml::no_action;

// Mock ID type for testing
enum class MockID : int {
  ID_1 = 1,
  ID_2 = 2,
};

// Mock event type for testing
enum class MockEvent : int {
  EVENT_1 = 1,
  EVENT_2 = 2,
  EVENT_3 = 3,
};

// Mock state type for testing
enum class MockState : int {
  STATE_1 = 1,
  STATE_2 = 2,
  STATE_3 = 3,
};

MockState getMockStateByOrder(int currentStateOrder);

// Mock attributes
BOOST_MSM_EUML_DECLARE_ATTRIBUTE(MockID, mockID)

// Define states for the state machine
BOOST_MSM_EUML_STATE((), State1)
BOOST_MSM_EUML_STATE((), State2)
BOOST_MSM_EUML_STATE((), State3)

// Define events for the state machine
BOOST_MSM_EUML_EVENT(Event1)
BOOST_MSM_EUML_EVENT(Event2)
BOOST_MSM_EUML_EVENT(Event3)

// Define the state machine
BOOST_MSM_EUML_TRANSITION_TABLE(
    (State1 + Event1 == State2,
     State2 + Event2 == State3,
     State3 + Event3 == State1),
    transition_table)

BOOST_MSM_EUML_DECLARE_STATE_MACHINE(
    (transition_table,
     init_ << State1,
     no_action,
     no_action,
     attributes_ << mockID),
    MockStateMachine)

} // namespace facebook::fboss
