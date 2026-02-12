# Workflow Details

## Mode 2: Python Script (BULK — 10+ files)

Use only for large-scale mechanical transforms (e.g., "replace X with Y in every file containing X").

Must explicitly justify: "This touches N files with pattern X, a script is appropriate because Y."

1. Write a script in `Scripts/` that calls `bak.py guard` internally.
2. Run the script — it should guard, transform, and build.
3. If errors: fix the script itself, re-run. **Never write a second "fix" script.**
4. Once 0 errors: `python Scripts/bak.py commit` to accept.

## Regex Safety Rules (for Mode 2 scripts)

- **NEVER** use `::TypeName` as regex — matches inside prefixed names (`sf::Color`). Use `(?<!\w)::TypeName\b`.
- **NEVER** replace inside `#define` macro names — `#define DEF_X` cannot become `#define hb::shared::X`.
- **ALWAYS** use `\b` word boundaries — `PlayerStatus` without `\b` matches inside `PlayerStatusData.h`.
- **ALWAYS** use `(?<!\w)` lookbehind for `::Name` — `::GetPoint2` matches inside `CMisc::GetPoint2`.
- **PREFER** `content.replace("exact_old", "exact_new")` over regex for known patterns.
- **ALWAYS** order replacements longest-first — `DEF_OBJECTMOVE_CONFIRM` before `DEF_OBJECTMOVE`.
- **USE** placeholder approach for substring collisions (`SFMLInput::` contains `Input::`).
- **TEST** on 2-3 files first when dealing with regex patterns.

## Include Ordering Pitfall

`GlobalDef.h` and `RenderConstants.h` both define `LOGICAL_WIDTH()`, `LOGICAL_HEIGHT()`, etc.
`RenderConstants.h` guards behind `#ifndef GLOBALDEF_H_RESOLUTION_FUNCTIONS`.

**Rule**: `GlobalDef.h` must be included BEFORE `IRenderer.h` (which pulls in `RenderConstants.h`).
- In .cpp files: include `MapData.h` or `Game.h` (transitively includes `GlobalDef.h`) before render headers.
- In .h files: use forward declarations for `IRenderer`, `SpriteCollection`, etc.
