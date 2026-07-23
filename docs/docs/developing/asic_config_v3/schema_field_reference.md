---
sidebar_position: 2
---

# Schema Field Reference

This page summarizes the fields defined by the JSON Schemas under
`fboss/lib/asic_config_v3/schemas/`. The schemas are the authoritative
reference. They define the complete structure of each configuration file, and
every field includes a description.

The configuration files contain two kinds of keys.

- **Framework fields** are the fields listed on this page. They are declared
  in the schemas and interpreted directly by the generator. Introducing a new
  framework field requires adding it to the corresponding schema.
- **Setting maps** are sections such as `platform_sai_overrides`,
  `device_config_overrides`, or the ASIC-level `global_defaults` and
  `sai_overrides`, whose keys are vendor SDK and SAI setting names, port
  speeds, or output table entries. The schemas deliberately accept any key
  inside a setting map, because the set of valid vendor settings cannot be
  enumerated, so adding a setting there requires no schema change.

## Platform Configuration

Platform configuration, defined by `platform_config.schema.json`. This schema
validates every `platforms/<platform>/asic_config.json` file.

### Top-level fields

| Field | Required | Description |
|---|---|---|
| `platform_name` | yes | Platform identifier. Must match the directory name under `platforms/`. |
| `vendor` | yes | ASIC vendor identifier. Must match a directory name under `vendors/`. |
| `asic` | yes | ASIC chip identifier. Must match the basename of a JSON file under `vendors/<vendor>/<family>/asics/`. |
| `num_ports_per_core` | no | Number of front-panel ports per ASIC core. Combined with the ASIC's lanes-per-core value to derive the number of lanes per port. |
| `num_lanes_per_port` | no | Explicit override of the derived lanes-per-port value, for platforms that wire only a subset of the lanes. |
| `defaults` | no | Shared settings inherited by every variant. A variant's own settings take precedence over the corresponding defaults. |
| `variants` | yes | Named configuration variants. Each variant produces a separate output file. |

### Variant fields

Each entry under `variants`, as well as the `defaults` block, may declare the
following fields.

| Field | Description |
|---|---|
| `asic_config_params` | Parameters controlling generator behavior: `config_type` (output format), `config_gen_type` (generation profile, for example `DEFAULT`), `exact_match` (enables exact-match forwarding table entries), and `mmu_lossless` (enables MMU lossless mode for PFC and RDMA workloads). |
| `port_config` | Front-panel port settings: `default_speed` (in Mbps), `speed_to_fec` (mapping from speed to FEC mode), `enable` (the value emitted for `PC_PORT.ENABLE`), and `pc_port_overrides` (extra `PC_PORT` key/value lines emitted verbatim, for example `LINK_TRAINING`). |
| `cpu_port` | CPU port settings: `speed` (in Mbps) and `num_lanes`. |
| `mgmt_port` | Management port settings: `enabled` (whether the port is emitted at all), `enable` (the `PC_PORT.ENABLE` value), `in_port_block` (include the port in the `PORT` MTU range), and `speed_variants` (per-speed settings keyed by speed). |
| `mgmt_port_overrides` | Platform overrides of the ASIC-level management port defaults: `logical_id`, `physical_id`, `speed`, `num_lanes`, and `fec`. |
| `port_mapping_overrides` | Adjustments to the logical-to-physical port mapping formula: `core_range` (which cores participate, for example `"0-15,48-63"`), `num_lp_ports_on_even_core`, `num_logical_ports_per_datapath`, `lp_start_step_offset`, `lp_offset_simple`, and `special_core_offset_apply_all`. |
| `platform_sai_overrides` | Platform-level SAI settings. These take precedence within the layered `global` sources and override the ASIC-level SAI overrides. |
| `device_config_overrides` | Key/value pairs written to the `DEVICE_CONFIG` output table, for example clock frequencies. |
| `features` | Boolean toggles for optional configuration sections: `generate_dlb_config`, `generate_dlb_ecmp_config`, `generate_low_clock_freq_settings`, and `generate_autoload_board_settings`. Absent flags default to disabled. |
| `conditional_settings` | Platform-level conditional settings, evaluated after the ASIC-level ones so that platform values take precedence. |
| `preamble_file` | Path, relative to the `asic_config_v3` directory, of a preamble file prepended to the generated YAML. Honored only when the ASIC declares `preamble_support: true`. |
| `platform_mapping_name` | Overrides the `platform_mapping_v2` directory consulted for lane-map and polarity data. Defaults to `platform_name`. Useful when several variants share one `asic_config.json` but consume sibling platform mapping directories. |
| `ctr_eflex_config` | Variant-level override of the ASIC's `ctr_eflex_config` (enhanced flex counter) pass-through block. When present, it replaces the ASIC-level value in its entirety. |

