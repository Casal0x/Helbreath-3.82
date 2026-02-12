# Changelog Format Guide

After every `bak.py commit`, append entries to `CHANGELOG.md` under category headers. Keep entries concise and explanatory — describe what changed from the user's perspective, not implementation details.

## Categories

Use whichever are relevant. Don't include empty categories.

- **New Features** — New functionality or systems
- **Bug Fixes** — Corrected broken behavior
- **Code Cleanup** — Refactoring, dead code removal, modernization, warning fixes
- **Infrastructure** — Build system, tooling, scripts, project structure

## Example

```markdown
### Bug Fixes
- Chat messages were not being delivered to the server — messages appeared empty on the server side
- VSync setting was not being applied when entering the game screen

### Code Cleanup
- Removed unused variables across 5 client files, fixing compiler warnings
- Modernized variable declarations in Game.cpp — moved to point of first use, renamed to descriptive snake_case
- Added descriptive naming guidance to coding standards
```

## Rules

- One bullet per logical change
- Start with what was affected, then briefly explain
- No code references, file paths, or struct names — keep it readable
- Group related changes into a single bullet when appropriate
- Append to the top of `CHANGELOG.md` (newest first)
- If the file is empty or missing, create it fresh with a `# Changelog` header — don't ask
