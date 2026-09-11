---
sidebar_position: 1
---

# Overview

The `asic_config_v3` tool, located at `fboss/lib/asic_config_v3/`, reads inputs
from `fboss/configs/` and generates the per-platform ASIC configuration that a
switch ASIC needs at SDK and SAI initialization time. It is the data-driven
successor to the code-driven `asic_config_v2` implementation. Whereas
`asic_config_v2` required a dedicated Python module for every platform,
`asic_config_v3` describes each platform in JSON data files that are processed
by a generic generator shared by all platforms of an ASIC family. As a result,
adding support for a new platform on an existing ASIC family requires no code
changes.

Two ASIC families are currently supported, each with its own generator and
output format:

| Family | Generator | Output format | Output file |
|---|---|---|---|
| Broadcom XGS | `BroadcomXgsGenerator` | Multi-document YAML with typed tables such as `PC_PM_CORE`, `PC_PORT`, and `global` | `<platform>_<variant>.yml` |
| Broadcom DNX | `BroadcomDnxGenerator` | Flat SOC property key/value pairs in the JSON representation of the thrift `AsicConfig` struct | `<platform>_<variant>.json` |

The two ASIC families share the same framework. Platform discovery, variants,
schema validation, wiring data from `platform_mapping_v2`, and conditional
settings all work the same way in both. The ASIC families differ in how
settings are layered and in the structure of the output. The family-specific
sections below describe these differences.

## Design

### Input layers

The configuration input is split into layers according to who owns the data.
Each layer is a JSON file:

| Layer | Families | File | Contents |
|---|---|---|---|
| OCP SAI common | XGS | `fboss/configs/asic_vendors/common/ocp_sai_common.json` | Vendor-agnostic SAI defaults shared by all vendors |
| Vendor family common | XGS | `fboss/configs/asic_vendors/<vendor>/<family>/sdk_common.json` and `sai_common.json` | SDK and SAI settings shared by all ASICs in a family |
| Per-ASIC | XGS and DNX | `fboss/configs/asic_vendors/<vendor>/<family>/asics/<asic>.json` | Data intrinsic to the chip |
| Platform | XGS and DNX | `fboss/configs/platforms/<system_vendor>/<platform>/asic_config/asic_config.json` | Board-specific data, such as the ASIC selection, configuration variants, and platform-level overrides |

The DNX family has no family-wide common files; everything above the platform
layer lives in its per-ASIC file.

Port wiring information is not duplicated in the files listed above. The
generator derives the lane and polarity maps for both families, as well as the
XGS-only logical-to-physical port mapping, from the platform mapping data
maintained under
`fboss/configs/platforms/<system_vendor>/<platform>/platform_mapping/`.

What the per-ASIC file contains differs by family. For XGS it declares the
port architecture, output table names, global defaults, SAI overrides,
pass-through data blocks, and ASIC-wide conditional settings. For DNX it
declares the SOC key suffix and key format strings, the core types to include
in wiring generation, always-emitted base SDK settings, and declarative
tables. Both schemas are documented in the
[Schema Field Reference](./schema_field_reference.md).

### Generation pipeline

