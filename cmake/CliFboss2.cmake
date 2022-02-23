# CMake to build libraries and binaries in fboss/cli/fboss2

# In general, libraries and binaries in fboss/foo/bar are built by
# cmake/FooBar.cmake

add_fbthrift_cpp_library(
  show_acl_model
  fboss/cli/fboss2/commands/show/acl/model.thrift
  OPTIONS
    json
)

add_fbthrift_cpp_library(
  show_arp_model
  fboss/cli/fboss2/commands/show/arp/model.thrift
  OPTIONS
    json
)

add_fbthrift_cpp_library(
  show_lldp_model
  fboss/cli/fboss2/commands/show/lldp/model.thrift
  OPTIONS
    json
)

add_fbthrift_cpp_library(
  show_ndp_model
  fboss/cli/fboss2/commands/show/ndp/model.thrift
  OPTIONS
    json
)

add_fbthrift_cpp_library(
  show_port_model
  fboss/cli/fboss2/commands/show/port/model.thrift
  OPTIONS
    json
)

add_fbthrift_cpp_library(
  show_transceiver_model
  fboss/cli/fboss2/commands/show/transceiver/model.thrift
  OPTIONS
    json
)

add_fbthrift_cpp_library(
  show_interface_flaps
  fboss/cli/fboss2/commands/show/interface/flaps/model.thrift
  OPTIONS
    json
)

add_fbthrift_cpp_library(
  show_interface_errors
  fboss/cli/fboss2/commands/show/interface/errors/model.thrift
  OPTIONS
    json
)

add_fbthrift_cpp_library(
  show_interface_counters
  fboss/cli/fboss2/commands/show/interface/counters/model.thrift
  OPTIONS
    json
)

add_fbthrift_cpp_library(
  show_interface_traffic
  fboss/cli/fboss2/commands/show/interface/traffic/model.thrift
  OPTIONS
    json
)

add_fbthrift_cpp_library(
  show_interface_counters_mka
  fboss/cli/fboss2/commands/show/interface/counters/mka/model.thrift
  OPTIONS
    json
  DEPENDS
    hardware_stats_cpp2
)

add_fbthrift_cpp_library(
  show_interface_status
  fboss/cli/fboss2/commands/show/interface/status/model.thrift
  OPTIONS
    json
)

add_fbthrift_cpp_library(
  show_interface_phymap
  fboss/cli/fboss2/commands/show/interface/phymap/model.thrift
  OPTIONS
    json
  DEPENDS
    mka_structs_cpp2
)
find_package(CLI11 CONFIG REQUIRED)

add_executable(fboss2
  fboss/cli/fboss2/commands/clear/CmdClearArp.h
  fboss/cli/fboss2/commands/clear/CmdClearInterfaceCounters.h
  fboss/cli/fboss2/commands/clear/CmdClearNdp.h
  fboss/cli/fboss2/CmdGlobalOptions.cpp
  fboss/cli/fboss2/CmdHandler.cpp
  fboss/cli/fboss2/CmdArgsLists.cpp
  fboss/cli/fboss2/CmdList.cpp
  fboss/cli/fboss2/commands/show/acl/CmdShowAcl.h
  fboss/cli/fboss2/commands/show/arp/CmdShowArp.h
  fboss/cli/fboss2/commands/show/lldp/CmdShowLldp.h
  fboss/cli/fboss2/commands/show/ndp/CmdShowNdp.h
  fboss/cli/fboss2/commands/show/port/CmdShowPort.h
  fboss/cli/fboss2/commands/show/port/CmdShowPortQueue.h
  fboss/cli/fboss2/commands/show/interface/CmdShowInterface.h
  fboss/cli/fboss2/commands/show/interface/flaps/CmdShowInterfaceFlaps.h
  fboss/cli/fboss2/commands/show/interface/errors/CmdShowInterfaceErrors.h
  fboss/cli/fboss2/commands/show/interface/counters/CmdShowInterfaceCounters.h
  fboss/cli/fboss2/commands/show/interface/traffic/CmdShowInterfaceTraffic.h
  fboss/cli/fboss2/commands/show/interface/counters/mka/CmdShowInterfaceCountersMKA.h
  fboss/cli/fboss2/commands/show/interface/phymap/CmdShowInterfacePhymap.h
  fboss/cli/fboss2/commands/show/interface/status/CmdShowInterfaceStatus.h
  fboss/cli/fboss2/commands/show/transceiver/CmdShowTransceiver.h
  fboss/cli/fboss2/CmdSubcommands.cpp
  fboss/cli/fboss2/Main.cpp
  fboss/cli/fboss2/oss/CmdGlobalOptions.cpp
  fboss/cli/fboss2/oss/CmdList.cpp
  fboss/cli/fboss2/utils/CmdUtils.cpp
  fboss/cli/fboss2/utils/CLIParserUtils.cpp
  fboss/cli/fboss2/utils/CmdClientUtils.cpp
  fboss/cli/fboss2/utils/Table.cpp
  fboss/cli/fboss2/utils/HostInfo.h
  fboss/cli/fboss2/utils/oss/CmdClientUtils.cpp
  fboss/cli/fboss2/utils/oss/CmdUtils.cpp
  fboss/cli/fboss2/utils/oss/CLIParserUtils.cpp
  fboss/cli/fboss2/options/OutputFormat.h
  fboss/cli/fboss2/options/SSLPolicy.h
)

target_link_libraries(fboss2
  CLI11::CLI11
  tabulate
  fb303_cpp2
  ctrl_cpp2
  qsfp_cpp2
  phy_cpp2
  hardware_stats_cpp2
  mka_structs_cpp2
  Folly::folly
  show_acl_model
  show_arp_model
  show_lldp_model
  show_ndp_model
  show_port_model
  show_transceiver_model
  show_interface_flaps
  show_interface_errors
  show_interface_counters
  show_interface_counters_mka
  show_interface_traffic
  show_interface_status
  show_interface_phymap
  ${RE2}
)

add_library(tabulate
  fboss/cli/fboss2/tabulate/asciidoc_exporter.hpp
  fboss/cli/fboss2/tabulate/cell.hpp
  fboss/cli/fboss2/tabulate/color.hpp
  fboss/cli/fboss2/tabulate/column.hpp
  fboss/cli/fboss2/tabulate/column_format.hpp
  fboss/cli/fboss2/tabulate/exporter.hpp
  fboss/cli/fboss2/tabulate/font_align.hpp
  fboss/cli/fboss2/tabulate/font_style.hpp
  fboss/cli/fboss2/tabulate/format.hpp
  fboss/cli/fboss2/tabulate/latex_exporter.hpp
  fboss/cli/fboss2/tabulate/markdown_exporter.hpp
  fboss/cli/fboss2/tabulate/optional_lite.hpp
  fboss/cli/fboss2/tabulate/printer.hpp
  fboss/cli/fboss2/tabulate/row.hpp
  fboss/cli/fboss2/tabulate/table.hpp
  fboss/cli/fboss2/tabulate/table_internal.hpp
  fboss/cli/fboss2/tabulate/tabulate.hpp
  fboss/cli/fboss2/tabulate/termcolor.hpp
  fboss/cli/fboss2/tabulate/utf8.hpp
  fboss/cli/fboss2/tabulate/variant_lite.hpp
)

set_target_properties(tabulate PROPERTIES LINKER_LANGUAGE CXX)

install(TARGETS fboss2)
