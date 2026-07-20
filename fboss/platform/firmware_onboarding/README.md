# fboss/platform/firmware_onboarding

Vendor-facing spec contract and the Meta-side pre-flight validator for onboarding
new BMC-Lite FBOSS platform firmware.

This directory is shared between Meta-internal automation (which drives the full
phase-orchestrated onboarding workflow) and the FBOSS open-source community
(which uses it to author and validate per-platform vendor specs before
contributing them upstream). Nothing here depends on Meta-internal services.

## Layout

```
firmware_onboarding/
  schemas/        JSON Schema (draft 2020-12) for the vendor spec
  validator/      pre-flight validator (Python) + unit tests
  configs/        one subdirectory per onboarded platform, each holding spec.json
  references/     vendor-facing schema documentation
```

## Running pre-flight under OSS

The validator is pure Python. Install the two PyPI dependencies and invoke the
module directly:

```bash
pip install -r fboss/platform/firmware_onboarding/requirements.txt

# From the repo root (so the fboss.* import path resolves)
python -m fboss.platform.firmware_onboarding.validator.preflight \
    fboss/platform/firmware_onboarding/configs/<platform>/spec.json
```

Exit code `0` and stdout `OK` means the spec passed every check. Errors are
prefixed `[schema]`, `[semantic]`, or `[warn]` on stderr, and name the offending
field path so the vendor can fix the spec without reading the validator source.

To run the unit tests:

```bash
python -m unittest discover fboss/platform/firmware_onboarding/validator/tests
```

## Running pre-flight at Meta

```bash
buck2 run fbcode//fboss/platform/firmware_onboarding/validator:run_preflight -- \
    fbcode/fboss/platform/firmware_onboarding/configs/<platform>/spec.json
```

## Authoring a new spec

Copy `configs/icecube800bc/spec.json` as a starting point and edit it for the new
platform. The schema reference for every field is
`references/vendor-spec-schema.md`. Run pre-flight after each edit — the
validator's error messages identify the field to fix.
