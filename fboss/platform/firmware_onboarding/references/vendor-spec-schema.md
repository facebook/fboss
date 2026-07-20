# Platform Spec Schema

This document describes the **platform spec** — a JSON file you (the platform supplier) provide to Meta so we can onboard your new switch's firmware into our automation. The spec captures what we need to know about the platform and its components. It does not require any knowledge of Meta-internal systems.

A spec lives at:

```
fbcode/fboss/platform/firmware_onboarding/configs/<platform_name>/spec.json
```

One directory per platform — same convention used by `fbcode/fboss/platform/configs/<platform>/`. Per-platform sidecar files (READMEs, change overlays, vendor handoff notes) can live in the same directory alongside `spec.json`.

A worked example is in `configs/icecube800bc/spec.json`.

> **Note on terminology — "the orchestrator":** several sections below refer to *the orchestrator*. This is the Meta-side automation that reads your `spec.json` and drives the per-platform onboarding work (registering components, allocating firmware identifiers, wiring upgrades, and tagging packages for production). You don't run it and you don't need to know its internals — it just consumes the spec you provide. The references to it explain what data the orchestrator will derive from each field.

---

## Top-level structure

```jsonc
{
  "spec_version": "1.0",
  "platform":     { ... },   // platform identity
  "components":   [ ... ],   // list of replaceable / firmware-bearing components
  "_review_notes": [ ... ]   // optional — fields you still need to confirm
}
```

| Field | Required | Notes |
|---|---|---|
| `spec_version` | yes | Currently `"1.0"`. |
| `platform` | yes | Platform-wide identity. |
| `components` | yes | One entry per individually-managed firmware instance. Components without firmware do not get entries. |
| `_review_notes` | optional | Free-form list flagging fields you want Meta to double-check before processing. |

---

## `platform`

Platform-wide identity. Both fields are required.

```jsonc
"platform": {
  "name": "icecube800bc",
  "silicon": [
    { "name": "TH6", "pcie_address": "00:01.0" }
  ]
}
```

| Field | Type | Notes |
|---|---|---|
| `name` | string | Lowercase canonical name of the platform (e.g. `"icecube800bc"`). This is the name Meta uses to refer to your platform across all systems — it's the same name that is programmed in the eeprom. The name that was communicated to you by Meta. No spaces, no uppercase. |
| `silicon` | list of objects | NPU silicon families on the platform. Always a list (even for a single NPU) so dual-NPU platforms can be expressed uniformly. |
| `silicon[].name` | string | Canonical short token for the silicon family (e.g. `"TH6"`, `"TH5"`, `"J3"`, `"R3"`). |
| `silicon[].pcie_address` | string | PCIe bus address of the NPU on this platform, format `BB:DD.F` (lowercase hex, e.g. `"00:01.0"`). Aggregated by Meta into the platform's PCIe presence-check list alongside per-component `pcie_address` entries. |

> **Component PCIe addresses** are declared per-component (see `pcie_address` in the components section), not here. Meta aggregates the silicon addresses with per-component addresses into the platform's PCIe presence-check list at onboarding time.

---

## `components`

**Only firmware-bearing components belong in this list.** A component is in scope if Meta will upgrade its firmware during provisioning or rollout pushes. Purely mechanical parts (fan trays without firmware, Passive PSUs without upgradable Firmware during provisioning, brackets) do not get entries.

How to count entries depends on whether multiple physical instances share a single firmware image:

- **Different image per instance** → declare one entry per instance (e.g. `pic_cpld1`, `pic_cpld2` as two separate entries, each with its own `name`).
- **Same image shared across instances** → declare a **single** entry with the `instances` block (see below). Meta will build one firmware package for all the physical instances and wire each instance to the shared package.

> **Note on naming**: this spec uses `at_end` and `before_first_underscore` for the `instances.position` field. Some Meta-internal tooling (`package_configuration` / `run_script`) uses `suffix` / `prefix` for the same concept — they're equivalent in meaning, only the field values differ. The longer names were chosen here because `prefix` is mildly ambiguous (it doesn't go at the literal start of the name, but right before the first underscore).

Each component entry. The example below shows the **shared-image multi-instance case** — `pic_cpld1` and `pic_cpld2` both run the same firmware binary, so they collapse into a single entry with an `instances` block:

```jsonc
{
  "name":                "pic_cpld",
  "location":            "Physical Interface Card",
  "display_location":    "pic",
  "field_replaceable":   true,
  "instances": {
    "range":    "1-2",
    "position": "at_end"
  },
  "firmware": {
    "priority":              40,
    "shared_with_platforms": []
  }
}
```

