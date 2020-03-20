add_executable(store_test
    fboss/agent/test/oss/Main.cpp
    fboss/agent/hw/sai/store/tests/SaiEmptyStoreTest.cpp
    fboss/agent/hw/sai/store/tests/BridgeStoreTest.cpp
    fboss/agent/hw/sai/store/tests/FdbStoreTest.cpp
    fboss/agent/hw/sai/store/tests/HashStoreTest.cpp
    fboss/agent/hw/sai/store/tests/HostifTrapStoreTest.cpp
    fboss/agent/hw/sai/store/tests/InSegStoreTest.cpp
    fboss/agent/hw/sai/store/tests/NeighborStoreTest.cpp
    fboss/agent/hw/sai/store/tests/NextHopStoreTest.cpp
    fboss/agent/hw/sai/store/tests/NextHopGroupStoreTest.cpp
    fboss/agent/hw/sai/store/tests/PortStoreTest.cpp
    fboss/agent/hw/sai/store/tests/QueueStoreTest.cpp
    fboss/agent/hw/sai/store/tests/RouteStoreTest.cpp
    fboss/agent/hw/sai/store/tests/RouterInterfaceStoreTest.cpp
    fboss/agent/hw/sai/store/tests/SchedulerStoreTest.cpp
    fboss/agent/hw/sai/store/tests/VlanStoreTest.cpp
)

target_link_libraries(store_test
    sai_store
    fake_sai
    ${GTEST}
    ${LIBGMOCK_LIBRARIES}
)

set_target_properties(store_test PROPERTIES COMPILE_FLAGS
  "-DSAI_VER_MAJOR=${SAI_VER_MAJOR} \
  -DSAI_VER_MINOR=${SAI_VER_MINOR}  \
  -DSAI_VER_RELEASE=${SAI_VER_RELEASE}"
)

gtest_discover_tests(store_test)
