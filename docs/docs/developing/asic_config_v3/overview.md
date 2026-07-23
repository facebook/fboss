---
sidebar_position: 1
---

# Overview

The `asic_config_v3` tool, located at `fboss/lib/asic_config_v3/`, generates
the per-platform ASIC configuration that a switch ASIC needs at SDK and SAI
initialization time (for example, the Broadcom YAML configuration). It is the
data-driven successor to the code-driven `asic_config_v2` implementation.
Whereas `asic_config_v2` required a dedicated Python module for every
platform, `asic_config_v3` describes each platform in JSON data files that are
processed by a generic generator shared by all platforms of an ASIC family. As
a result, adding support for a new platform on an existing ASIC family
requires no code changes.

## Design

### Input layers

The configuration input is split into layers according to who owns the data.
Each layer is a JSON file:

| Layer | File | Contents |
|---|---|---|
| OCP SAI common | `common/ocp_sai_common.json` | Vendor-agnostic SAI defaults shared by all vendors |
| Vendor family common | `vendors/<vendor>/<family>/sdk_common.json` and `sai_common.json` | SDK and SAI settings shared by all ASICs in a family (for example, Broadcom XGS) |
| Per-ASIC | `vendors/<vendor>/<family>/asics/<asic>.json` | Data intrinsic to the chip, such as the port architecture, output table names, global defaults, SAI overrides, pass-through data blocks, and ASIC-wide conditional settings |
| Platform | `platforms/<platform>/asic_config.json` | Board-specific data, such as the ASIC selection, port defaults, configuration variants, and platform-level SAI overrides |

Port wiring information (the lane map, polarity map, and logical-to-physical
port mapping) is not duplicated in these files. The generator computes it from
the platform data already maintained under `fboss/lib/platform_mapping_v2`.

### Generation pipeline

