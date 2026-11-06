# Tests

Qt Test suites for the editor sources in `src/` (everything except `src/runtime/`,
which is covered by its own project). See [TESTING.md](TESTING.md) for the
methodology this folder implements.

## Running

```bash
cmake -S . -B build                       # tests are configured by default
cmake --build build
ctest --test-dir build --output-on-failure          # everything
ctest --test-dir build -L ci                        # the CI subset only
ctest --test-dir build -L local                     # the local-only suites
```

Configure without tests (e.g. for a release build) with `-DCS3_BUILD_TESTS=OFF`.

## Targets

| Target    | Label  | Needs | Covers |
| --------- | ------ | ----- | ------ |
| `tracker` | `ci`   | git, python3 | `scripts/track_sources.py --check`: every file in `src/` is listed in [TRACKER.md](TRACKER.md) with its current `git hash-object` |
| `tst_core` | `ci` | nothing (no `QApplication`, no resources) | `mapfile`, `report`, `stamp`, the undo commands |
| `tst_gui` | `ci`  | `QT_QPA_PLATFORM=offscreen` | dialogs and widgets: `keyvaluedialog`, `dlgattr`, `dlgresize`, `dlgabout`, `dlgstat`, `dlgmaps`, `mapprops`, `MapView`, `MapWidget`, `TileSelectorWidget`, `LayerDock` |
| `tst_local` | `local` | `QT_QPA_PLATFORM=offscreen`, `src/mapedit.qrc`, a hunspell dictionary | `mainwindow`, `dlgselect`, `dlgspell`, `tilebox`, `SpellHighlighter`, the `.ui` files |

`tst_gui` deliberately does **not** link `src/mapedit.qrc`: nothing in it may read
the `:/data/...` resources. Anything that does belongs in `tst_local`
(`CTileBox` is the example - it crashes without the resources, see [REVIEW.md](REVIEW.md)).

Spell checking is skipped unless a dictionary is found. Point the tests at one with

```bash
CS3_SPELL_AFF=/path/to/en_US.aff CS3_SPELL_DIC=/path/to/en_US.dic \
    ctest --test-dir build -L local --output-on-failure
```

## Test data

`tests/in/` holds the fixtures, passed to the suites as `CS3_TEST_DATA_DIR`
(`CS3_SOURCE_DIR` is also defined). `levels1.mapz` is the multi-map archive used
by the read/write tests. `tests/in/refs/` is out of scope (encoding-conversion
references, see [TESTING.md](TESTING.md)).

## Adding a test

1. Pick the suite by environment, not by source file: headless → `tst_core`,
   needs a `QApplication` → `tst_gui`, needs resources/hunspell/`MainWindow` →
   `tst_local`.
2. Add a `private slot` to the existing class; only add a new target in
   `tests/CMakeLists.txt` (via `cs3_add_test()`) when a genuinely new
   environment is required.
3. Never call `exec()` on a dialog - drive it through its public API and
   `findChild<>()`.
4. Update [TRACKER.md](TRACKER.md): set the `Test target` and `Status` columns
   for the files you touched.

## Tracker

```bash
python3 scripts/track_sources.py           # regenerate hashes after editing sources
python3 scripts/track_sources.py --check   # what CI runs: fails on changed/untracked files
```

`--check` reports sources whose hash differs from the recorded one, files that
are missing from the table and stale entries. The `Test target`, `Status` and
`Note` columns are maintained by hand and survive regeneration.

## CI

`.github/workflows/tests.yml` builds on `ubuntu-22.04` and `windows-2022`
(Qt 6.10.3, same as the release workflows) and runs `ctest -L ci`. The `local`
label is not run there: it needs the compiled resources and a dictionary.
