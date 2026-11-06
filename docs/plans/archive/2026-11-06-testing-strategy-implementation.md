# Testing strategy implementation

Status: complete — Phases 0–4 done, verified locally 2026-11-06 (`ctest` 4/4).
The CI workflow has not executed on GitHub yet.
Date: 2026-11-06
Source methodology: [tests/TESTING.md](../../../tests/TESTING.md)

## Goal

Implement the methodology described in `tests/TESTING.md`:

- Inventory `src/**.{h,cpp}` (excluding `src/runtime/`, covered by another project).
- Qt Test suites per source file, aiming at full coverage.
- Track `git hash-object` hashes per covered file in `tests/TRACKER.md`.
- Isolate `src/runtime/` as a separately linkable library used by the app and the test targets.
- Split test targets so CI-runnable (headless) and local-only (AUTOUIC / platform-specific) suites are separate.
- Wire the tests into the root `CMakeLists.txt` (`enable_testing()`), add GitHub CI hooks for all platforms already present.
- Maintain `tests/README.md`, `tests/STATUS.md`, `tests/REVIEW.md`.

## Current state (verified)

| Item | Finding |
| --- | --- |
| App sources (non-runtime) | 30 `.cpp`, 40 `.h`, 8 `.ui`, `src/mapedit.qrc`, `src/mapedit.pro` (legacy) |
| `src/runtime/` | 102 files (`runtime/`, `runtime/shared/`, `shared/qtgui/`, `shared/implementers/`, `shared/interfaces/`) |
| Build | single `qt_add_executable` globbing `src/*.cpp` + `src/runtime/*.cpp` + `src/runtime/shared/*.cpp` + `src/runtime/shared/qtgui/*.cpp`, minus `tileset_tex.cpp`; defs `SDL_NOT_WANTED=1`, `USE_QFILE=1`; optional hunspell; zlib bundled in CI |
| Runtime self-containment | runtime includes only its own headers (no back-dependency on app code); no hunspell/SDL use in runtime; `g_layerdata` defined in `runtime/layerdata.cpp` → extractable as a library |
| CI | `.github/workflows/linux_release.yml` (ubuntu-22.04, Qt 6.10.3 gcc, `libhunspell-dev`), `.github/workflows/windows_release_msvc.yml` (windows-2022, VS2022, Qt 6.10.3, hunspell from submodule) |
| Local Qt | Qt 6.11.2 at `/usr/lib`, `Qt6Test` config present (`/usr/lib/cmake/Qt6Test`) |
| `tests/` | only `TESTING.md`, stub `README.md`, `in/` fixtures; no `unit/`, `TRACKER.md`, `STATUS.md`, `REVIEW.md` |

Fixtures available in `tests/in/`: `obl3/`, `obl4/`, `obl5/`, `mcx/`, `ima/`, `imc1/`, `levels1.mapz`,
`annie.obl`, `savegame-cs3.dat`, `test.rec`. `refs/` is out of scope (cross-encoding conversion refs;
`src/runtime/shared/` has its own PNG library independent of GDI).

## Phase 0 — Inventory and hash tracker

- `scripts/track_sources.py`
  - Walks `src/**.{h,cpp}`, excludes `src/runtime/`.
  - Records `git hash-object` per file.
  - Generates/updates `tests/TRACKER.md`: `path | hash | test target | status`.
  - `--check` mode: exits non-zero when a tracked file's hash drifted (source changed without a
    test update) or when a new source file is not yet tracked.
- `tests/TRACKER.md` generated: 47 sources (20 `.cpp`, 27 `.h`), `src/runtime/` excluded,
  `src/main.cpp` and `src/app_version.h` recorded as `n/a`. Verified: `--check` exits 1 on a
  corrupted hash and on a stale row, exits 0 after regeneration.
- `tests/STATUS.md`, `tests/REVIEW.md`, `tests/README.md`: written in Phase 4.

## Phase 1 — CMake restructure (behaviour-preserving)

- `mapruntime` (STATIC): `src/runtime/*.cpp`, `src/runtime/shared/*.cpp`,
  `src/runtime/shared/qtgui/*.cpp`, minus `tileset_tex.cpp`.
  - `PUBLIC` compile definitions `SDL_NOT_WANTED=1`, `USE_QFILE=1` (headers guard on them).
  - `PUBLIC` include dirs `src/runtime`, `src/runtime/shared`, `external`.
  - Links Qt6 `Core`/`Gui`/`Widgets` and zlib (`ZLIB::ZLIB` or system `z`) as `PUBLIC`.
