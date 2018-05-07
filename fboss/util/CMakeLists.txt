# TODO(rsher)
# depends on agent/hw/bcm/facebook/bcm_config.thrift
# investigate open sourcing
# add_executable(lldp_tool
#       fboss/util/lldp_tool.cpp
# )
#
# target_link_libraries(lldp_tool fboss-agent)

add_executable(cp2112_util
    fboss/util/cp2112_util.cpp
)
target_link_libraries(cp2112_util fboss_agent)

add_executable(wedge_qsfp_util
    fboss/util/wedge_qsfp_util.cpp
    fboss/util/oss/wedge_qsfp_util.cpp
)
target_link_libraries(wedge_qsfp_util fboss_agent)
