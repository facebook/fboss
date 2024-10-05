load("@fbcode_macros//build_defs:auto_headers.bzl", "AutoHeaders")
load("@fbcode_macros//build_defs:cpp_library.bzl", "cpp_library")
load(
    "//fboss:THIRD-PARTY-VERSIONS.bzl",
    "NATIVE_IMPLS",
    "SAI_BRCM_IMPLS",
    "SAI_CREDO_IMPLS",
    "SAI_FAKE_IMPLS",
    "SAI_IMPLS",
    "SAI_LEABA_IMPLS",
    "SAI_PHY_IMPLS",
    "SAI_VENDOR_IMPLS",
    "to_impl_suffix",
    "to_versions",
)

# TODO: re-exporting since this lib has a bunch of deps. Will remove deps and directly ref THIRD-PARTY-VERSIONS
NATIVE_IMPLS = NATIVE_IMPLS
SAI_BRCM_IMPLS = SAI_BRCM_IMPLS
SAI_CREDO_IMPLS = SAI_CREDO_IMPLS
SAI_FAKE_IMPLS = SAI_FAKE_IMPLS
SAI_IMPLS = SAI_IMPLS
SAI_LEABA_IMPLS = SAI_LEABA_IMPLS
SAI_VENDOR_IMPLS = SAI_VENDOR_IMPLS
SAI_PHY_IMPLS = SAI_PHY_IMPLS
to_versions = to_versions
to_sai_versions = to_versions
to_impl_suffix = to_impl_suffix
to_sdk_suffix = to_impl_suffix

# TODO: reuse to_impl_suffix here as well (missing a dash)
def to_impl_lib_name(sai_impl):
    return "{}-{}".format(
        sai_impl.name,
        sai_impl.version,
    )

def to_impl_bin_name(sai_impl, prefix):
    return "{}-{}-{}".format(
        prefix,
        sai_impl.name,
        sai_impl.version,
    )

def impl_category_suffix(is_npu):
    return "" if is_npu else "-phy"

def to_impl_external_deps(sai_impl):
    _external_deps_map = {
        ("brcm-sai", "8.2.0.0_odp"): [("brcm-sai", None, "sai")],
        ("brcm-sai", "8.2.0.0_sim_odp"): [("brcm-sai", None, "sai")],
        ("brcm-sai", "9.2.0.0_odp"): [("brcm-sai", None, "sai")],
        ("brcm-sai", "9.0_ea_sim_odp"): [("brcm-sai", None, "sai")],
        ("brcm-sai", "10.0_ea_odp"): [("brcm-sai", None, "sai")],
        ("brcm-sai", "10.0_ea_sim_odp"): [("brcm-sai", None, "sai")],
        ("brcm-sai", "10.0_ea_dnx_sim_odp"): [("brcm-sai", None, "sai")],
        ("brcm-sai", "10.2.0.0_odp"): [("brcm-sai", None, "sai")],
        ("brcm-sai", "11.0_ea_odp"): [("brcm-sai", None, "sai")],
        ("brcm-sai", "11.3.0.0_odp"): [("brcm-sai", None, "sai")],
        ("brcm-sai", "11.3.0.0_dnx_odp"): [("brcm-sai", None, "sai")],
        ("brcm-sai", "12.0_ea_dnx_odp"): [("brcm-sai", None, "sai")],
        ("CredoB52SAI", "0.7.2"): [("CredoB52SAI", None, "saiowl")],
        ("CredoB52SAI", "0.8.4"): [("CredoB52SAI", None, "saiowl")],
        ("fake", "1.14.0"): [("sai", None)],
        ("leaba-sdk", "1.42.8"): [("leaba-sdk", None, "sai-sdk")],
        ("leaba-sdk", "24.4.90"): [("leaba-sdk", None, "dyn-sai"), ("leaba-sdk", None, "dyn-sdk")],
        ("leaba-sdk", "24.4.90_yuba"): [("leaba-sdk", None, "sai-sdk")],
        ("leaba-sdk", "24.6.1_yuba"): [("leaba-sdk", None, "sai-sdk")],
        ("leaba-sdk", "24.7.0_yuba"): [("leaba-sdk", None, "sai-sdk")],
        ("leaba-sdk", "24.8.3001"): [("leaba-sdk", None, "dyn-sai"), ("leaba-sdk", None, "dyn-sdk")],
    }
    return _external_deps_map[(sai_impl.sdk_name, sai_impl.version)]

def _get_fake_impls():
    fake_impls = []
    for fake_impl in SAI_FAKE_IMPLS:
        fake_impls.extend([fake_impl])
    return fake_impls

