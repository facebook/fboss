include(fboss/agent/hw/mlnx/CMakeLists.txt)

add_executable(mlnx_agent
  fboss/agent/platforms/mlnx/mlnx_agent.cpp
)

target_link_libraries(mlnx_agent
  fboss_agent
)