The entry point, `gen.py`, discovers every
`platforms/*/*/asic_config/asic_config.json`
file under `fboss/configs/`, looks up the appropriate generator in a registry
keyed on the ASIC vendor and ASIC names, and runs that generator once per
[variant](#platform-configuration-and-variants):

```python
_GENERATOR_REGISTRY = {
    ("broadcom", "tomahawk5"): BroadcomXgsGenerator,
    ("broadcom", "tomahawk6"): BroadcomXgsGenerator,
    ("broadcom", "jericho3"): BroadcomDnxGenerator,
}
```

Each family applies its input layers in a fixed order that is the same for
every platform and variant. When two steps write the same key, the later step
wins, so the step table in each family section below is also in ascending
precedence order.

#### Broadcom XGS

The XGS generator builds named YAML tables. Steps 1 through 6 all feed the
`global` table and form a single priority chain. The remaining steps mostly
write to their own tables.

| Order | Setting | Source, in priority order | Output table |
|---|---|---|---|
| 1 | OCP SAI common | `asic_vendors/common/ocp_sai_common.json` (`global`) | `global` |
| 2 | ASIC global defaults | `asic_vendors/broadcom/xgs/asics/<asic>.json` (`global_defaults`) | `global` |
| 3 | Vendor SDK common | `asic_vendors/broadcom/xgs/sdk_common.json` (`global`) | `global` |
| 4 | Vendor SAI common, minus keys listed in an active `skip_from_sai_common` | `asic_vendors/broadcom/xgs/sai_common.json` (`global`) | `global` |
| 5 | ASIC SAI overrides | `asic_vendors/broadcom/xgs/asics/<asic>.json` (`sai_overrides`) | `global` |
| 6 | Platform SAI overrides | `platforms/<system_vendor>/<platform>/asic_config/asic_config.json` (`platform_sai_overrides`) | `global` |
| 7 | [Pass-through settings](#pass-through-settings-xgs) | `asic_vendors/broadcom/xgs/asics/<asic>.json`, or `platforms/<system_vendor>/<platform>/asic_config/asic_config.json` when the variant redefines a block | `PORT_CONFIG`, `FP_CONFIG`, `TM_THD_CONFIG`, `CTR_EFLEX_CONFIG`, `global` |
| 8 | [Conditional settings](#conditional-settings), ASIC entries then platform entries, each in array order | `asic_vendors/broadcom/xgs/asics/<asic>.json` (`conditional_settings`), then `platforms/<system_vendor>/<platform>/asic_config/asic_config.json` (`conditional_settings`) | tables named by each entry, for example `global`, `TM_THD_CONFIG`, `DEVICE_CONFIG` |
| 9 | Device configuration overrides | `platforms/<system_vendor>/<platform>/asic_config/asic_config.json` (`device_config_overrides`) | `DEVICE_CONFIG` |
| 10 | Port mapping, port configuration, lane map, and polarity map | computed from `platform_mapping_v2`, the ASIC `port_architecture` and `mgmt_port_defaults`, and the platform `port_config`, `cpu_port`, `mgmt_port`, `mgmt_port_overrides`, and `port_mapping_overrides` | `PC_PM_CORE`, `PC_PORT_PHYS_MAP`, `PC_PORT`, `PORT` |

#### Broadcom DNX

The DNX generator builds a single flat map of SOC properties, each with a
string key and a string value. All settings are
written to the `common` section of the output, which is described in
[DNX output structure](#dnx-output-structure).

| Order | Setting | Source |
|---|---|---|
| 1 | Base SDK settings | `asic_vendors/broadcom/dnx/asics/<asic>.json` (`base_sdk_settings`) |
| 2 | [Declarative tables](#dnx-declarative-tables) for port speeds, TM port headers, DTM flow regions, and flow remote cores | `asic_vendors/broadcom/dnx/asics/<asic>.json` (`declarative_tables`) |
| 3 | Platform SDK overrides | `platforms/<system_vendor>/<platform>/asic_config/asic_config.json` (`platform_sdk_overrides`) |
| 4 | Lane and polarity keys | computed from `platform_mapping_v2` connections and the ASIC `core_types`, `key_formats`, and `num_lanes_per_core`, plus the ASIC `default_polarity_settings` |
| 5 | [Conditional settings](#conditional-settings), ASIC entries then platform entries, each in array order | `asic_vendors/broadcom/dnx/asics/<asic>.json` (`conditional_settings`), then `platforms/<system_vendor>/<platform>/asic_config/asic_config.json` (`conditional_settings`) |

The lane and polarity keys in step 4 reproduce the wiring that
`asic_config_v2` computed in per-platform Python code. The generator iterates
the platform's `platform_mapping_v2` connections and keeps the connection
ends whose core type the ASIC declares in `core_types`. For each remaining
connection it computes the SOC lane number as
`core_id * num_lanes_per_core + core_lane` and renders each key through the
ASIC's `key_formats` strings. Core types that are not declared, for example
the Jericho 3 recycle and eventor ports, are skipped automatically.

### Schema validation

The configuration files are validated against the JSON Schemas stored in the
`schemas/` directory (`platform_config.schema.json`,
`broadcom_xgs_asic_config.schema.json`, `broadcom_dnx_asic_config.schema.json`,
`conditional_setting.schema.json`, and `vendor_common.schema.json`). The
schemas reject unknown keys in most
places, so a new configuration key with a description must be added to the
corresponding schema in the same change. Run the validator with:

```shell
python3 -m fboss.lib.asic_config_v3.test.validate_asic_configs_schemas \
  --fboss-root "$PWD/fboss"
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

Output files are written beside each platform input under
`fboss/configs/platforms/<system_vendor>/<platform>/asic_config/generated/`,
one per platform variant.
The file extension is determined by the family, `.yml` for XGS and `.json`
for DNX, matching the output formats in the family table at the top of this
[page](#overview). To generate a single platform, pass the `--platform <name>`
argument.

### Verifying generated output

Generated output is verified by comparing it against known-good reference
configurations.

- The committed files under
  `fboss/configs/platforms/<system_vendor>/<platform>/asic_config/generated/`
  are the permanent regression references. Regenerating with an unchanged
  input configuration must reproduce them exactly.
- During the migration from `asic_config_v2`, a platform's output is
  additionally compared against the v2 references. The output must match the
  files under `fboss/lib/asic_config_v2/generated_asic_configs/` exactly,
  and the synced references under
  `fboss/lib/asic_config_v2/synced_asic_configs/` semantically.

## Configuration details

### Pass-through settings (XGS)

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

:::note

The DNX family does not use pass-through settings. Its output has no named
tables, and unconditional data blocks are expressed directly as
`base_sdk_settings`, `declarative_tables`, or `platform_sdk_overrides`.

:::

### Conditional settings

Conditional settings express feature toggles as data instead of code
branches. The mechanism is shared by both ASIC families: the condition
grammar is defined once in `schemas/conditional_setting.schema.json` and
evaluated by the base generator class. Each family's generator declares the
effects it can execute. Each entry carries a name, a condition, and one or
more effects. Unconditional data does not belong here; it has dedicated
mechanisms instead (pass-through settings on XGS; `base_sdk_settings` and
`platform_sdk_overrides` on DNX).

A condition names a parameter from the variant's `asic_config_params` or
`features` section and exactly one operator: `equals`, `not_equals`, `in`,
`not_in`, `starts_with`, or `not_starts_with`. A parameter the variant does
not declare evaluates as null, and each negative operator is the strict
complement of its positive counterpart, so an absent parameter satisfies
every negative operator.

An effect names what the entry does when its condition holds. The `apply`
effect writes settings to a named output target, which is an output table
for XGS and an output section for DNX. The `apply_from` effect copies a
named settings block from the ASIC file into an output target. The
`skip_from_sai_common` effect omits keys from the vendor SAI common layer
and is meaningful only for XGS, which is the only family that layers that
block. Validation is split between the schema and the generator. The schema
validator checks the structure of every entry, including operator and
operand types. Each generator additionally declares the effects it supports
(all three for XGS, and `apply` and `apply_from` for DNX) and, when it is
constructed,
rejects an entry that declares any other effect, has a malformed effect
payload, targets an unknown output table or section, or applies a value of
the wrong type for its output format. The generator check covers every
entry, not only those whose condition currently holds, so an invalid entry
cannot remain latent until another variant activates it. Operand types are
checked only by the schema validator, so rerun it after editing conditions.

Entries are evaluated once per variant. ASIC-level entries come before
platform-level entries, each in array order, so a platform value overrides
a chip-level one. Within an entry, `apply_from` executes before `apply`, so
an inline setting overrides the copied block. Each generator executes the
effects at the point in its pipeline where they belong: XGS collects
`skip_from_sai_common` before it layers the vendor SAI common block and
writes `apply` and `apply_from` settings in its conditional-settings step,
while DNX writes them in its final step.

The following XGS entry from the Tomahawk5 file illustrates the mechanism:

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

The `apply_from` effect serves the same purpose as `apply` but avoids
repeating a large settings block inside the entry: the entry names a
top-level block of the ASIC file and the output target to copy it into. The
following Tomahawk5 entry copies the chip's `dlb_defaults` block into the
`global` table when the variant enables the `generate_dlb_config` feature:

```json
"dlb_defaults": {
  "ecmp_dlb_port_speeds": 1,
  "l3_ecmp_member_secondary_mem_size": 4096
},
"conditional_settings": [
  {
    "name": "dlb_config",
    "description": "Enables Dynamic Load Balancing settings for ECMP groups.",
    "condition": {
      "source": "features",
      "param": "generate_dlb_config",
      "equals": true
    },
    "apply_from": {
      "source": "dlb_defaults",
      "target_table": "global"
    }
  }
]
```

Because `apply_from` executes before `apply` within an entry, an entry may
copy a block with `apply_from` and then override individual keys of that
block with an inline `apply`.

The following DNX entry from the meru800bia platform file selects the
single-stage port map for every port configuration that is not a dual-stage
topology:

```json
{
  "name": "single_stage_port_map",
  "condition": {
    "param": "port_config",
    "not_starts_with": "dual_stage"
  },
  "apply": {
    "common": {
      "fabric_connect_mode.BCM8889X": "FE",
      "ucode_port_0.BCM8889X": "CPU.0:core_0.0"
    }
  }
}
```

On DNX, scenario selection is expressed entirely through conditional settings
on the `asic_config_params` values. The `config_gen_type` parameter selects
the generation profile, such as production or hardware test. The
`port_config` parameter selects the port-map profile by exact name and the
topology family by prefix. The `multistage_role` parameter selects the
multistage fabric role. Because scenario selection happens through these
conditional settings, the bucketed `vendor_config.json` files that
`asic_config_v2` consumed through `platform_mapping_v2` are unnecessary;
`asic_config_v3` does not read them.

### Platform configuration and variants

Each platform file declares the platform identity (`platform_name`, `vendor`,
and `asic`), which selects both the per-ASIC data file and the generator to
use. It then declares a `defaults` block together with one or more named
`variants`. A variant represents one deliverable configuration for the
platform, and the generator produces one output file per variant, named
`generated/<platform>_<variant>.<extension>` beside the platform input.

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
platform defaults. Most variant fields belong to a single family. For
example, the `port_config` object, `cpu_port`, and `platform_sai_overrides`
are XGS fields, while `platform_sdk_overrides` and the `port_config`,
`multistage_role`, and `hyper_port` parameters inside `asic_config_params`
belong to DNX.

Because the generator does not act on the variant name, two variants produce
different output only through these fields. The parameters that switch
behavior on and off are `asic_config_params` and `features`. A
`conditional_settings` entry names one of their values as its condition and
applies extra settings when it matches, as described in
[Conditional settings](#conditional-settings). To give a variant new
behavior, set the relevant parameter or feature flag and add the matching
conditional setting; no new variant "type" or generator change is involved.
The `asic_config_params` that currently drive output through conditional
settings are `mmu_lossless`, `exact_match`, and `config_gen_type` on XGS, and
`config_gen_type`, `port_config`, and `multistage_role` on DNX.

### DNX output structure

The DNX output file is the JSON representation of the thrift `AsicConfig`
struct, which is defined in `fboss/agent/hw/config/asic_config_v2.thrift`.
The struct consists of a `common` entry holding a key/value `config` map,
together with an optional `npuEntries` map for platforms with more than one
NPU. The platform file declares which structure to produce through the
top-level `output_structure` field:

```json
"output_structure": {"mode": "common_only"}
```

- The `common_only` mode is for single-NPU platforms such as meru800bia.
  Every generated key is written to `common.config`. This is the only
  supported mode.
- Support for multi-NPU platforms, which distribute the per-chip wiring
  keys for lanes and polarity into per-NPU sections while the
  chip-independent settings remain in `common.config`, is planned as a
  further mode.

The number of NPUs is a property of the board rather than of the ASIC, which
is why `output_structure` is declared in the platform file and not in the
per-ASIC file.

### DNX ASIC data

The per-ASIC file, for example
`fboss/configs/asic_vendors/broadcom/dnx/asics/jericho3.json`,
declares the data intrinsic to the chip. It contains the following groups of
fields.

- **Key formats and suffixes.** The `asic_suffix` value, for example
  `BCM8889X`, is appended to generated SOC keys. The `key_formats` object
  holds the format strings for the lane map and polarity keys, with
  `{family}`, `{lane}`, and `{suffix}` placeholders. The `core_types` object
  maps each `platform_mapping_v2` core type the chip exposes, for example
  `J3_NIF` and `J3_FE`, to the key families used when rendering its wiring
  keys.
- **Base SDK settings.** The `base_sdk_settings` map holds the SDK settings
  emitted for every platform and variant of this chip. Keys carry their SOC
  suffix inline.
- **Default polarity settings.** The `default_polarity_settings` map holds
  the default polarity keys that carry no lane number. Jericho 3 emits
  them. An ASIC without such keys omits the field.
- **Declarative tables.** These are described in the next section.

#### DNX declarative tables

Declarative tables capture the fixed tables that `asic_config_v2` generated
in per-ASIC Python code:

| Table | Emits | Notes |
|---|---|---|
| `port_speed_map` | `port_init_speed_*` keys | Interface-type-to-speed entries, emitted verbatim. |
| `tm_port_header_map` | `tm_port_header_type_{in,out}_*` keys | Keyed by topology variant. The generator selects `dual_stage_3q_2q` when the variant's `port_config` starts with `dual_stage`, and `default` otherwise. |
| `dtm_flow_region_map` | `dtm_flow_mapping_mode_region_<N>` keys | Expanded from `key_format`, `start_region`, `count`, and `value`. |
| `flow_remote_cores` | `dtm_flow_nof_remote_cores_region*` keys | Emitted verbatim. |

### Overriding a setting outside these fields

A setting that no variant field covers can usually still be overridden
without a code change.

On XGS:

- A key in the `global` output table can be set through
  `platform_sai_overrides`, which takes precedence over all the other layered
  `global` sources (steps 1 through 6 of the XGS pipeline table).
- A key in the `DEVICE_CONFIG` table can be set through
  `device_config_overrides`.
- A table fed by a pass-through block can be replaced in its entirety by
  declaring the block at the variant level, as described in
  [Pass-through settings](#pass-through-settings-xgs).
- Any other output table the ASIC declares can be modified through a
  platform-level `conditional_settings` entry whose `apply` block names the
  table. The generator rejects an entry that targets a table not listed in
  the ASIC's `table_names` when it is constructed.

On DNX:

- An unconditional SOC property can be set through
  `platform_sdk_overrides`, which overrides `base_sdk_settings` and the
  declarative tables.
- A scenario-dependent SOC property belongs in a platform-level
  `conditional_settings` entry applying to `common`. Conditional settings
  are the highest-priority step of the DNX pipeline and therefore also
  override generated wiring keys.

In both ASIC families, a setting that requires computation or new structure (for
example, new port mapping logic) needs a generic, data-driven extension of
the generator and the schema rather than a platform-specific branch in the
code.

## Example: adding an XGS platform

No Python changes are required to add a platform on an ASIC family that the
tool already supports. Suppose a new Tomahawk5 board named `newboard` is being
added.

1. Ensure that `fboss/configs/platforms/<system_vendor>/newboard/platform_mapping/` exists and
   contains the platform's static mapping and vendor data (see the
   [platform mapping documentation](../platform_mapping.md)).
2. Create
   `fboss/configs/platforms/<system_vendor>/newboard/asic_config/asic_config.json`.
   The following is a complete example declaring a single variant; an existing
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
python3 -m fboss.lib.asic_config_v3.test.validate_asic_configs_schemas \
  --fboss-root "$PWD/fboss"
./fboss/lib/asic_config_v3/run-helper.sh
```

The output appears beside the input as
`generated/newboard_newvariant.yml`.

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

## Example: adding a DNX platform

Adding a platform on the DNX family follows the same pattern as on XGS. Only
the variant fields differ. The meru800bia platform file at
`fboss/configs/platforms/arista/meru800bia/asic_config/asic_config.json`
is a complete reference. The following example shows the overall structure:

```json
{
  "platform_name": "newdnxboard",
  "vendor": "broadcom",
  "asic": "jericho3",
  "output_structure": {"mode": "common_only"},
  "defaults": {
    "asic_config_params": {
      "config_type": "KEY_VALUE_CONFIG",
      "config_gen_type": "DEFAULT",
      "port_config": "default",
      "multistage_role": "NONE"
    },
    "platform_sdk_overrides": {
      "appl_param_local_system_port_voq_connector_start": "93184"
    },
    "conditional_settings": [
      {
        "name": "port_profile_default",
        "condition": {"param": "port_config", "equals": "default"},
        "apply": {"common": {"ucode_port_8.BCM8889X": "CDGE4_0:core_0.2"}}
      },
      {
        "name": "prod_sdk_settings",
        "condition": {"param": "config_gen_type", "equals": "DEFAULT"},
        "apply": {"common": {"dpp_db_path": "/etc/packages/neteng-fboss-wedge_agent/current/db"}}
      }
    ]
  },
  "variants": {
    "default": {}
  }
}
```

The following steps complete the platform.

1. Ensure that `fboss/configs/platforms/<system_vendor>/<name>/platform_mapping/`
   contains the platform's complete `platform_mapping_v2` data set: static
   mapping, port profile mapping, profile settings, and SI settings. The
   platform mapping parser loads them together, and the generator derives
   every lane and polarity key from the static mapping.
2. Declare the SOC properties that are specific to this board and apply to
   every variant in `platform_sdk_overrides`.
3. Declare the port-map profile entries, such as the `ucode_port_*` keys, in
   a conditional setting keyed on `port_config`. Declare per-scenario
   settings keyed on `config_gen_type`, and multistage-role settings keyed
   on `multistage_role`, as applicable.
4. Validate, generate, and compare the output against the platform's
   reference in `fboss/lib/asic_config_v2/synced_asic_configs/`. The output
   appears beside the input as `generated/<name>_<variant>.json` and must
   match the reference byte for byte.

## Example: adding a new ASIC family

Supporting a new ASIC family, such as a different vendor or a different
product line of an existing vendor, requires one new generator plus its data
files. The Broadcom DNX family was added following these steps, and its
files can be used as a reference.

1. **Generator.** Add a generator module under `generators/` that subclasses
   `BaseAsicConfigGenerator`. The subclass declares `SUPPORTED_EFFECTS` and
   implements `generate()`, `output_extension`, and the conditional-setting
   hooks `_apply_settings`, `_validate_apply_target`, and
   `_validate_apply_value`. Reuse the shared patterns where possible. The
   variant and defaults merge, the `asic_config_params` handling, and
   condition evaluation come from the base class, wiring data comes from the
   platform mapping parser, and feature toggles use conditional settings.
   Keep the behavior data-driven and avoid per-platform branches.
2. **Vendor data.** Add
   `fboss/configs/asic_vendors/<vendor>/<family>/asics/<asic>.json` with
   the chip-intrinsic data the generator needs. Add family-wide common files
   only if several ASICs in the family share those settings. The DNX family
   has no family-wide common files.
3. **Registration.** Add the new vendor and ASIC pair to
   `_GENERATOR_REGISTRY` in `gen.py`.
4. **Schema.** Add a schema for the new ASIC file under `schemas/` and
   register it in `test/validate_asic_configs_schemas.py`. Extend
   `platform_config.schema.json` if the family introduces new platform-level
   fields. The DNX family added `output_structure`,
   `platform_sdk_overrides`, and new `asic_config_params` parameters.
5. **Build.** Add the new generator source to
   `cmake/AsicConfigV3ConfigCli.cmake`, and add a `python_library` target
   for it in `fboss/lib/asic_config_v3/BUCK`, wired into the `gen` binary's
   dependencies.
6. **Platform.** Add
   `fboss/configs/platforms/<system_vendor>/<name>/asic_config/asic_config.json`
   selecting the new vendor and ASIC.
7. **Verification.** Compare the generated output against a known-good
   reference configuration for that family. For ASIC families with synced
   references, the match must be exact.