def get_all_impls():
    all_impls = []
    all_impls.extend(_get_fake_impls())
    for sai_impl in SAI_VENDOR_IMPLS:
        all_impls.extend([sai_impl])
    return all_impls

def get_all_npu_impls():
    return _get_fake_impls() + [
        sai_impl
        for sai_impl in SAI_BRCM_IMPLS + SAI_LEABA_IMPLS
    ]

def get_all_phy_impls():
    return [sai_impl for sai_impl in SAI_PHY_IMPLS] + _get_fake_impls()

def get_all_native_phy_impls():
    return [impl for impl in NATIVE_IMPLS]

def get_all_impls_for(npu):
    return get_all_npu_impls() if npu else get_all_phy_impls()

def sai_fake_impl_lib():
    return [cpp_library(
        name = to_impl_lib_name(sai_impl),
        srcs = [
        ],
        auto_headers = AutoHeaders.SOURCES,
        exported_deps = [
            "//fboss/agent/hw/sai/fake:fake_sai",
        ],
    ) for sai_impl in SAI_FAKE_IMPLS]

def sai_leaba_impl_lib():
    return [cpp_library(
        name = to_impl_lib_name(sai_impl),
        srcs = [
        ],
        auto_headers = AutoHeaders.SOURCES,
        linker_flags = [
            # When we use the leaba python debug shell, it will dynamically
            # load a shared library built with SWIG. We need that library
            # to consider the symbols it depends on resolved so that it
            # doesn't load them again.
            # see:
            # https://ftp.gnu.org/old-gnu/Manuals/ld-2.9.1/html_node/ld_3.html
            # for more details
            "--export-dynamic",
        ],
        exported_deps = [
            "//fboss/agent/hw/sai/tracer:sai_tracer{}".format(to_impl_suffix(sai_impl)),
        ],
    ) for sai_impl in SAI_LEABA_IMPLS]

def get_link_group_map(binary_name, sai_impl):
    if sai_impl.is_dyn:
        shared_libs = []
        for (_, _, libname) in to_impl_external_deps(sai_impl):
            shared_libs.append(("fbcode//third-party-buck/platform010-compat/build/{}/{}:{}".format(sai_impl.name, sai_impl.version, libname), "node", None, "shared"))
        return [
            (
                binary_name,
                shared_libs,
            ),
        ]
    else:
        return []

def sai_brcm_impl_lib():
    return [cpp_library(
        name = to_impl_lib_name(sai_impl),
        srcs = [
        ],
        linker_flags = [
            # Brcm SDK ISSU needs these flags when linking with DLL library
            "--export-dynamic",
        ],
        exported_deps = [
            "//fboss/agent/hw/sai/tracer:sai_tracer{}".format(to_impl_suffix(sai_impl)),
        ],
        auto_headers = AutoHeaders.SOURCES,
        versions = to_versions(sai_impl),
    ) for sai_impl in SAI_BRCM_IMPLS]

def sai_credo_impl_lib():
    return [cpp_library(
        name = to_impl_lib_name(sai_impl),
        srcs = [
        ],
        link_whole = True,
        exported_deps = [
            "//fboss/agent/hw/sai/tracer:sai_tracer{}".format(to_impl_suffix(sai_impl)),
        ],
        auto_headers = AutoHeaders.SOURCES,
    ) for sai_impl in SAI_CREDO_IMPLS]

def sai_impl_libs():
    sai_fake_impl_lib()
    sai_leaba_impl_lib()
    sai_brcm_impl_lib()
    sai_credo_impl_lib()

def _sai_impl_version_lib(sai_impl, srcs = []):
    external_deps = to_impl_external_deps(sai_impl)
    if not srcs:
        srcs = ["util.cpp", "facebook/{}/version.cpp".format(sai_impl.name)]
    return cpp_library(
        name = "{}-version".format(to_impl_lib_name(sai_impl)),
        srcs = srcs,
        auto_headers = AutoHeaders.SOURCES,
        exported_deps = [
            "fbsource//third-party/fmt:fmt",
            ":impl-version",
        ],
        exported_external_deps = external_deps + ["re2", "glog"],
    )

def sai_impl_version_libs():
    cpp_library(
        name = "impl-version",
        srcs = [
        ],
        headers = [
            "util.h",
            "version.h",
        ],
    )
    for sai_impl in SAI_IMPLS:
        if (sai_impl.name == "fake"):
            _sai_impl_version_lib(sai_impl, ["version.cpp"])
        else:
            _sai_impl_version_lib(sai_impl, [])
