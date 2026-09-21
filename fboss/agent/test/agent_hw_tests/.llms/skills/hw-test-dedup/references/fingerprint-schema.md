# Per-Test Coverage Profile (Fingerprint) Schema

This document defines the per-test "coverage profile" (fingerprint): a normalized, token-based record of **what each test actually verifies on the hardware** — the HW objects/SAI calls it programs, the conditions/qualifiers it matches, the actions it installs, how deeply it verifies, its warmboot shape, its traffic mode, and the ASIC/SDK gates under which all of that runs. Fingerprints are tokens, not prose or source excerpts, so they can be compared mechanically across hundreds of tests and across feature areas.

```json
{
  "type": "object",
  "required": ["tests", "rosterNote"],
  "properties": {
    "rosterNote": {
      "type": "string",
      "description": "Confirmed count of test cases found and any discrepancy vs prior inventory"
    },
    "tests": {
      "type": "array",
      "items": {
        "type": "object",
        "required": [
          "globalId",
          "testName",
          "fixture",
          "productionFeatures",
          "fixtureMode",
          "verificationApis",
          "trafficMode",
          "warmbootShape",
          "polarity"
        ],
        "properties": {
          "globalId": {
            "type": "string",
            "description": "filepath::FullyQualifiedTestName"
          },
          "testName": { "type": "string" },
          "fixture": { "type": "string" },
          "baseClass": { "type": "string" },
          "productionFeatures": {
            "type": "array",
            "items": { "type": "string" }
          },
          "fixtureMode": {
            "type": "string",
            "description": "Area-agnostic runtime-mode binding, formatted `<binding>:<modeTokens>` where binding is `pinned` (fixture overrides the flag via setCmdLineFlagOverrides) or `inherited` (takes the platform default), and modeTokens is the comma-separated mode value(s) the test actually runs in. The flag and the mode tokens are area-specific and defined by the area's coverage pack (e.g. ACL: `pinned:multi`, `inherited:single,multi` for FLAGS_enable_acl_table_group; multi_switch areas: `inherited:mono,mnpu`)."
          },
          "perEntry": {
            "type": "array",
            "items": {
              "type": "object",
              "properties": {
                "matchTuple": {
                  "type": "array",
                  "items": { "type": "string" },
                  "description": "The matched conditions/qualifiers for this entry (combination, not flattened set)"
                },
                "valueClasses": {
                  "type": "array",
                  "items": { "type": "string" },
                  "description": "boundary/min/max/masked, v4 vs v6 widths, etc."
                },
                "actions": {
                  "type": "array",
                  "items": {
                    "type": "object",
                    "properties": {
                      "type": { "type": "string" },
                      "param": { "type": "string" }
                    }
                  }
                },
                "priority": { "type": "string" },
                "overlaps": { "type": "boolean" }
              }
            }
          },
          "counterTypes": {
            "type": "array",
            "items": { "type": "string" }
          },
          "hwObjects": {
            "type": "array",
            "items": { "type": "string" },
            "description": "HW objects / SAI calls programmed"
          },
          "verificationApis": {
            "type": "array",
            "items": { "type": "string" }
          },
          "verificationDepthByObject": {
            "type": "string",
            "description": "brief: which objects are verified and how deeply"
          },
          "warmbootShape": {
            "type": "object",
            "description": "Per-stage warmboot coverage, decomposed so two warmboot tests can be compared stage-by-stage (not as a single opaque string). See warmboot-semantics.md.",
            "properties": {
              "argForm": { "type": "string", "enum": ["1-arg", "2-arg", "4-arg", "none"] },
              "coldSetup": { "type": "array", "items": { "type": "string" }, "description": "state/mutations established in setup() (runs cold-boot only; persists into warmboot as warm state)" },
              "coldVerify": { "type": "array", "items": { "type": "string" }, "description": "assertions in verify() (runs on BOTH boots); EMPTY means this test's coverage is warmboot-only" },
              "postWbSetup": { "type": "array", "items": { "type": "string" }, "description": "mutations in setupPostWarmboot() (runs warmboot only)" },
              "postWbVerify": { "type": "array", "items": { "type": "string" }, "description": "assertions in verifyPostWarmboot() (runs warmboot only)" },
              "direction": { "type": "string", "description": "boot-direction intent: canary-on/off, up-down-up, before/after reinit, etc." }
            }
          },
          "trafficMode": {
            "type": "string",
            "enum": ["none", "cpu", "frontpanel", "both"]
          },
          "addrFamily": { "type": "string" },
          "asicSdkGates": {
            "type": "array",
            "items": { "type": "string" }
          },
          "polarity": {
            "type": "string",
            "enum": ["positive", "negative", "mixed"]
          },
          "mutationSequence": {
            "type": "array",
            "items": { "type": "string" }
          },
          "scaleClass": { "type": "string" },
          "uniqueAssertions": {
            "type": "array",
            "items": { "type": "string" }
          },
          "coveragePack": {
            "type": "object",
            "description": "Open, area-keyed extension. Each feature area adds its own fields under its area key.",
            "additionalProperties": { "type": "object" },
            "examples": [
              { "acl": {}, "qos": {} }
            ]
          }
        }
      }
    }
  }
}
```

