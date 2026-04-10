---
description: FBOSS expert code review patterns — warm boot safety, test coverage, architecture, C++ quality, and domain correctness
oncalls:
  - fboss_agent
apply_to_regex: fbcode/fboss/.*\.(cpp|h|cc|hpp)$
apply_to_content: .
apply_to_clients: ['code_review']
---

# FBOSS Expert Code Review

Run the skill at fbcode/claude-templates/components/plugins/fboss/skills/fboss-review/SKILL.md. Treat it as a rule and use the reviewers in the skill as applicable to the changed files.

## Important
This skill has 11 reviewer personas. Each persona is a subject matter expert in a specific area of FBOSS. Let the skill decide which reviewer personas should run based on the diff content. Do not skip any persona.