> `platform_identifier` is omitted in this example — it defaults to `platform.name` ("icecube800bc" here). Only set it explicitly when the same firmware image is shared across multiple platforms (e.g. `"IceLakeD"`); see the field description below.
>
> A single-instance entry (e.g. one BIOS) would simply omit the `instances` block. See [`instances` block](#instances-block-shared-image-multi-instance) below for the full multi-instance contract.

| Field | Type | Required | Notes |
|---|---|---|---|
| `name` | string | yes | Lowercase, unique per spec. For a single-instance component, this is the literal firmware-instance name (e.g. `"bios"`, `"smb_cpld"`, `"fan_cpld"`). For a shared-image multi-instance component (when `instances` is present), this is the **base** name (e.g. `"pic_cpld"`) — the orchestrator expands it into the per-instance names using the `instances` block. |
| `platform_identifier` | string | optional, default = `platform.name` | The platform this firmware applies to. Defaults to the spec's `platform.name`. Override only when the same firmware image is shared across multiple platforms — in that case use a stable shared identifier (e.g. `"IceLakeD"`). Meta uses this to deduplicate shared firmware so the same image isn't created twice. |
| `location` | string | yes | Human-readable physical home of the component (e.g. `"Physical Interface Card"`, `"Main Board"`, `"Power Supply"`). Used by Meta's repair-routing automation. |
| `display_location` | string | yes | Short tag used for grouping in Meta's UIs and dashboards (e.g. `"pic"`, `"smb"`, `"psu"`, `"fan"`). Lowercase, no spaces. |
| `field_replaceable` | boolean | yes | `true` if the component (or the assembly it sits on) can be replaced in the field without returning the entire chassis. |
| `instances` | object | optional | **Use only when a single firmware image is shared across multiple physical instances of this component.** Drives expansion of the base `name` into per-instance names. See [`instances` block](#instances-block-shared-image-multi-instance) below. Omit when each instance has its own firmware image — declare separate entries instead. |
| `pcie_address` | string \| list of strings | optional | The component's PCIe bus address(es) in the format `BB:DD.F` (e.g. `"01:00.0"`). Single string for a single-instance entry; list of N strings (one per instance, in the same order as the expanded `instances` range) for a multi-instance entry. Provide for components that appear on the PCIe bus; omit for components that aren't PCIe-attached. |
| `firmware` | object | yes | Always required. Components without firmware should not be declared. |
| `firmware.priority` | integer | yes | Upgrade ordering priority. Lower numbers are upgraded first. Components with the same priority may be upgraded in any order. Range: 1–100. |
| `firmware.shared_with_platforms` | list of strings | yes | Other platform names where this same firmware image is also installed. **Always present — use an empty list `[]` when the firmware is platform-exclusive.** Meta cross-checks the listed platforms' specs to verify they declare a matching `platform_identifier`. |

### `instances` block (shared-image multi-instance)

Use this block **only** when a single firmware image is installed on multiple physical instances of the same component. The orchestrator expands the entry's base `name` into per-instance names at onboarding time, but builds **only one firmware package** that all the physical instances reference.

```jsonc
"instances": {
  "range":    "1-2",
  "position": "at_end"
}
```

| Field | Type | Required | Notes |
|---|---|---|---|
| `range` | string | yes | Contiguous integer range, inclusive (e.g. `"1-2"`, `"1-8"`). Single instance is allowed but pointless — drop the `instances` block in that case. Non-contiguous ranges (`"1,3,5"`) are not supported in v1. |
| `position` | string | yes | Where the index is inserted into the base `name`. One of: `"at_end"` (appended to the end — `pic_cpld` + index 1 → `pic_cpld1`) or `"before_first_underscore"` (inserted right before the first underscore — `pic_cpld` + index 1 → `pic1_cpld`). `before_first_underscore` requires the base `name` to contain at least one underscore. |

**Important — when to use `instances` vs separate entries:**

| Situation | What to declare |
|---|---|
| 8 PICs, all 8 run the same CPLD firmware image | One entry, `name: "pic_cpld"`, `instances: {range: "1-8", position: "at_end"}` |
| 8 PICs, each PIC has its own distinct firmware image | Eight separate entries: `pic_cpld1` … `pic_cpld8` |
| 1 BIOS on the main board | One entry, `name: "bios"`, no `instances` block |
| 2 SMB FPGAs sharing one image | One entry, `name: "smb_fpga"`, `instances: {range: "1-2", position: "at_end"}` |

**Worked expansion example** (range `"1-2"`, position `"at_end"`, base name `pic_cpld`):

- Effective instance names: `pic_cpld1`, `pic_cpld2` (used for per-instance presence checks and replaceable-unit tracking)
- Firmware package: a **single** package built once, named after the base `pic_cpld` (no index)
- Both instances reference that one package for upgrades

If `pcie_address` is provided alongside `instances`, it must be a list whose length matches the range size, with addresses in the same order as the expanded indices (first address → instance 1, second → instance 2, etc.).

### Recommended components to declare

For a typical FBOSS BMC-Lite switch the list usually includes:

- `bios` (on the main board)
- One entry per distinct CPLD design (`smb_cpld`, `fan_cpld`, …) — if multiple physical instances of the **same** CPLD design exist on the board, use a single entry with the `instances` block (see [`instances` block](#instances-block-shared-image-multi-instance)) rather than one entry per physical instance.
- One entry per distinct FPGA design on PCIe (`smb_fpga`, `scm_fpga`, …) — these get `pcie_address` populated (a list when paired with `instances`).
- **Per-PIC firmware:** prefer a single entry with `instances` (e.g. `pic_cpld` with `instances: { range: "1-2", position: "at_end" }`) when both PIC boards run the **same** firmware image. Only declare separate entries (`pic_cpld1`, `pic_cpld2`) when each PIC carries a **distinct** firmware image — which is rare on FBOSS designs.
- PSU entries only when the PSU carries firmware (omit if purely mechanical).

Add others as your design requires — but always only firmware-bearing components.

---

## `_review_notes`

Free-form list of strings flagging fields you want Meta to verify before processing. Meta surfaces these during review.

```jsonc
"_review_notes": [
  "components[3].firmware.priority is provisional — confirm desired upgrade order with platform lead",
  "pic_cpld instances may be reduced from 8 to 4 in the production build"
]
```

---

## Validation

A JSON Schema (`schemas/platform-spec.v1.json`) backs runtime validation. Meta's automation will reject the spec and report which fields failed.

Common validation errors:

- `platform.name` contains uppercase or spaces, or starts with a digit / underscore (must match `^[a-z][a-z0-9_]*$` — start with a lowercase letter, then lowercase letters, digits, or underscores).
- A component declares no `firmware` block, or its `firmware` block is missing `priority` or `shared_with_platforms`.
- Two components share the same `name` after instance expansion (e.g. one entry uses `name: "pic_cpld"` with `instances` covering 1–2, and another entry literally named `pic_cpld1` exists — collision).
- A component's `display_location` contains uppercase or spaces.
- A component's `pcie_address` doesn't match the `BB:DD.F` format, or collides with another component's address on the same platform.
- `instances.position == "before_first_underscore"` but the base `name` has no underscore.
- `instances.range` is malformed (not `"<start>-<end>"`, or end &lt; start, or non-integer).
- `pcie_address` is provided as a list when `instances` is absent, or as a string when `instances` is present, or its list length doesn't match the range size.
- `firmware.priority` falls outside the 1–100 range.
- `firmware.shared_with_platforms` lists a platform whose spec doesn't exist yet (warning only — the sibling platform may not be onboarded yet).
- `firmware.shared_with_platforms` lists a platform whose spec **does** exist but declares a different `platform_identifier` for the same component (**hard failure** — shared firmware images must use the same `platform_identifier` across every platform that shares them, otherwise Meta would create duplicate firmware records).

---

## Versioning

- Currently only `spec_version: "1.0"` is supported.
- Breaking changes bump the major version. Meta refuses unknown major versions.
- Forward-compatible additions (new optional fields) do not bump the version — older specs continue to work.

---

## Lifecycle

There is exactly **one canonical path per platform** — `configs/<platform>/spec.json`. The lifecycle stage is determined by the spec's content, not by where the file lives:

1. **Draft** — `_review_notes` is non-empty and/or the validator reports errors. The orchestrator refuses to operate on draft specs.
2. **Active** — `_review_notes` is empty and the validator passes. The orchestrator will run onboarding phases against the spec.
3. **Frozen** — the platform has reached production. The spec is immutable from this point; subsequent platform changes use a separate change-overlay file (mechanism TBD in v2; out of scope for v1).

This means a vendor writes directly into the final location from day one. There is no "promote from draft to active" step — just clear `_review_notes` and fix anything the validator flags, and the orchestrator will pick up the spec.

---

## Worked example

See `configs/icecube800bc/spec.json`.
