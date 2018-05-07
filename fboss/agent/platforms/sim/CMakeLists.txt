include(fboss/agent/hw/sim/CMakeLists.txt)

add_executable(sim_agent
  fboss/agent/platforms/sim/sim_ctrl.cpp
)

target_link_libraries(sim_agent
  fboss_agent
)