In addition to the fields above, the generator honors a variant-level override
of any data block routed by the ASIC's `pass_through_settings`. Declaring
`flex_counter_settings`, `port_config_defaults`, `fp_config_defaults`, or
`tm_thd_config_defaults` in a variant replaces the corresponding ASIC-level
block, in the same way as `ctr_eflex_config`. The schema currently declares
only `ctr_eflex_config` among these.

The platform schema accepts unknown keys at the top and variant levels, so a
key that is not listed here does not fail validation; the generator ignores it
without an error or warning. When a setting appears to have no effect, verify
the field name against this reference.

## ASIC Configuration

Broadcom XGS ASIC configuration, defined by
`broadcom_xgs_asic_config.schema.json`. This schema validates the per-ASIC
files under `vendors/broadcom/xgs/asics/`.

| Field | Description |
|---|---|
| `vendor`, `asic` | Chip identity, for example `broadcom` and `tomahawk5`. |
| `port_architecture` | Core, lane, and datapath layout consumed by the port mapping logic: `num_cores`, `num_lanes_per_core`, `num_logical_ports_per_datapath`, `num_physical_ports_per_datapath`, and `num_lp_ports_on_even_core_default`. |
| `mmu_size` | MMU buffer size in bytes. Also used as the `MAX_FRAME_SIZE` and port MTU values. |
| `table_names` | Ordered list of output YAML table names. Controls which tables appear in the output and their order. |
| `mgmt_port_defaults` | Default management port settings: `logical_id`, `physical_id`, `speed`, `num_lanes`, and `fec`. Platforms may override these per variant. |
| `preamble_support` | Whether the ASIC supports a preamble section prepended to the output YAML. When `false`, any `preamble_file` declared by a platform is ignored. |
| `global_defaults` | ASIC-specific SDK settings copied into the `global` output table. |
| `sai_overrides` | ASIC-specific SAI settings layered on top of the vendor SAI common values. |
| `flex_counter_settings`, `ctr_eflex_config`, `port_config_defaults`, `fp_config_defaults`, `tm_thd_config_defaults` | Named data blocks routed into output tables by `pass_through_settings`. |
| `dlb_defaults`, `dlb_ecmp_config_defaults` | Dynamic Load Balancing settings, applied through conditional settings gated by the corresponding feature toggles. |
| `pass_through_settings` | Declarative routing list. Each entry names a `source` data block in this file and the `target_table` it is copied into. |
| `conditional_settings` | ASIC-wide conditional settings, described below. |

### Conditional setting entries

Each entry in a `conditional_settings` array supports the following fields.
The entry structure is defined in the Broadcom XGS ASIC schema.
Platform-level entries follow the same structure, but the platform schema does
not currently validate their internal structure, so field names and the
`condition` format in platform-level entries should be reviewed carefully.

| Field | Required | Description |
|---|---|---|
| `name` | yes | Identifier, unique within the surrounding list. |
| `description` | no | Free-form explanation of the setting. |
| `condition` | yes | The test to evaluate against the variant configuration. Its keys are listed in the table below. |
| `apply` | no | Inline settings applied when the condition is satisfied, keyed by target output table name. |
| `apply_from` | no | Reference to a data block elsewhere in the ASIC file; the named `source` block is copied into the named `target_table` when the condition is satisfied. |
| `skip_from_sai_common` | no | Keys to omit from the vendor SAI common layer while the condition is satisfied. Honored only on ASIC-level entries. |

The `condition` object contains exactly the following three keys, all of which
are required:

| Key | Description |
|---|---|
| `source` | The variant-config section holding the parameter to evaluate: either `asic_config_params` or `features`. |
| `param` | The name of the parameter to compare. |
| `equals` | The value the parameter must equal for the condition to be satisfied. |

## Vendor Common Configuration

Vendor common configuration, defined by `vendor_common.schema.json`. This
schema validates the family-wide common files, such as
`vendors/broadcom/xgs/sdk_common.json` and
`vendors/broadcom/xgs/sai_common.json`.

| Field | Required | Description |
|---|---|---|
| `global` | yes | SDK or SAI settings copied into the `global` output table according to the layering order described in the [Overview](./overview.md). |
