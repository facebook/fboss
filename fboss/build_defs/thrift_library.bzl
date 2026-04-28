"""Thrift library rule for FBOSS Bazel build.

Compiles .thrift files to C++ using the fbthrift compiler and produces
a cc_library with the generated code.
"""

load("@rules_cc//cc:defs.bzl", "cc_library")

# Thrift compiler from pre-built fbthrift (declared as a Bazel label so
# genrules properly depend on the fbthrift repository rule).
_THRIFT_COMPILER_LABEL = "@fbthrift//:bin/thrift1"

# Include paths for the thrift compiler
_THRIFT_INCLUDES = [
    "/var/FBOSS/fboss",
    "/var/FBOSS/tmp_bld_dir/installed/fbthrift/include",
]

def _thrift_gen_type_files(name):
    """Return the type-related files generated for a .thrift file."""
    prefix = "gen-cpp2/" + name
    return [
        prefix + "_constants.cpp",
        prefix + "_constants.h",
        prefix + "_data.cpp",
        prefix + "_data.h",
        prefix + "_for_each_field.h",
        prefix + "_metadata.cpp",
        prefix + "_metadata.h",
        prefix + "_types.cpp",
        prefix + "_types.h",
        prefix + "_types.tcc",
        prefix + "_types_fwd.h",
        prefix + "_types_custom_protocol.h",
        prefix + "_types_binary.cpp",
        prefix + "_types_compact.cpp",
        prefix + "_types_serialization.cpp",
        prefix + "_visit_by_thrift_field_metadata.h",
        prefix + "_visit_union.h",
        prefix + "_visitation.h",
        prefix + "_sinit.cpp",
        prefix + "_clients.h",
        prefix + "_clients_fwd.h",
        prefix + "_handlers.h",
    ]

def _thrift_gen_service_files(service_name):
    """Return the service-related files generated for a thrift service."""
    prefix = "gen-cpp2/" + service_name
    return [
        prefix + ".cpp",
        prefix + ".h",
        prefix + ".tcc",
        prefix + "AsyncClient.cpp",
        prefix + "AsyncClient.h",
        prefix + "_custom_protocol.h",
        prefix + "_processmap_binary.cpp",
        prefix + "_processmap_compact.cpp",
    ]

def fboss_thrift_library(
        name,
        thrift_srcs,
        services = [],
        deps = [],
        thrift_cpp2_options = "",
        visibility = None):
    """Generate a C++ library from .thrift files.

    Args:
        name: Target name
        thrift_srcs: Either a list of .thrift files (no services) or a dict
            mapping .thrift files to lists of service names, e.g.:
            {"ctrl.thrift": ["FbossCtrl"], "config.thrift": []}
        services: (deprecated) Flat list of services, only used with list-form thrift_srcs
        deps: Dependencies (other thrift libraries or cc_libraries)
        thrift_cpp2_options: Comma-separated options for mstch_cpp2 generator
        visibility: Visibility specification
    """
    gen_rules = []
    cc_srcs = []

    # Normalize thrift_srcs to dict form: {"file.thrift": [services]}
    if type(thrift_srcs) == type({}):
        srcs_dict = thrift_srcs
    else:
        # List form -- pair with flat services list (all services on first file)
        srcs_dict = {}
        for i, tf in enumerate(thrift_srcs):
            srcs_dict[tf] = services if i == 0 else []

    for thrift_file, file_services in srcs_dict.items():
        base = thrift_file.replace(".thrift", "")
        gen_name = name + "_" + base + "_gen"

        # Collect output files: types for this file + services defined in it
        outputs = _thrift_gen_type_files(base)
        for svc in file_services:
            outputs.extend(_thrift_gen_service_files(svc))

        gen_options = thrift_cpp2_options if thrift_cpp2_options else "json"
        include_flags = " ".join(["-I " + p for p in _THRIFT_INCLUDES])

        cmd = (
            "$(location " + _THRIFT_COMPILER_LABEL + ")" +
            " --gen mstch_cpp2:" + gen_options +
            " " + include_flags +
            " -o $(RULEDIR)" +
            " $(location " + thrift_file + ")"
        )

        native.genrule(
            name = gen_name,
            srcs = [thrift_file],
            outs = outputs,
            cmd = cmd,
            tools = [_THRIFT_COMPILER_LABEL],
            visibility = ["//visibility:private"],
        )

        gen_rules.append(gen_name)
        cc_srcs.append(":" + gen_name)

    # Add fbthrift and folly runtime deps, avoiding duplicates
    all_deps = list(deps)
    for d in ["@fbthrift//:fbthrift", "@folly//:folly"]:
        if d not in all_deps:
            all_deps.append(d)

    cc_library(
        name = name,
        srcs = cc_srcs,
        deps = all_deps,
        copts = [
            "-I$(GENDIR)",
        ],
        visibility = visibility or ["//visibility:public"],
    )
