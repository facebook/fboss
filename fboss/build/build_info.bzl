load("@fbcode_macros//build_defs:native_rules.bzl", "buck_genrule")

def _json_value(v):
    t = type(v)
    if t == str or t == "string":
        return '"' + v + '"'
    elif t == bool or t == "bool":
        return "true" if v else "false"
    elif t == int or t == "int":
        return str(v)
    else:
        fail("Unsupported JSON value type:", str(t))

def _dict_to_json(d):
    pairs = []
    for k in sorted(d.keys()):
        pairs.append('"' + k + '":' + _json_value(d[k]))
    return "{" + ",".join(pairs) + "}"

def get_sdk_build_info(sdk):
    """Build a dict of SDK metadata for the agent_build_info ELF section."""
    if not sdk:
        return {}
    info = {
        "sdk_is_dyn": sdk.is_dyn,
        "sdk_name": sdk.sdk_name,
        "sdk_product_name": sdk.name,
        "sdk_stage": sdk.stage,
        "sdk_version": sdk.version,
    }
    if sdk.sai_version:
        info["sai_version"] = sdk.sai_version
    if sdk.native_bcm_sdk_version:
        info["native_bcm_sdk_version"] = sdk.native_bcm_sdk_version
    if sdk.native_sdk_version:
        info["native_sdk_version"] = sdk.native_sdk_version
    return info

def gen_sdk_build_info_src(name, sdk):
    """Generate a .cpp file that embeds SDK metadata in an ELF section."""
    info = get_sdk_build_info(sdk)
    if not info:
        return
    json_str = _dict_to_json(info)
    content = '__attribute__((used, retain, section("agent_build_info"))) const char agent_build_info[] = R"json(' + json_str + ')json";'
    buck_genrule(
        name = name,
        out = name + ".cpp",
        cmd = "printf '%s\\n' '" + content + "' > \"$OUT\"",
    )
