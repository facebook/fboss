# CMake to build libraries and binaries in fboss/agent/hw/sai/tracer

# In general, libraries and binaries in fboss/foo/bar are built by
# cmake/FooBar.cmake

add_library(sai_tracer
  fboss/agent/hw/sai/tracer/AclApiTracer.cpp
  fboss/agent/hw/sai/tracer/BridgeApiTracer.cpp
  fboss/agent/hw/sai/tracer/PortApiTracer.cpp
  fboss/agent/hw/sai/tracer/QueueApiTracer.cpp
  fboss/agent/hw/sai/tracer/RouteApiTracer.cpp
  fboss/agent/hw/sai/tracer/SaiTracer.cpp
  fboss/agent/hw/sai/tracer/SwitchApiTracer.cpp
  fboss/agent/hw/sai/tracer/Utils.cpp
  fboss/agent/hw/sai/tracer/VlanApiTracer.cpp
)

target_link_libraries(sai_tracer
  fboss_error
  Folly::folly
)

target_link_options(sai_tracer
  PUBLIC
  "LINKER:-wrap,sai_api_query"
)
