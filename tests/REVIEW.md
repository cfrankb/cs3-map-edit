# Review

Findings from writing the suites (see [STATUS.md](STATUS.md)). Ordered by
severity. "Test" says what the current suites do about it.

## 1. `CTileBox` crashes when the tile resources are missing

`src/tilebox.cpp`, `setupToolbox()`: the last statement
`m_buttons[m_tile]->setStyleSheet(highlightStyle())` is **outside** the
`if (file.open(":/data/tiles.obl", "rb"))` block, so when the resource cannot be
opened `m_buttons` was never allocated → null dereference (SIGSEGV, observed).
The same array is only filled for non-`FLAG_HIDDEN` tiles, so
`m_buttons[m_tile]` can also be uninitialised garbage on the success path.

Fix: return early inside the `if`, and initialise `m_buttons` entries to
`nullptr`.
Test: `tst_local tileBox_hasPages` covers the working path only; `tst_gui`
excludes `CTileBox` because it does not link `src/mapedit.qrc`.

## 2. `CLayer::shift()` corrupts tile ids and drops the wrapped row

`src/runtime/layer.cpp`: `std::vector<uint8_t> tmp(m_len)` is used to stash rows
of a `std::vector<uint16_t>` tile array.

- `UP`/`DOWN`: the saved row is truncated to 8 bits **and never written back**,
  even though the comments say "place top row at bottom" - the wrapped row ends
  up duplicated from the opposite edge.
- `LEFT`/`RIGHT`: `tmp[0] = *start` truncates the wrapped tile id to 8 bits, so
  any tile id > 255 changes when the map is shifted.

Fix: `std::vector<uint16_t> tmp(...)` and restore the saved row/column.
Test: `tst_core commands_shiftMap` uses `RIGHT` with ids < 256, so it passes
either way.

## 3. `KeyValueDialog::parseStringToUint16()` accepts out-of-range input

`src/keyvaluedialog.cpp`: the `std::stoul` result is narrowed to `uint16_t`
without a range check, so `0x1FFFF` is reported as valid `0xFFFF` and `-1`
becomes `65535`. Long digit strings make `std::stoul` throw
`std::out_of_range` out of a slot.

Fix: parse with `std::from_chars` and reject anything outside `0..0xFFFF`.
Test: `tst_gui keyValueDialog_parseStringToUint16` pins the current behaviour
and avoids throwing input.

## 4. `MainWindow::loadFile()` blocks on a modal dialog

On a read failure it calls `warningMessage()` (`QMessageBox::exec()`), which
hangs any headless test and gives no machine-readable result.

Fix: report through `setStatus()`/a signal and return `bool`; keep the modal for
interactive use only.
Test: `tst_local mainwindow_loadFile*` only loads files that exist.

## 5. Spell dictionary path is hardcoded to a developer home directory

`src/mapprops.cpp:204`:
`new Hunspell("/home/cfrankb/spell/en_US.aff", "/home/cfrankb/spell/en_US.dic")`.
It ignores `DlgSpell::isConfigured()`, leaks the `Hunspell` instance, and on any
other machine silently builds an empty dictionary (every word then counts as
misspelled).

Fix: construct from `DlgSpell::affFile()/dicFile()` when `isConfigured()`, own it
with a `std::unique_ptr`, and share one instance between `MapPropertiesDialog`
and any other editor.

## 6. `SpellTextEdit` is dead code that does not even link

`src/SpellTextEdit.h` declares a `Q_OBJECT` class in a header with no `.cpp` and
it is not listed in any target's sources, so its moc output is never generated:
instantiating it fails with `undefined reference to vtable for SpellTextEdit`.
Adding the header to `mapedit_core`'s `HEADERS` fixes that but breaks builds
without hunspell, because it includes `<hunspell.hxx>` unconditionally.

Fix: either wire it into the message editors (guarded by `USE_HUNSPELL`) or
delete it.
Test: not covered; `tests/TRACKER.md` marks it `n/a`.

## 7. `SpellHighlighter` underlines the wrong occurrence

`src/SpellHighlighter.h`: `int index = text.indexOf(word)` always returns the
first occurrence, so a word repeated in the same block is formatted at the wrong
offset. It also splits on `\W+`, so words carrying punctuation are looked up
whole.

Fix: iterate `QRegularExpression("\\w+")` and use `matchedStart()/matchedLength()`.
Test: `tst_local spell_underlinesMisspelledWords` uses a single occurrence.

## 8. `file(GLOB)` without `CONFIGURE_DEPENDS`

`CMakeLists.txt` globs `src/runtime/*.cpp` and `src/*.cpp`; new files are ignored
until CMake is re-configured (documented in the comment, but easy to trip over).
Fix: add `CONFIGURE_DEPENDS`, or keep the manual re-run and say so in
`tests/README.md` (currently done).

## 9. `src/runtime/statedata copy.h` is staged in git

An accidental duplicate that the tracker does not see (it is under
`src/runtime/`). Remove it before the next commit.

## 10. `USE_HUNSPELL` differs between CI platforms

Linux installs `libhunspell-dev`; Windows builds `external/hunspell` from the
submodule. Both define `USE_HUNSPELL`, but nothing fails if neither does, so the
spell code can silently stop being compiled. Fix: `message(STATUS "hunspell: ON/OFF")`
at configure time, or a CI grep of the generated `compile_commands.json`.

## 11. Mixed error handling in the runtime API

`CLayer::at()/get()` throw `std::out_of_range` while `CMapFile::read()`,
`CLayer::shift()` and friends return `bool`. Callers (and tests) have to know
which is which; `at()` after `clear()` aborts the process. Fix: pick one
convention for the runtime API.

## 12. Stale build definitions

- `src/mapedit.pro` still describes the pre-CMake build (no library split, no
  tests) and will drift further. Delete it or mark it unsupported.
- `AGENTS.md` "CMake layout" still documents the QLang project
  (`libruntime.a`, `examples/qlang_run.cpp`, `tests/scripts/*.ql`). Needs an
  update - ask before editing that file.
