# CMake to build libraries and binaries for agent link testing

add_executable(bcm_link_test
  fboss/agent/test/link_tests/BcmLinkTests.cpp
)

target_link_libraries(bcm_link_test
  link_tests
  platform
  bcm
  bcm_ecmp_utils
  bcm_packet_trap_helper
  bcm_qos_utils
)
