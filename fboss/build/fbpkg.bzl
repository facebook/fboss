load("//antlir/bzl:structs.bzl", "structs")
load("//fbpkg:fbpkg.bzl", "fbpkg")

# Helpers to standardize our fbpkg structures

# For each buck mode, creates an fbpkg target <name>.<mode>. Opt packages will not have the mode appended,
# as they are assumed default. Restricting to add debuginfo only for opt packages to save on build times
def fboss_fbpkg_builder(name, buck_modes = None, split_debuginfo = True, buck_opts = None, **kwargs):
    kwargs.setdefault("expire_days", 20)
    buck_opts_dict = structs.to_dict(buck_opts) if buck_opts else {}
    if not buck_modes:
        buck_modes = ["opt"]
    for mode in buck_modes:
        buck_opts_dict["mode"] = mode
        is_opt = mode == "opt"
        full_name = name if is_opt else "{}.{}".format(name, mode)
        fbpkg.builder(
            name = full_name,
            buck_opts = struct(**buck_opts_dict),
            split_debuginfo = split_debuginfo,
            **kwargs
        )
