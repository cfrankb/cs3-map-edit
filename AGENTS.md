# Agent instructions

## session start

relooad `TESTING.md`


## plans

akwars write created plans in `docs/plans`. once a plan is completed move it to `docs/plans/archive`

## Ponytail

Follow the Ponytail rules in [ponytail/AGENTS.md](https://github.com/DietrichGebert/ponytail/blob/main/AGENTS.md) for all code changes.   


## CMake layout
- Maintain separate, independently buildable targets:
  - Library: `src/runtime/*.cpp` → `libruntime.a`
  - Demo: e.g., `examples/hello.cpp` (runner: `examples/qlang_run.cpp`); `examples/` holds demos only
  - Tests: `tests/unit/*.cpp` — one executable per feature area (defined in the specification), each registered with `add_test`
  - Script suites: `tests/scripts/*.ql` — QLang regression scripts driven by `qlang_run`

## Commits
- Suggest a commit name; do not commit automatically.
- Stage by concern. Keep unrelated doc edits out of feature commits.
- Exclude or delete patch artifacts (`*.orig`, `*.rej`).

## Edit this file
- Always ask for explicit user permission before editing AGENTS.md.