## Field reference

| Field | Meaning (one line) |
| --- | --- |
| `globalId` | Stable join key `filepath::FullyQualifiedTestName` used to reference this test across all phases. |
| `testName` | Bare test-case name (no fixture/path). |
| `fixture` | The test fixture/class the case is defined in. |
| `baseClass` | The base test class the fixture derives from (e.g. the AgentHwTest base). |
| `productionFeatures` | Exact `getProductionFeaturesVerified()` set this test attests to. |
| `fixtureMode` | Area-agnostic runtime-mode binding `<binding>:<modeTokens>` (binding = pinned\|inherited). The flag and mode tokens are area-specific (defined in `coveragePack`); e.g. ACL `pinned:multi`, multi-switch `inherited:mono,mnpu`. |
| `perEntry[]` | One record per installed entry, capturing its matched conditions/qualifiers and effects. |
| `perEntry[].matchTuple` | The matched conditions/qualifiers for this entry, as a combination (tuple), not a flattened type set. |
| `perEntry[].valueClasses` | Value classes exercised: boundary/min/max/masked, v4 vs v6 widths, etc. |
| `perEntry[].actions[]` | Actions installed on the entry, each with a `type` and a `param` (e.g. setDscp value, queue id, counter-sharing mode). |
| `perEntry[].priority` | Entry priority/ordering token. |
| `perEntry[].overlaps` | Whether this entry overlaps another (priority-resolution coverage). |
| `counterTypes[]` | Counter types exercised (PACKETS/BYTES/both). |
| `hwObjects[]` | HW objects / SAI calls the test programs. |
| `verificationApis[]` | Verification APIs called (state-introspection and dataplane checks). |
| `verificationDepthByObject` | Brief note of which objects are verified and how deeply. |
| `warmbootShape` | Per-stage warmboot coverage object: `argForm`, `coldSetup[]`, `coldVerify[]`, `postWbSetup[]`, `postWbVerify[]`, `direction`. Decomposed per `verifyAcrossWarmBoots` stage so Gate 3 compares stage-by-stage; an empty `coldVerify` means coverage is warmboot-only. |
| `trafficMode` | Dataplane mode exercised: none, cpu, frontpanel, or both. |
| `addrFamily` | Address family/families exercised (v4/v6). |
| `asicSdkGates[]` | ASIC/SDK conditionality gates that change what runs (SDK-version gates, `isSupportedOnAllAsics(...)`, metadata-qualifier swaps, vendor branches). |
| `polarity` | positive (accept), negative (reject), or mixed. |
| `mutationSequence[]` | Lifecycle/idempotency steps (add/modify/del/re-add). |
| `scaleClass` | Entry-count/scale intent bucket. |
| `uniqueAssertions[]` | Assertions/scenarios this test carries that may need to be ported on removal. |
| `coveragePack` | Open object keyed by area (e.g. `{"acl": {...}, "qos": {...}}`); each area's discoverer adds extra fingerprint fields here so areas can extend the profile without changing the core schema. |

`globalId` is the stable join key used across all phases — fingerprinting, detection, refutation, merge, and reporting all reference tests by `globalId`, so it must be unique and reproduced verbatim.

## Parameterized / typed macros (one fingerprint per macro)

Emit exactly one fingerprint per `TEST_F`/`TEST_P`/`TYPED_TEST` macro — do not expand instantiations (this keeps the count aligned with `scripts/roster-count.js`). Collapse a parameterized/typed macro's instantiations into that single fingerprint by taking the **union, never the intersection**, of their coverage. This applies to the tag fields, not just value fields: `productionFeatures`, `asicSdkGates`, and `hwObjects` must each be the union over every instantiation (and `addrFamily`/`fixtureMode` likewise span all type params). Union is the safe direction: a type param that swaps the fixture (e.g. physical-port vs aggregate-port) can add a feature tag such as `LAG` that the base instantiation lacks, and under-counting `productionFeatures` here could silently strip the macro's last-owner protection under Gate 1.