The entry point, `gen.py`, discovers every `platforms/*/asic_config.json`
file, looks up the appropriate generator in a registry keyed on the ASIC
vendor and ASIC names, and runs that generator once per
[variant](#platform-configuration-and-variants). For the Broadcom XGS family,
the generator applies the settings below in order. When two steps write the
same key of the same output table, the later step wins, so the list is also
the precedence order (lowest first). Steps 1 through 6 all build the `global`
table and therefore form a single priority chain; the remaining steps mostly
write to their own tables.

| Order | Setting | Source, in priority order | Output table |
|---|---|---|---|
| 1 | OCP SAI common | `common/ocp_sai_common.json` (`global`) | `global` |
| 2 | ASIC global defaults | `vendors/broadcom/xgs/asics/<asic>.json` (`global_defaults`) | `global` |
| 3 | Vendor SDK common | `vendors/broadcom/xgs/sdk_common.json` (`global`) | `global` |
| 4 | Vendor SAI common, minus keys listed in an active `skip_from_sai_common` | `vendors/broadcom/xgs/sai_common.json` (`global`) | `global` |
| 5 | ASIC SAI overrides | `vendors/broadcom/xgs/asics/<asic>.json` (`sai_overrides`) | `global` |
| 6 | Platform SAI overrides | `platforms/<platform>/asic_config.json` (`platform_sai_overrides`) | `global` |
| 7 | [Pass-through settings](#pass-through-settings) | `vendors/broadcom/xgs/asics/<asic>.json`, or `platforms/<platform>/asic_config.json` when the variant redefines a block | `PORT_CONFIG`, `FP_CONFIG`, `TM_THD_CONFIG`, `CTR_EFLEX_CONFIG`, `global` |
| 8 | [Conditional settings](#conditional-settings), ASIC entries then platform entries, each in array order | `vendors/broadcom/xgs/asics/<asic>.json` (`conditional_settings`), then `platforms/<platform>/asic_config.json` (`conditional_settings`) | tables named by each entry, for example `global`, `TM_THD_CONFIG`, `DEVICE_CONFIG` |
| 9 | Device configuration overrides | `platforms/<platform>/asic_config.json` (`device_config_overrides`) | `DEVICE_CONFIG` |
| 10 | Port mapping, port configuration, lane map, and polarity map | computed from `platform_mapping_v2`, the ASIC `port_architecture` and `mgmt_port_defaults`, and the platform `port_config`, `cpu_port`, `mgmt_port`, `mgmt_port_overrides`, and `port_mapping_overrides` | `PC_PM_CORE`, `PC_PORT_PHYS_MAP`, `PC_PORT`, `PORT` |

The ordering is the same for every platform and variant. Within steps 7 and
8, the entries of each list apply in array order.

### Schema validation

The platform, per-ASIC, and vendor common configuration files are validated
against the JSON Schemas stored in the `schemas/` directory
(`platform_config.schema.json`, `broadcom_xgs_asic_config.schema.json`, and
`vendor_common.schema.json`). The schemas reject unknown keys in most places,
so a new configuration key must be added to the corresponding schema, with a
description, in the same change. Run the validator with:

```shell
python3 fboss/lib/asic_config_v3/validation/validate_asic_configs.py
```

The schemas document every field and are the fastest way to look up valid keys
and values; the [Schema Field Reference](./schema_field_reference.md) page
summarizes the important ones. The schemas are read only by the validator,
never by the generator, so a schema change by itself never alters the
generated output.

### Running the generator

Run the helper script from the root of the repository:

```shell
./fboss/lib/asic_config_v3/run-helper.sh
```

Output files are written to
`fboss/lib/asic_config_v3/generated_asic_configs/`. To generate a single
platform, pass the `--platform <name>` argument.

## Configuration details

### Pass-through settings

Pass-through settings route a named data block from the ASIC file into an
output table declaratively, without any dedicated code:

```json
"pass_through_settings": [
  {"source": "flex_counter_settings", "target_table": "global"},
  {"source": "ctr_eflex_config", "target_table": "CTR_EFLEX_CONFIG"}
]
```

A platform variant may declare a block under the same source key to replace
the ASIC-level block entirely, which lets a platform emit different values
than the chip-wide defaults.

### Conditional settings

Conditional settings express feature toggles as data instead of code
branches. Each entry carries a name, a condition naming a parameter from the
variant's `asic_config_params` or `features` section together with the value
it must equal, and the settings to apply when the condition holds. Entries are
applied in array order, with later entries taking precedence, and ASIC-level
entries are evaluated before platform-level ones so that platform values win.
An ASIC-level entry may also list keys to suppress from the vendor SAI common
layer while its condition is active, which allows a feature to remove a
default setting rather than override it. The following entry from the
Tomahawk5 file illustrates the mechanism:

```json
{
  "name": "mmu_lossless",
  "description": "Enables MMU lossless mode for PFC and RDMA workloads.",
  "condition": {
    "source": "asic_config_params",
    "param": "mmu_lossless",
    "equals": true
  },
  "apply": {
    "global": {
      "sai_mmu_custom_config": 1,
      "sai_rdma_udf_disable": 1,
      "sai_l3_byte1_udf_disable": 1,
      "clm_enable": 1
    },
    "TM_THD_CONFIG": {
      "THRESHOLD_MODE": "LOSSY_AND_LOSSLESS",
      "SKIP_BUFFER_RESERVATION": 1
    }
  },
  "skip_from_sai_common": [
    "sai_mmu_qgroups_default",
    "sai_optimized_mmu"
  ]
}
```

### Platform configuration and variants

Each platform file declares the platform identity (`platform_name`, `vendor`,
and `asic`), which selects both the per-ASIC data file and the generator to
use. It then declares a `defaults` block together with one or more named
`variants`. A variant represents one deliverable configuration for the
platform, and the generator produces one output file per variant, named
`generated_asic_configs/<platform>_<variant>.yml`.

The effective configuration for a variant is formed by merging the variant's
settings on top of the `defaults` block. The merge is recursive for nested
objects. An object present in both places has its keys combined, with the
variant winning for any key declared on both sides. Scalar values and lists
are not combined; a variant that declares one replaces the default value
outright. This arrangement keeps shared settings in one place and lets each
variant declare only what makes it different. A platform with a single variant
typically keeps all of its settings in `defaults` and leaves the variant
entry empty.

A variant name is a free-form label chosen by the platform author; it is not
drawn from a fixed set and the generator does not interpret it. The names in
use today (`base`, `default`, `internal`, `rack`, `chassis`, `test_fixture`,
and so on) are identifiers only, and the name determines nothing beyond the
suffix of the output file. There is no need to reuse a name from another
platform, and adding a variant never requires a code change.

### Variant fields

A `defaults` block or a variant may declare any of the variant fields listed
in the [Schema Field Reference](./schema_field_reference.md). Every field is
optional, and a variant only needs to declare the fields that differ from the
platform defaults. A variant may also replace any data block that the ASIC
file routes through `pass_through_settings` by declaring a key of the same
name.

Because the generator does not act on the variant name, two variants produce
different output only through these fields. The parameters that switch
behavior on and off are `asic_config_params` and `features`. A
`conditional_settings` entry names one of their values as its condition and
applies extra settings when it matches, as described in
[Conditional settings](#conditional-settings). To give a variant new
behavior, set the relevant parameter or feature flag and add the matching
conditional setting; no new variant "type" or generator change is involved.
Note that of the `asic_config_params`, only `mmu_lossless` and `exact_match`
currently drive Broadcom XGS output; `config_gen_type` is accepted and
validated but is not yet consumed by any XGS conditional setting.

### Overriding a setting outside these fields

A setting that no variant field covers can usually still be overridden
without a code change:

- A key in the `global` output table can be set through
  `platform_sai_overrides`, which takes precedence over all the other layered
  `global` sources (steps 1 through 6 of the pipeline table).
- A key in the `DEVICE_CONFIG` table can be set through
  `device_config_overrides`.
- A table fed by a pass-through block can be replaced in its entirety by
  declaring the block at the variant level, as described in
  [Pass-through settings](#pass-through-settings).
- Any other output table the ASIC declares can be modified through a
  platform-level `conditional_settings` entry whose `apply` block names the
  table. The condition must reference a parameter in `asic_config_params` or a
  `features` flag; `asic_config_params` accepts platform-defined parameters,
  so a platform can introduce its own parameter and condition on it.

Two limitations apply. The generator emits only the tables named in the
ASIC's `table_names` list, so settings applied to any other table do not
appear in the output. In addition, a setting that requires computation or new
structure (for example, new port mapping logic) needs a generic, data-driven
extension of the generator and the schema rather than a platform-specific
branch in the code.

## Example: adding a platform for an existing ASIC family

No Python changes are required to add a platform on an ASIC family that the
tool already supports. Suppose a new Tomahawk5 board named `newboard` is being
added.

1. Ensure that `fboss/lib/platform_mapping_v2/platforms/newboard/` exists and
   contains the platform's static mapping and vendor data (see the
   [platform mapping documentation](../platform_mapping.md)).
2. Create `fboss/lib/asic_config_v3/platforms/newboard/asic_config.json`. The
   following is a complete example declaring a single variant; an existing
   platform on the same ASIC is a good starting template:

```json
{
  "platform_name": "newboard",
  "vendor": "broadcom",
  "asic": "tomahawk5",
  "num_ports_per_core": 2,
  "defaults": {
    "asic_config_params": {
      "config_type": "YAML_CONFIG",
      "exact_match": false,
      "mmu_lossless": false,
      "config_gen_type": "DEFAULT"
    },
    "port_config": {
      "default_speed": 400000,
      "speed_to_fec": {
        "100000": "PC_FEC_RS544",
        "200000": "PC_FEC_RS544_2XN",
        "400000": "PC_FEC_RS544_2XN",
        "800000": "PC_FEC_RS544_2XN"
      }
    },
    "cpu_port": {
      "speed": 10000,
      "num_lanes": 1
    },
    "mgmt_port": {
      "enabled": true,
      "speed_variants": {
        "100000": { "num_lanes": 4, "fec": "PC_FEC_RS528" }
      }
    },
    "features": {
      "generate_dlb_config": true,
      "generate_autoload_board_settings": true
    }
  },
  "variants": {
    "newvariant": {}
  }
}
```

3. Validate and generate:

```shell
python3 fboss/lib/asic_config_v3/validation/validate_asic_configs.py
./fboss/lib/asic_config_v3/run-helper.sh
```

The output appears as `generated_asic_configs/newboard_newvariant.yml`.

If the platform needs a setting that no listed field covers, see
[Overriding a setting outside these fields](#overriding-a-setting-outside-these-fields).

## Example: adding a variant

A variant is added by inserting one entry under `variants`; everything not
declared in it is inherited from `defaults`. Continuing the example above,
the following adds a `newvariant2` variant that enables MMU lossless mode
while the `newvariant` variant keeps the defaults:

```json
"variants": {
  "newvariant": {},
  "newvariant2": {
    "asic_config_params": {
      "mmu_lossless": true
    }
  }
}
```

The next generator run produces two output files, `newboard_newvariant.yml`
and `newboard_newvariant2.yml`. In the `newvariant2` output, `mmu_lossless`
satisfies the Tomahawk5 conditional setting of the same name, so the lossless MMU settings are
applied and the corresponding SAI common keys are suppressed; all other
settings are identical to `newvariant` because the merge with `defaults` fills
in every field the variant does not declare.

Another common pattern is a variant per hardware configuration, where each
variant reads its wiring data from a different `platform_mapping_v2`
directory:

```json
"variants": {
  "rack": {
    "platform_mapping_name": "newboard_rack"
  },
  "test_fixture": {
    "platform_mapping_name": "newboard_test_fixture"
  }
}
```

## Example: adding a new ASIC family and a platform for it

Supporting a new ASIC family (for example, a different vendor or a different
Broadcom product line) requires one new generator plus its data files.
Suppose a hypothetical Broadcom DNX family with a `jericho3` ASIC is being
added. The new files are:

```
fboss/lib/asic_config_v3/
├── vendors/broadcom/dnx/
│   ├── sdk_common.json                     # optional family-wide settings
│   └── asics/jericho3.json                 # chip-intrinsic data
├── platforms/newdnxboard/asic_config.json  # first platform on the family
├── generators/broadcom_dnx_generator.py    # the family generator
└── schemas/broadcom_dnx_asic_config.schema.json
```

1. **Vendor data.** `vendors/broadcom/dnx/asics/jericho3.json` holds the
   chip-intrinsic data the generator needs, such as the identity, port
   architecture, output table names, base settings, and conditional settings.
   Add family-wide common files only if several ASICs in the family will share
   those settings.
2. **Generator.** The generator subclasses `BaseAsicConfigGenerator` and
   implements two members:

```python
from fboss.lib.asic_config_v3.base_generator import BaseAsicConfigGenerator

class BroadcomDnxGenerator(BaseAsicConfigGenerator):
    @property
    def output_extension(self) -> str:
        return ".yml"

    def generate(self) -> str:
        # Load the vendor and ASIC JSON, layer the settings, compute
        # port wiring from platform_mapping_v2, and return the complete
        # output file as a string.
        ...
```

   Reuse the shared patterns where possible, such as layered settings,
   pass-through settings, conditional settings, and the platform mapping
   parser for wiring data. Keep the behavior data-driven and avoid
   per-platform branches.

3. **Registration.** Add the new vendor and ASIC pair to the registry in
   `gen.py`:

```python
_GENERATOR_REGISTRY = {
    ("broadcom", "tomahawk5"): BroadcomXgsGenerator,
    ("broadcom", "tomahawk6"): BroadcomXgsGenerator,
    ("broadcom", "jericho3"): BroadcomDnxGenerator,   # new entry
}
```

4. **Schema.** Add a schema for the new ASIC file under `schemas/` and
   register it in `validation/validate_asic_configs.py`. Extend
   `platform_config.schema.json` if the family introduces new platform-level
   fields.
5. **Build.** Add the new generator source files, and any new modules they
   import, to `cmake/AsicConfigV3ConfigCli.cmake` so that the build target
   includes them.
6. **Platform.** Add `platforms/newdnxboard/asic_config.json` exactly as in
   the first example, with `"vendor": "broadcom"` and `"asic": "jericho3"`
   selecting the new generator.
7. **Verification.** Compare the generated output against a known-good
   reference configuration for that family.
