# FBOSS Skills

This directory contains reusable skills for working on
[FBOSS](https://github.com/facebook/fboss), the Facebook Open Switching
System. They cover FBOSS services and components such as Agent, QSFP, FSDB,
platform code, SAI/SDK integration, configuration, and hardware tests. The
skills are plain-text workflow instructions that can be used by coding agents
such as Claude Code, Codex, MetaCode, or other tools that understand
skill-style task guidance. They are written to be useful in both open-source
checkouts and Meta-internal environments.

The open-source skill set focuses on three workflows:

- Debugging FBOSS AgentHwTest failures.
- Applying FBOSS coding standards while changing code.
- Reviewing FBOSS diffs with FBOSS-specific review guidance.

## Directory Layout

In an open-source FBOSS checkout, the exported skills are expected under:

```text
fboss/skills/
  debug-agent-hw-test/
  fboss-code-standards/
  fboss-review/
```

In Meta-internal checkouts, the same source lives under:

```text
fbcode/claude-templates/components/plugins/fboss/skills/
```

The internal path contains `claude-templates` for packaging/history reasons,
but the skill content is not tied to Claude.

Some skills support optional environment-specific overrides under a
`facebook/` subdirectory. Those overrides are not required for open-source use.
When an override is not present, the skill falls back to the generic files under
`references/`.

## How To Use

Install or expose the `fboss/skills/` directory using the skill-loading
mechanism supported by your coding agent. Then ask the agent to use the relevant
skill by name:

```text
Use debug-agent-hw-test to debug AgentAclTest.AclNexthopTest on my switch.
Use fboss-code-standards while changing the route updater.
Use fboss-review to review this pull request.
```

Each skill has a `SKILL.md` entry point. The entry point tells the agent which
reference files to read for the task, so users do not need to know the full
directory structure.

## Skills

### `debug-agent-hw-test`

Use this skill when building, running, and debugging FBOSS AgentHwTest failures.
It covers:

- Running one test or a batch of tests.
- Mono and multi-switch AgentHwTest flows.
- Building test binaries and copying them to a switch.
- Running cold boot and warm boot test cycles.
- Reading test output, SAI replayer logs, and crash logs.
- Categorizing failures as FBOSS, vendor SDK, timeout, config, or pass.
- Creating a vendor-escalation package with replayer logs and hardware config.

The skill is intentionally environment-neutral. The open-source references use
standard `ssh`/`scp` style examples. Meta environments may provide their own
device-access and build-system overrides.

### `fboss-code-standards`

Use this skill as passive coding guidance when modifying FBOSS code. It captures
review patterns and common pitfalls across:

- Agent architecture, including mono and multi-switch behavior.
- Warmboot state preservation.
- SAI/SDK object lifecycle and attribute handling.
- FSDB and `thrift_cow` usage.
- Platform and config changes.
- Agent HW test patterns.
- General C++ conventions used in FBOSS.

This is the skill to consult before or during implementation.

### `fboss-review`

Use this skill for a structured FBOSS code review. It combines general code
review with FBOSS-specific reviewers for:

- Reliability and error handling.
- Engineering quality and performance.
- Code clarity and API design.
- Summary and test-plan quality.
- Silent failure risks.
- Agent architecture.
- FSDB and `thrift_cow`.
- Platform and config changes.
- SAI/SDK integration.
- Testing standards.
- Cross-cutting FBOSS architecture.

The skill reports findings to the user only. It does not post review comments
automatically.

## Typical Usage

For an AgentHwTest failure:

1. Use `debug-agent-hw-test`.
2. Gather the test name, mode, SDK/vendor SAI, switch, and config file.
3. Build the relevant binaries.
4. Run cold boot and warm boot.
5. Analyze logs only for failing tests.
6. Categorize the result and record the evidence.

For code changes:

1. Use `fboss-code-standards` while implementing.
2. Run the relevant unit tests, AgentHwTests, or build targets.
3. Use `fboss-review` before submitting a diff or pull request.

## Environment Notes

The skills avoid hardcoding one company's lab or build setup. They describe
abstract operations such as:

```text
UPLOAD <local-file> TO <switch>:<remote-file>
RUN ON <switch>: <command>
DOWNLOAD <switch>:<remote-file> TO <local-file>
```

Map those operations to your environment:

- In a generic open-source lab, this may be `ssh` and `scp`.
- In a Meta-internal lab, this may be a lab access tool or MCP integration.
- In CI, this may be a job runner or artifact upload/download step.

The important part is the workflow: build fresh binaries, deploy them, run the
test, inspect the right logs, and keep enough evidence for the result.

## Contributing

When adding or updating a skill:

- Keep instructions generic unless they belong in an environment-specific
  override.
- Put reusable workflow documentation under `references/`.
- Keep Meta-only or lab-specific commands out of open-source references.
- Prefer small, checkable rules with a clear "what to check" and "why it
  matters".
- Test the skill on a real or representative FBOSS workflow before publishing.
