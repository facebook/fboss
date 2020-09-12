# CMake to build libraries and binaries in fboss/agent/hw/sai/fake

# In general, libraries and binaries in fboss/foo/bar are built by
# cmake/FooBar.cmake

add_library(fake_sai
    fboss/agent/hw/sai/fake/FakeSai.cpp
    fboss/agent/hw/sai/fake/FakeSaiAcl.cpp
    fboss/agent/hw/sai/fake/FakeSaiBridge.cpp
    fboss/agent/hw/sai/fake/FakeSaiBuffer.cpp
    fboss/agent/hw/sai/fake/FakeSaiDebugCounter.cpp
    fboss/agent/hw/sai/fake/FakeSaiFdb.cpp
    fboss/agent/hw/sai/fake/FakeSaiHash.cpp
    fboss/agent/hw/sai/fake/FakeSaiHostif.cpp
    fboss/agent/hw/sai/fake/FakeSaiInSegEntry.cpp
    fboss/agent/hw/sai/fake/FakeSaiInSegEntryManager.cpp
    fboss/agent/hw/sai/fake/FakeSaiNeighbor.cpp
    fboss/agent/hw/sai/fake/FakeSaiNextHop.cpp
    fboss/agent/hw/sai/fake/FakeSaiNextHopGroup.cpp
    fboss/agent/hw/sai/fake/FakeSaiObject.cpp
    fboss/agent/hw/sai/fake/FakeSaiPort.cpp
    fboss/agent/hw/sai/fake/FakeSaiQosMap.cpp
    fboss/agent/hw/sai/fake/FakeSaiQueue.cpp
    fboss/agent/hw/sai/fake/FakeSaiRoute.cpp
    fboss/agent/hw/sai/fake/FakeSaiRouterInterface.cpp
    fboss/agent/hw/sai/fake/FakeSaiScheduler.cpp
    fboss/agent/hw/sai/fake/FakeSaiSwitch.cpp
    fboss/agent/hw/sai/fake/FakeSaiVirtualRouter.cpp
    fboss/agent/hw/sai/fake/FakeSaiVlan.cpp
    fboss/agent/hw/sai/fake/FakeSaiWred.cpp
 )

target_link_libraries(fake_sai
  sai_api
  address_util
  sai_version
  sai_fake_extensions
  Folly::folly
)

set_target_properties(fake_sai PROPERTIES COMPILE_FLAGS
  "-DSAI_VER_MAJOR=${SAI_VER_MAJOR} \
  -DSAI_VER_MINOR=${SAI_VER_MINOR} \
  -DSAI_VER_RELEASE=${SAI_VER_RELEASE}"
)
