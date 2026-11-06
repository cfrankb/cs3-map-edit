# Test status

Last run: 2026-11-06, local machine (Arch Linux, GCC 16.2.1, CMake 4.4.3,
Qt 6.11.2, hunspell 1.7.3), commit `e2a4e53`.

```bash
cmake -S . -B build && cmake --build build
ctest --test-dir build --output-on-failure
```

| Target | Label | Result | Detail (Qt Test `Totals:`) |
| --- | --- | --- | --- |
| `tracker` | `ci` | PASS | 47 sources tracked, no drift |
| `tst_core` | `ci` | PASS | 31 passed, 0 failed, 0 skipped (~8 ms) |
| `tst_gui` | `ci` | PASS | 18 passed, 0 failed, 0 skipped (~49 ms, offscreen) |
| `tst_local` | `local` | PASS | 10 passed, 0 failed, 1 skipped (~149 ms, offscreen) |

`ctest` reports 4 tests; the totals above are the per-executable Qt Test counts
(including `initTestCase`/`cleanupTestCase`).

`tst_local` skips `spell_underlinesMisspelledWords` when no hunspell dictionary is
configured (`/usr/share/hunspell` is empty on this machine). With
`CS3_SPELL_AFF=/home/cfrankb/spell/en_US.aff CS3_SPELL_DIC=/home/cfrankb/spell/en_US.dic`
it reports `11 passed, 0 failed, 0 skipped`.

## CI

`.github/workflows/tests.yml` runs `ctest -L ci` (`tracker`, `tst_core`,
`tst_gui`) on `ubuntu-22.04` and `windows-2022` with Qt 6.10.3. Not executed
yet - first run happens when the workflow lands on GitHub.

## Coverage (from [TRACKER.md](TRACKER.md))

| Status | Files |
| --- | --- |
| covered | 16 |
| partial | 26 |
| todo | 2 (`dlgtest.cpp/.h`, need a rendered map preview) |
| n/a | 3 (`main.cpp`, `app_version.h`, `SpellTextEdit.h`) |

`partial` means the suite constructs the class and exercises one or two entry
points; painting, mouse input, `exec()`-driven dialogs and file dialogs are not
covered anywhere.

## Bugs found while writing the tests

See [REVIEW.md](REVIEW.md) - the tests currently avoid the broken paths
(`CTileBox` without resources, `CLayer::shift` with tile ids > 255,
`KeyValueDialog::parseStringToUint16` with out-of-range input).
