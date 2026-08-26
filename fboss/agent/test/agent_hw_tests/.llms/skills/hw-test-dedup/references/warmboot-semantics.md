# Warmboot semantics: `verifyAcrossWarmBoots`

This cheat-sheet explains how `verifyAcrossWarmBoots` runs callbacks across the cold-boot and warmboot processes, so Gate 3 (warmboot shape) can be adjudicated correctly.

## The 4-arg form and its callback ordering

The canonical 4-arg signature (from `fboss/agent/test/AgentHwTest.h`, the template at ~lines 57-89) is:

```cpp
template <
    typename SETUP_FN,
    typename VERIFY_FN,
    typename SETUP_POSTWB_FN,
    typename VERIFY_POSTWB_FN>
void verifyAcrossWarmBoots(
    SETUP_FN setup,
    VERIFY_FN verify,
    SETUP_POSTWB_FN setupPostWarmboot,
    VERIFY_POSTWB_FN verifyPostWarmboot) {
  if (agentEnsemble_->getSw()->getBootType() != BootType::WARM_BOOT) {
    XLOG(DBG2) << "cold boot setup()";
    setup();
  } else {
    XLOG(DBG2) << "skip setup() for warmboot";
  }

  if (runVerification()) {
    XLOG(DBG2) << "verify()";
    verify();
  }

  if (agentEnsemble_->getSw()->getBootType() == BootType::WARM_BOOT) {
    // If we did a warm boot, do post warmboot actions now
    XLOG(DBG2) << "setupPostWarmboot()";
    setupPostWarmboot();

    if (runVerification()) {
      XLOG(DBG2) << "verifyPostWarmboot()";
      verifyPostWarmboot();
    }
  }
  ...
}
```

The behavioral facts that matter:

- On the **cold-boot process**, `setup()` runs, then `verify()` runs. `setupPostWarmboot()` and `verifyPostWarmboot()` do NOT run.
- On the **warmboot process**, `setup()` is skipped; `verify()` runs, then `setupPostWarmboot()` runs, then `verifyPostWarmboot()` runs.
- Therefore `verify()` runs on BOTH the cold and warmboot passes, while `setupPostWarmboot()`/`verifyPostWarmboot()` run ONLY on the warmboot process, and `setup()`'s effects exist on the warmboot pass only as warm-booted state, not via re-execution.

## The base classes that define `verifyAcrossWarmBoots`

Four base classes define `verifyAcrossWarmBoots`:

- `fboss/agent/test/AgentHwTest.h`
- `fboss/agent/test/AgentEnsembleTest.h`
- `fboss/agent/test/AgentTest.h`
- `fboss/agent/hw/test/HwTest.h` (legacy HwTest framework)

Differences to be aware of:

- The three Agent-framework classes (`AgentHwTest`, `AgentEnsembleTest`, `AgentTest`) gate both `verify()` and `verifyPostWarmboot()` behind `runVerification()`, and each also provides a 1-arg verify-only overload in addition to the 2-arg and 4-arg forms.
- The legacy `HwTest` runs `verify()` unconditionally (no `runVerification()` guard) and provides only the 2-arg and 4-arg forms, not the 1-arg overload.
- The callback ORDERING is identical across all four: cold boot runs `setup()` then `verify()`; warmboot skips `setup()`, runs `verify()`, then `setupPostWarmboot()`, then `verifyPostWarmboot()`.

The 2-arg and 1-arg overloads all delegate to the 4-arg form with no-op (`[]() {}`) post-warmboot callbacks, so a 2-arg call never exercises a post-warmboot mutation phase.

## The practical rule

A 2-arg test `verifyAcrossWarmBoots(setup, verify)` cannot subsume a 4-arg test, because the 2-arg form passes no-op post-warmboot callbacks and so never runs the post-warmboot mutate-and-verify phase the 4-arg test depends on.

Coverage placed only in `setup` is exercised on the cold-boot pass only; coverage placed only in `setupPostWarmboot`/`verifyPostWarmboot` is exercised on the warmboot pass only. When checking subsumption, match coverage to the boot type on which it actually runs, not merely to the test that contains it.

## Distinguishing two 4-arg shapes from each other

"Both 4-arg" does not make two warmboot tests interchangeable. Fingerprint each one's per-stage token lists and compare them stage-by-stage:

| field | what it holds |
| --- | --- |
| `argForm` | `1-arg` / `2-arg` / `4-arg` / `none` |
| `coldSetup` | state/mutations established in `setup()` (cold-boot only; persists into warmboot as warm state) |
| `coldVerify` | assertions in `verify()` (runs on BOTH boots). **Empty ⇒ coverage is warmboot-only** |
| `postWbSetup` | mutations in `setupPostWarmboot()` (warmboot only) |
| `postWbVerify` | assertions in `verifyPostWarmboot()` (warmboot only) |
| `direction` | boot-direction intent: canary-on/off, up-down-up, before/after reinit, etc. |

Two 4-arg tests are foldable only if all of these match. They commonly differ in: which mutation runs cold vs post-warmboot (e.g. member-down in `coldSetup` vs in `postWbSetup`), whether `coldVerify` is empty (coverage warmboot-only), and `direction` (down-then-up before reinit vs up-down-up). Any such difference is a distinct boot-time HW path and blocks the fold under Gate 3 — even though a single "4-arg warmboot" label would hide it.

## Concrete ACL example

Folding `RemoveAclTable`'s empty-group teardown into a 4-arg post-warmboot phase moved that teardown so it ran only on the warmboot pass. That lost the cold-boot coverage of removing the ACL table from an empty group on a freshly cold-booted switch. Because the cold-boot path was a distinct, real coverage point, the test was kept rather than folded.
