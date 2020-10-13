# CMake to build libraries and binaries in fboss/agent/hw/sai/tracer

# In general, libraries and binaries in fboss/foo/bar are built by
# cmake/FooBar.cmake

add_library(sai_tracer
  fboss/agent/hw/sai/tracer/AclApiTracer.cpp
  fboss/agent/hw/sai/tracer/BridgeApiTracer.cpp
  fboss/agent/hw/sai/tracer/BufferApiTracer.cpp
  fboss/agent/hw/sai/tracer/FdbApiTracer.cpp
  fboss/agent/hw/sai/tracer/HashApiTracer.cpp
  fboss/agent/hw/sai/tracer/HostifApiTracer.cpp
  fboss/agent/hw/sai/tracer/MplsApiTracer.cpp
  fboss/agent/hw/sai/tracer/NeighborApiTracer.cpp
  fboss/agent/hw/sai/tracer/NextHopApiTracer.cpp
  fboss/agent/hw/sai/tracer/NextHopGroupApiTracer.cpp
  fboss/agent/hw/sai/tracer/PortApiTracer.cpp
  fboss/agent/hw/sai/tracer/QosMapApiTracer.cpp
  fboss/agent/hw/sai/tracer/QueueApiTracer.cpp
  fboss/agent/hw/sai/tracer/RouteApiTracer.cpp
  fboss/agent/hw/sai/tracer/RouterInterfaceApiTracer.cpp
  fboss/agent/hw/sai/tracer/SaiTracer.cpp
  fboss/agent/hw/sai/tracer/SchedulerApiTracer.cpp
  fboss/agent/hw/sai/tracer/SwitchApiTracer.cpp
  fboss/agent/hw/sai/tracer/TamApiTracer.cpp
  fboss/agent/hw/sai/tracer/Utils.cpp
  fboss/agent/hw/sai/tracer/VirtualRouterApiTracer.cpp
  fboss/agent/hw/sai/tracer/VlanApiTracer.cpp
)

target_link_libraries(sai_tracer
  fboss_error
  async_logger
  sai_version
  Folly::folly
)

set_target_properties(sai_tracer PROPERTIES COMPILE_FLAGS
  "-DSAI_VER_MAJOR=${SAI_VER_MAJOR} \
  -DSAI_VER_MINOR=${SAI_VER_MINOR}  \
  -DSAI_VER_RELEASE=${SAI_VER_RELEASE}"
)

target_link_options(sai_tracer
  PUBLIC
  "LINKER:-wrap,sai_api_query"
  "LINKER:-wrap,sai_api_initialize"
)