- `mapedit_core` (STATIC): `src/*.cpp` except `src/main.cpp`; `AUTOUIC`/`AUTOMOC` on, `.ui` files and
  hunspell defs/includes propagated as before.
- `cs3-map-edit` executable: `src/main.cpp` + `mapedit_core` + `mapruntime` (behaviour unchanged).
- Root `CMakeLists.txt`: `option(CS3_BUILD_TESTS "Build tests" ON)`, `enable_testing()`,
  `add_subdirectory(tests)` when enabled.

Result (2026-11-06):
- Targets are `mapruntime` and `mapedit_core` (static), not `libruntime.a` - the name in
  `AGENTS.md`'s CMake layout does not match this repo.
- `src/mapedit.qrc` deliberately stays in the executable: resource initializers inside a static
  library can be dropped by the linker without `Q_INIT_RESOURCE`. Consequence for tests: a test
  that needs `:/data/...` (e.g. `report.cpp` reading `:/data/tiles.obl`) must compile the `.qrc`
  into its own target.
- `USE_HUNSPELL` / hunspell includes are `PUBLIC` on `mapedit_core` because `SpellTextEdit.h` and
  `SpellHighlighter.h` expose `<hunspell.hxx>`.
- zlib resolution was hoisted above the targets into `CS3_ZLIB` and linked `PUBLIC` on `mapruntime`.
- Verified: full build clean, `cmake --build build --target mapruntime mapedit_core` builds
  standalone, `QT_QPA_PLATFORM=offscreen ./build/cs3-map-edit` starts and stays alive, resource
  data still present in the executable.

Notes:
- AUTOUIC-generated `ui_*.h` headers stay private to `mapedit_core`; tests construct dialogs/widgets
  through their public headers and do not include `ui_*.h`. Tests that must inspect `.ui` files enable
  `AUTOUIC` themselves and list the `.ui` source (local-only target).
- Static-initialisation order (tile tables, `g_layerdata`) survived the split: the app starts
  offscreen without crashing. Loading `tests/in/levels1.mapz` is exercised by `tst_core` in Phase 2.

## Phase 2 — Test harness

Ponytail constraint: one test executable per *execution environment*, not per source file — the
split that actually matters is headless vs offscreen-GUI vs local-only. Suites are separate
`Qt Test` classes inside the executable, so per-file granularity is kept without a target per file.

| Target | Suites (sources under test) | Runs |
| --- | --- | --- |
| `tst_core` | `mapfile`, `report` (fixtures from `tests/in/`), `undo/*` commands, `stamp.h` | CI, no QPA needed |
| `tst_gui` | `TileSelectorWidget`, `MapView`, `MapWidget`, `LayerDock`, `LayerRowWidget`, `keyvaluedialog`, `dlgabout`, `dlgattr`, `dlgresize`, `dlgstat`, `dlgmaps`, `mapprops` | CI, `QT_QPA_PLATFORM=offscreen` |
| `tst_local` | `mainwindow`, `dlgselect`, `tilebox`, `.ui` widget smoke tests, `dlgspell`/`SpellHighlighter` (hunspell) | local only, per TESTING.md |

Shared CMake boilerplate is one `function(cs3_add_test)` (Qt6::Test + `mapedit_core` + `mapruntime`,
`add_test`, `ci`/`local` label, offscreen env). The hash check needs no executable:
`add_test(NAME tracker COMMAND Python::Interpreter ${CMAKE_SOURCE_DIR}/scripts/track_sources.py --check)`.

Excluded and documented in `tests/README.md`: `src/main.cpp` (process entry, `QLocalServer`
single-instance logic), `src/app_version.h` (build-generated constants), `src/mapedit.pro` (legacy).

Result (2026-11-06):
- `tests/CMakeLists.txt` defines `cs3_add_test(NAME ... LABELS ... QPA ... SOURCES ...)` plus the
  `tracker` CTest entry; Qt Test totals `tst_core` 31, `tst_gui` 18, `tst_local` 10 (1 skip without
  a dictionary), all passing.
