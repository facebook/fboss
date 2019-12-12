# CMake to build SAI libraries and binaries

add_library(fake_sai
    fboss/agent/hw/sai/fake/FakeSai.cpp
    fboss/agent/hw/sai/fake/FakeSaiBridge.cpp
    fboss/agent/hw/sai/fake/FakeSaiFdb.cpp
    fboss/agent/hw/sai/fake/FakeSaiHostif.cpp
    fboss/agent/hw/sai/fake/FakeSaiInSegEntry.cpp
    fboss/agent/hw/sai/fake/FakeSaiInSegEntryManager.cpp
    fboss/agent/hw/sai/fake/FakeSaiNeighbor.cpp
    fboss/agent/hw/sai/fake/FakeSaiNextHop.cpp
    fboss/agent/hw/sai/fake/FakeSaiNextHopGroup.cpp
    fboss/agent/hw/sai/fake/FakeSaiObject.cpp
    fboss/agent/hw/sai/fake/FakeSaiPort.cpp
    fboss/agent/hw/sai/fake/FakeSaiQueue.cpp
    fboss/agent/hw/sai/fake/FakeSaiRoute.cpp
    fboss/agent/hw/sai/fake/FakeSaiRouterInterface.cpp
    fboss/agent/hw/sai/fake/FakeSaiScheduler.cpp
    fboss/agent/hw/sai/fake/FakeSaiSwitch.cpp
    fboss/agent/hw/sai/fake/FakeSaiVirtualRouter.cpp
    fboss/agent/hw/sai/fake/FakeSaiVlan.cpp
)

target_link_libraries(fake_sai Folly::folly)

target_compile_definitions(fake_sai PRIVATE
    SAI_VER_MAJOR=1
    SAI_VER_MINOR=4
    SAI_VER_RELEASE=0
)

add_library(address_util
    fboss/agent/hw/sai/api/AddressUtil.cpp
)

target_link_libraries(address_util
    Folly::folly
)

target_link_libraries(fake_sai
    address_util
)

install(TARGETS fake_sai)
