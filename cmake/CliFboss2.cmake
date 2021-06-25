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
  show_transceiver_model
  fboss/cli/fboss2/commands/show/transceiver/model.thrift
  OPTIONS
    json
)

add_executable(fboss2
  fboss/cli/fboss2/commands/clear/CmdClearArp.h
  fboss/cli/fboss2/commands/clear/CmdClearNdp.h
  fboss/cli/fboss2/CmdGlobalOptions.cpp
  fboss/cli/fboss2/CmdHandler.cpp
  fboss/cli/fboss2/CmdList.cpp
  fboss/cli/fboss2/commands/show/acl/CmdShowAcl.h
  fboss/cli/fboss2/commands/show/arp/CmdShowArp.h
  fboss/cli/fboss2/commands/show/lldp/CmdShowLldp.h
  fboss/cli/fboss2/commands/show/ndp/CmdShowNdp.h
  fboss/cli/fboss2/commands/show/CmdShowPort.h
  fboss/cli/fboss2/commands/show/CmdShowPortQueue.h
  fboss/cli/fboss2/commands/show/CmdShowInterface.h
  fboss/cli/fboss2/commands/show/transceiver/CmdShowTransceiver.h
  fboss/cli/fboss2/CmdSubcommands.cpp
  fboss/cli/fboss2/Main.cpp
  fboss/cli/fboss2/oss/CmdGlobalOptions.cpp
  fboss/cli/fboss2/oss/CmdList.cpp
  fboss/cli/fboss2/utils/CmdUtils.cpp
  fboss/cli/fboss2/utils/CmdClientUtils.cpp
  fboss/cli/fboss2/utils/oss/CmdClientUtils.cpp
  fboss/cli/fboss2/utils/oss/CmdUtils.cpp
  fboss/cli/fboss2/options/SSLPolicy.h
)

target_link_libraries(fboss2
  CLI11
  tabulate
  ctrl_cpp2
  qsfp_cpp2
  Folly::folly
  show_acl_model
  show_arp_model
  show_lldp_model
  show_ndp_model
  show_transceiver_model
)

add_library(CLI11
  fboss/cli/fboss2/CLI11/App.hpp
  fboss/cli/fboss2/CLI11/CLI.hpp
  fboss/cli/fboss2/CLI11/Config.hpp
  fboss/cli/fboss2/CLI11/ConfigFwd.hpp
  fboss/cli/fboss2/CLI11/Error.hpp
  fboss/cli/fboss2/CLI11/Formatter.hpp
  fboss/cli/fboss2/CLI11/FormatterFwd.hpp
  fboss/cli/fboss2/CLI11/Macros.hpp
  fboss/cli/fboss2/CLI11/Option.hpp
  fboss/cli/fboss2/CLI11/Split.hpp
  fboss/cli/fboss2/CLI11/StringTools.hpp
  fboss/cli/fboss2/CLI11/Timer.hpp
  fboss/cli/fboss2/CLI11/TypeTools.hpp
  fboss/cli/fboss2/CLI11/Validators.hpp
  fboss/cli/fboss2/CLI11/Version.hpp
)

set_target_properties(CLI11 PROPERTIES LINKER_LANGUAGE CXX)

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