- `tst_gui` does **not** compile `src/mapedit.qrc`. That is what surfaced the `CTileBox` crash
  (REVIEW.md #1), so `tilebox` moved to `tst_local`.
- `dlgtest` is not covered (needs a rendered map preview); `SpellTextEdit` is not covered
  (dead code, REVIEW.md #6).
- Spell tests `QSKIP` unless `CS3_SPELL_AFF`/`CS3_SPELL_DIC` point at a dictionary — this machine
  has none in `/usr/share/hunspell`. Verified green with `en_US.aff`/`en_US.dic`.
- `QStandardPaths::setTestModeEnabled(true)` keeps `MainWindow`'s `recentFileList` and the
  `DlgSpell` dictionary settings out of the developer's real configuration.
- `QSyntaxHighlighter` formats live in the block *layout*, not in the document's character formats —
  the spell assertion reads `block.layout()->formats()`.

## Phase 3 — CI

- New `.github/workflows/tests.yml` mirroring the existing platforms and Qt version:
  - `ubuntu-22.04`, Qt 6.10.3 `linux_gcc_64`, `submodules: recursive`, `libhunspell-dev`,
    `cmake -G Ninja -B build -DCMAKE_BUILD_TYPE=Release -DCS3_BUILD_ZLIB_FROM_SOURCE=ON -DCS3_BUILD_TESTS=ON`,
    `ctest --output-on-failure -L ci`.
  - `windows-2022`, VS2022 x64, Qt 6.10.3 `win64_msvc2022_64`, same configure, `ctest -L ci`.
  - `workflow_dispatch` trigger; runs on `pull_request` to `master` and pushes to `master`/`develop`.
- Release workflows stay untouched.

Result (2026-11-06): one workflow, one job, a two-entry matrix (`generator`, `-A x64` and
`-C Release` are matrix keys — Ninja ignores `-A`, so it is only passed on Windows). Configure
passes `-DCS3_BUILD_TESTS=ON` explicitly even though the option defaults to `ON`. The Windows job
prepends `build/zlib/Release` to `PATH` so the bundled zlib DLL is found by the test executables
(the release workflow copies it next to the app instead). YAML validated locally; the workflow
itself has not run yet — first execution happens on GitHub.

## Phase 4 — Documentation and review

- `tests/README.md`: layout, conventions, how to build and run locally and in CI, how to add a test.
- `tests/STATUS.md`: results per target and platform (pass/fail/skip, date, Qt version).
- `tests/REVIEW.md`: recommendations. Already queued:
  - `CMakeLists.txt` comment claims `layerdata.cpp` is excluded from the build, but only
    `tileset_tex.cpp` is in `list(REMOVE_ITEM ...)`; the comment is stale.
  - `src/runtime/statedata copy.h` is staged in git — looks like an accidental duplicate.
  - `file(GLOB ...)` without `CONFIGURE_DEPENDS` requires manual re-configure when files are added.
  - hunspell is unavailable on the Windows CI runner, so spell-related code paths are untested there.
  - No coverage tooling yet (`gcov`/`lcov` locally, or OpenCppCoverage on Windows) — recommend a
    local-only coverage target.
  - `src/mapedit.pro` is legacy and drifts from the CMake build.
  - The "CMake layout" section of `AGENTS.md` still describes the QLang project
    (`examples/qlang_run.cpp`, `tests/scripts/*.ql`, `libruntime.a`); this repo has no `examples/`
    and no QLang. Needs a rewrite (requires explicit user permission to edit `AGENTS.md`).

## Verification

- After each phase: `cmake -S . -B build`, `cmake --build build`, `ctest --output-on-failure`
  (local), plus `ctest -L ci` to confirm the CI subset passes headless.
- The app binary must build and launch unchanged after the Phase 1 library split.
- Final run (2026-11-06, commit `e2a4e53`): configure + build clean, `ctest` 4/4 passed
  (`tracker`, `tst_core`, `tst_gui` labelled `ci`; `tst_local` labelled `local`), and
  `tst_local` 11/11 with `CS3_SPELL_AFF`/`CS3_SPELL_DIC` set.
- Suggested commit split: (1) `build: split runtime and core libraries`, (2) `test: add Qt Test harness and suites`,
  (3) `ci: run tests on Linux and Windows`, (4) `docs: testing tracker, status and review`.
