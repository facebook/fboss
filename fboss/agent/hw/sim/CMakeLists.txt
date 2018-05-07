add_thrift_cpp2_library(
  sim_ctrl_cpp2
  fboss/agent/hw/sim/sim_ctrl.thrift
  SERVICES
    SimCtrl
  DEPENDS
    fboss_cpp2
    ctrl_cpp2
)

target_sources(fboss_agent
  PRIVATE
    fboss/agent/hw/sim/SimHandler.cpp
    fboss/agent/hw/sim/SimSwitch.cpp
    fboss/agent/hw/sim/SimPlatform.cpp
)

target_link_libraries(fboss_agent
  PRIVATE
    sim_ctrl_cpp2
)

