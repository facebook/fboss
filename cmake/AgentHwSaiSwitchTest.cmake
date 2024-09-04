# CMake to build libraries and binaries in fboss/agent/hw/sai/switch/tests

# In general, libraries and binaries in fboss/foo/bar are built by
# cmake/FooBar.cmake

if(BUILD_SAI_FAKE)
add_executable(switch_test
    fboss/agent/test/oss/Main.cpp
    fboss/agent/hw/sai/switch/tests/AclTableGroupManagerTest.cpp
    fboss/agent/hw/sai/switch/tests/AclTableManagerTest.cpp
    fboss/agent/hw/sai/switch/tests/ArsManagerTest.cpp
    fboss/agent/hw/sai/switch/tests/ArsProfileManagerTest.cpp
    fboss/agent/hw/sai/switch/tests/BridgeManagerTest.cpp
    fboss/agent/hw/sai/switch/tests/FdbManagerTest.cpp
    fboss/agent/hw/sai/switch/tests/InSegEntryManagerTest.cpp
    fboss/agent/hw/sai/switch/tests/LagManagerTest.cpp
    fboss/agent/hw/sai/switch/tests/ManagerTestBase.cpp
    fboss/agent/hw/sai/switch/tests/MirrorManagerTest.cpp
    fboss/agent/hw/sai/switch/tests/NeighborManagerTest.cpp
    fboss/agent/hw/sai/switch/tests/NextHopGroupManagerTest.cpp
    fboss/agent/hw/sai/switch/tests/NextHopManagerTest.cpp
    fboss/agent/hw/sai/switch/tests/QosMapManagerTest.cpp
    fboss/agent/hw/sai/switch/tests/RouteManagerTest.cpp
    fboss/agent/hw/sai/switch/tests/RouterInterfaceManagerTest.cpp
    fboss/agent/hw/sai/switch/tests/SamplePacketManagerTest.cpp
    fboss/agent/hw/sai/switch/tests/SchedulerManagerTest.cpp
    fboss/agent/hw/sai/switch/tests/SwitchManagerTest.cpp
    fboss/agent/hw/sai/switch/tests/SystemPortManagerTest.cpp
    fboss/agent/hw/sai/switch/tests/UnsupportedFeatureTest.cpp
    fboss/agent/hw/sai/switch/tests/UdfManagerTest.cpp
    fboss/agent/hw/sai/switch/tests/VirtualRouterManagerTest.cpp
    fboss/agent/hw/sai/switch/tests/VlanManagerTest.cpp
    fboss/agent/hw/sai/switch/tests/TunnelManagerTest.cpp
)

target_link_libraries(switch_test
    agent_test_utils
    switchid_scope_resolver
    sai_platform
    sai_store
    sai_switch
    fake_sai
    hw_switch_fb303_stats
    manager_test_base
    ${GTEST}
    ${LIBGMOCK_LIBRARIES}
)

add_library(manager_test_base
  fboss/agent/hw/sai/switch/tests/ManagerTestBase.cpp
)

target_link_libraries(manager_test_base
  core
  switchid_scope_resolver
  sai_store
  sai_switch
  fake_sai
  Folly::folly
  ${GTEST}
  ${LIBGMOCK_LIBRARIES}
)

set_target_properties(manager_test_base PROPERTIES COMPILE_FLAGS
  "-DSAI_VER_MAJOR=${SAI_VER_MAJOR} \
  -DSAI_VER_MINOR=${SAI_VER_MINOR}  \
  -DSAI_VER_RELEASE=${SAI_VER_RELEASE}"
)

set_target_properties(switch_test PROPERTIES COMPILE_FLAGS
  "-DSAI_VER_MAJOR=${SAI_VER_MAJOR} \
  -DSAI_VER_MINOR=${SAI_VER_MINOR}  \
  -DSAI_VER_RELEASE=${SAI_VER_RELEASE}"
)

# switch_test can't run on devservers because of some library loading error,
# but gtest_discover_tests will attempt to execute the binary, so disable it.
if(NOT DEFINED ENV{DEVSERVER})
gtest_discover_tests(switch_test)
endif()

endif()
